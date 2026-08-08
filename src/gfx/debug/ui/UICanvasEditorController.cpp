#include "gfx/debug/ui/UICanvasEditorController.h"

#include "gfx/UIRenderer.h"
#include "Game.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <limits>

namespace {
constexpr float MinimumElementSizePixels = 4.0f;

float NormalizeDegrees(float degrees)
{
    while (degrees > 180.0f) {
        degrees -= 360.0f;
    }
    while (degrees < -180.0f) {
        degrees += 360.0f;
    }
    return degrees;
}

glm::vec2 RotatePoint(const glm::vec2& point, float degrees)
{
    const float radians = glm::radians(degrees);
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    return glm::vec2(
        point.x * cosine - point.y * sine,
        point.x * sine + point.y * cosine);
}
}

UICanvasEditorController::UICanvasEditorController(DebugEditorContext& context)
    : mContext(context)
{
}

void UICanvasEditorController::Update(
    UILoadSystem* uiLoadSystem,
    std::string& statusMessage)
{
    if (!uiLoadSystem || !mContext.uiRenderer) {
        return;
    }

    ValidateSelection(uiLoadSystem);
    HandleShortcuts(uiLoadSystem, statusMessage);
    UpdateCanvasSelection(uiLoadSystem);
    DrawSelectionOverlay(uiLoadSystem);
    DrawGizmo(uiLoadSystem, statusMessage);
}

void UICanvasEditorController::SetSingleSelection(std::size_t index)
{
    SetSingleSelection(
        ElementReference{SelectionSource::Custom, index, std::string()});
}

void UICanvasEditorController::ToggleSelection(std::size_t index)
{
    ToggleSelection(
        ElementReference{SelectionSource::Custom, index, std::string()});
}

void UICanvasEditorController::SelectFromList(std::size_t index, bool additive)
{
    if (additive) {
        ToggleSelection(index);
    } else {
        SetSingleSelection(index);
    }
}

void UICanvasEditorController::SelectExistingTextureFromList(
    const std::string& key,
    bool additive)
{
    const ElementReference element{
        SelectionSource::ExistingTexture,
        0,
        key};
    if (additive) {
        ToggleSelection(element);
    } else {
        SetSingleSelection(element);
    }
}

void UICanvasEditorController::SelectExistingTextFromList(
    const std::string& key,
    bool additive)
{
    const ElementReference element{
        SelectionSource::ExistingText,
        0,
        key};
    if (additive) {
        ToggleSelection(element);
    } else {
        SetSingleSelection(element);
    }
}

void UICanvasEditorController::ClearSelection()
{
    mSelectedElements.clear();
    mPrimarySelection.reset();
    mHasLastPick = false;
}

bool UICanvasEditorController::IsSelected(std::size_t index) const
{
    return IsSelected(
        ElementReference{SelectionSource::Custom, index, std::string()});
}

bool UICanvasEditorController::IsExistingTextureSelected(
    const std::string& key) const
{
    return IsSelected(
        ElementReference{SelectionSource::ExistingTexture, 0, key});
}

bool UICanvasEditorController::IsExistingTextSelected(
    const std::string& key) const
{
    return IsSelected(
        ElementReference{SelectionSource::ExistingText, 0, key});
}

int UICanvasEditorController::GetPrimarySelectedIndex() const
{
    if (!mPrimarySelection ||
        mPrimarySelection->source != SelectionSource::Custom) {
        return -1;
    }
    return static_cast<int>(mPrimarySelection->customIndex);
}

UICanvasEditorController::SelectionSource
UICanvasEditorController::GetPrimarySelectionSource() const
{
    return mPrimarySelection
               ? mPrimarySelection->source
               : SelectionSource::None;
}

const std::string& UICanvasEditorController::GetPrimaryExistingKey() const
{
    static const std::string EmptyKey;
    return mPrimarySelection ? mPrimarySelection->existingKey : EmptyKey;
}

std::size_t UICanvasEditorController::GetSelectedCount() const
{
    return mSelectedElements.size();
}

void UICanvasEditorController::SetSingleSelection(
    const ElementReference& element)
{
    mSelectedElements.assign(1, element);
    mPrimarySelection = element;
}

void UICanvasEditorController::ToggleSelection(
    const ElementReference& element)
{
    const auto selectedIt = std::find(
        mSelectedElements.begin(),
        mSelectedElements.end(),
        element);
    if (selectedIt != mSelectedElements.end()) {
        const bool removedPrimary =
            mPrimarySelection && *mPrimarySelection == element;
        mSelectedElements.erase(selectedIt);
        if (removedPrimary) {
            if (mSelectedElements.empty()) {
                mPrimarySelection.reset();
            } else {
                mPrimarySelection = mSelectedElements.back();
            }
        }
        return;
    }

    mSelectedElements.push_back(element);
    mPrimarySelection = element;
}

bool UICanvasEditorController::IsSelected(
    const ElementReference& element) const
{
    return std::find(
               mSelectedElements.begin(),
               mSelectedElements.end(),
               element) != mSelectedElements.end();
}

void UICanvasEditorController::SetOperation(Operation operation)
{
    if (mIsUsingGizmo) {
        return;
    }
    mOperation = operation;
}

UICanvasEditorController::Operation UICanvasEditorController::GetOperation() const
{
    return mOperation;
}

bool UICanvasEditorController::DuplicateSelected(
    UILoadSystem* uiLoadSystem,
    std::string& statusMessage)
{
    if (!uiLoadSystem || mSelectedElements.empty()) {
        return false;
    }

    PushUndo(uiLoadSystem);

    std::vector<std::size_t> sourceIndices;
    for (const ElementReference& element : mSelectedElements) {
        if (element.source == SelectionSource::Custom) {
            sourceIndices.push_back(element.customIndex);
        }
    }
    if (sourceIndices.empty()) {
        mUndoStack.pop_back();
        statusMessage = "コード連携UIは複製できません";
        return false;
    }
    std::sort(sourceIndices.begin(), sourceIndices.end());

    std::vector<ElementReference> duplicatedElements;
    for (std::size_t sourceIndex : sourceIndices) {
        const std::optional<std::size_t> duplicatedIndex =
            uiLoadSystem->DuplicateCustomElement(sourceIndex);
        if (duplicatedIndex) {
            duplicatedElements.push_back(
                ElementReference{
                    SelectionSource::Custom,
                    *duplicatedIndex,
                    std::string()});
        }
    }

    if (duplicatedElements.empty()) {
        mUndoStack.pop_back();
        statusMessage = "要素の複製に失敗しました";
        return false;
    }

    mSelectedElements = std::move(duplicatedElements);
    mPrimarySelection = mSelectedElements.back();

    const bool saved = uiLoadSystem->SaveCustomUI();
    statusMessage =
        saved ? "選択中のUIを複製して保存しました" : "UIは複製しましたが保存に失敗しました";
    return true;
}

bool UICanvasEditorController::DeleteSelected(
    UILoadSystem* uiLoadSystem,
    std::string& statusMessage)
{
    if (!uiLoadSystem || mSelectedElements.empty()) {
        return false;
    }

    PushUndo(uiLoadSystem);

    std::vector<std::size_t> deleteIndices;
    for (const ElementReference& element : mSelectedElements) {
        if (element.source == SelectionSource::Custom) {
            deleteIndices.push_back(element.customIndex);
        }
    }
    if (deleteIndices.empty()) {
        mUndoStack.pop_back();
        statusMessage = "コード連携UIは削除できません";
        return false;
    }
    std::sort(deleteIndices.rbegin(), deleteIndices.rend());

    bool deleted = false;
    for (std::size_t index : deleteIndices) {
        deleted |= uiLoadSystem->RemoveCustomElement(index);
    }

    if (!deleted) {
        mUndoStack.pop_back();
        statusMessage = "UIの削除に失敗しました";
        return false;
    }

    ClearSelection();
    const bool saved = uiLoadSystem->SaveCustomUI();
    statusMessage =
        saved ? "選択中のUIを削除して保存しました" : "UIは削除しましたが保存に失敗しました";
    return true;
}

bool UICanvasEditorController::RestoreUndo(
    UILoadSystem* uiLoadSystem,
    std::string& statusMessage)
{
    if (!uiLoadSystem || mUndoStack.empty()) {
        return false;
    }

    UndoState undoState = std::move(mUndoStack.back());
    mUndoStack.pop_back();
    uiLoadSystem->GetCustomElements() = std::move(undoState.customElements);
    uiLoadSystem->GetEditableTextureInfos() = std::move(undoState.textureInfos);
    uiLoadSystem->GetEditableTextInfos() = std::move(undoState.textInfos);
    uiLoadSystem->ClearCustomVisibilityOverrides();
    ClearSelection();

    const bool savedCustomUI = uiLoadSystem->SaveCustomUI();
    const bool savedExistingUI =
        uiLoadSystem->SaveUIInfo("../assets/data/ui/ui.yaml");
    const bool saved = savedCustomUI && savedExistingUI;
    statusMessage =
        saved ? "UI編集を元に戻して保存しました" : "UI編集は戻しましたが保存に失敗しました";
    return true;
}

void UICanvasEditorController::ValidateSelection(const UILoadSystem* uiLoadSystem)
{
    if (!uiLoadSystem) {
        ClearSelection();
        return;
    }

    const std::size_t customElementCount =
        uiLoadSystem->GetCustomElements().size();
    const auto& textureInfos = uiLoadSystem->GetEditableTextureInfos();
    const auto& textInfos = uiLoadSystem->GetEditableTextInfos();
    std::erase_if(
        mSelectedElements,
        [&](const ElementReference& element) {
            switch (element.source) {
            case SelectionSource::Custom:
                return element.customIndex >= customElementCount;
            case SelectionSource::ExistingTexture:
                return !textureInfos.contains(element.existingKey);
            case SelectionSource::ExistingText:
                return !textInfos.contains(element.existingKey);
            case SelectionSource::None:
                return true;
            }
            return true;
        });

    if (mPrimarySelection && !IsSelected(*mPrimarySelection)) {
        mPrimarySelection.reset();
    }
    if (!mPrimarySelection && !mSelectedElements.empty()) {
        mPrimarySelection = mSelectedElements.back();
    }
}

void UICanvasEditorController::HandleShortcuts(
    UILoadSystem* uiLoadSystem,
    std::string& statusMessage)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput || ImGui::IsAnyItemActive()) {
        return;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_E, false)) {
        SetOperation(Operation::Translate);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_R, false)) {
        SetOperation(Operation::Rotate);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_T, false)) {
        SetOperation(Operation::Scale);
    }

    const bool primaryModifier = io.KeyCtrl || io.KeySuper;
    if (primaryModifier && ImGui::IsKeyPressed(ImGuiKey_D, false)) {
        DuplicateSelected(uiLoadSystem, statusMessage);
        return;
    }
    if (primaryModifier && ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
        RestoreUndo(uiLoadSystem, statusMessage);
        return;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Delete, false) ||
        ImGui::IsKeyPressed(ImGuiKey_Backspace, false)) {
        DeleteSelected(uiLoadSystem, statusMessage);
    }
}

void UICanvasEditorController::UpdateCanvasSelection(const UILoadSystem* uiLoadSystem)
{
    if (!mContext.game || !mContext.game->GetWindow()) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse || mIsUsingGizmo || ImGuizmo::IsOver() || ImGuizmo::IsUsing()) {
        return;
    }

    const ImVec2 mousePosition = ImGui::GetMousePos();
    if (mContext.gameViewport.IsValid() && !mIsBoxMouseDown) {
        const DebugEditorGameViewport& gameViewport =
            mContext.gameViewport;
        const bool isInsideGameViewport =
            mousePosition.x >= gameViewport.x &&
            mousePosition.x <= gameViewport.x + gameViewport.width &&
            mousePosition.y >= gameViewport.y &&
            mousePosition.y <= gameViewport.y + gameViewport.height;
        if (!isInsideGameViewport) {
            return;
        }
    }
    const bool additive = io.KeyCtrl || io.KeySuper || io.KeyShift;

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        const std::vector<ElementReference> hits =
            PickElementsAt(uiLoadSystem, ImGuiToFramebuffer(mousePosition));

        if (!hits.empty()) {
            std::size_t hitIndex = 0;
            if (!additive && mHasLastPick) {
                const float deltaX = mousePosition.x - mLastPickPosition.x;
                const float deltaY = mousePosition.y - mLastPickPosition.y;
                constexpr float CycleRadius = 6.0f;
                if (deltaX * deltaX + deltaY * deltaY <= CycleRadius * CycleRadius) {
                    const auto current =
                        mPrimarySelection
                            ? std::find(hits.begin(), hits.end(), *mPrimarySelection)
                            : hits.end();
                    if (current != hits.end()) {
                        hitIndex =
                            (static_cast<std::size_t>(std::distance(hits.begin(), current)) + 1) %
                            hits.size();
                    }
                }
            }

            mLastPickPosition = mousePosition;
            mHasLastPick = true;

            if (additive) {
                ToggleSelection(hits[hitIndex]);
            } else {
                SetSingleSelection(hits[hitIndex]);
            }
            return;
        }

        mIsBoxMouseDown = true;
        mIsBoxSelecting = false;
        mBoxStart = mousePosition;
        mBoxEnd = mousePosition;
        if (!additive) {
            ClearSelection();
        }
    }

    if (mIsBoxMouseDown && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        mBoxEnd = mousePosition;
        const float deltaX = mBoxEnd.x - mBoxStart.x;
        const float deltaY = mBoxEnd.y - mBoxStart.y;
        mIsBoxSelecting = deltaX * deltaX + deltaY * deltaY > 25.0f;
    }

    if (mIsBoxMouseDown && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        if (mIsBoxSelecting) {
            const ImVec2 rectMin(
                std::min(mBoxStart.x, mBoxEnd.x),
                std::min(mBoxStart.y, mBoxEnd.y));
            const ImVec2 rectMax(
                std::max(mBoxStart.x, mBoxEnd.x),
                std::max(mBoxStart.y, mBoxEnd.y));
            SelectElementsInBox(uiLoadSystem, rectMin, rectMax, additive);
        }
        mIsBoxMouseDown = false;
        mIsBoxSelecting = false;
    }
}

void UICanvasEditorController::DrawSelectionOverlay(const UILoadSystem* uiLoadSystem) const
{
    if (!uiLoadSystem) {
        return;
    }

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    for (const ElementReference& element : mSelectedElements) {
        ElementTransform transform;
        if (!ResolveElementTransform(uiLoadSystem, element, transform)) {
            continue;
        }

        const glm::vec2 halfSize = transform.size * 0.5f;
        const glm::vec2 localCorners[] = {
            {-halfSize.x, -halfSize.y},
            {halfSize.x, -halfSize.y},
            {halfSize.x, halfSize.y},
            {-halfSize.x, halfSize.y},
        };

        ImVec2 points[4];
        for (int corner = 0; corner < 4; ++corner) {
            points[corner] =
                FramebufferToImGui(
                    transform.center +
                    RotatePoint(localCorners[corner], transform.rotationDegrees));
        }

        const ImU32 color =
            mPrimarySelection && *mPrimarySelection == element
                ? IM_COL32(255, 190, 40, 255)
                : IM_COL32(80, 190, 255, 235);
        for (int corner = 0; corner < 4; ++corner) {
            drawList->AddLine(points[corner], points[(corner + 1) % 4], color, 2.0f);
        }
    }

    if (mIsBoxSelecting) {
        const ImVec2 rectMin(
            std::min(mBoxStart.x, mBoxEnd.x),
            std::min(mBoxStart.y, mBoxEnd.y));
        const ImVec2 rectMax(
            std::max(mBoxStart.x, mBoxEnd.x),
            std::max(mBoxStart.y, mBoxEnd.y));
        drawList->AddRectFilled(rectMin, rectMax, IM_COL32(80, 190, 255, 35));
        drawList->AddRect(rectMin, rectMax, IM_COL32(80, 190, 255, 230), 0.0f, 0, 2.0f);
    }
}

void UICanvasEditorController::DrawGizmo(
    UILoadSystem* uiLoadSystem,
    std::string& statusMessage)
{
    if (!uiLoadSystem || mSelectedElements.empty() || !mContext.uiRenderer) {
        mIsUsingGizmo = false;
        return;
    }

    const int framebufferWidth = mContext.uiRenderer->GetFbWidth();
    const int framebufferHeight = mContext.uiRenderer->GetFbHeight();
    if (framebufferWidth <= 0 || framebufferHeight <= 0) {
        return;
    }

    if (!mIsUsingGizmo) {
        mEditingMatrix = CreateSelectionMatrix(uiLoadSystem);
    }

    const glm::mat4 matrixBeforeManipulation = mEditingMatrix;
    const float halfWidth = static_cast<float>(framebufferWidth) * 0.5f;
    const float halfHeight = static_cast<float>(framebufferHeight) * 0.5f;
    const float cameraDistance =
        std::max(static_cast<float>(framebufferWidth), static_cast<float>(framebufferHeight));
    const glm::vec3 cameraTarget(halfWidth, halfHeight, 0.0f);
    const glm::mat4 view = glm::lookAt(
        cameraTarget + glm::vec3(0.0f, 0.0f, cameraDistance),
        cameraTarget,
        glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 projection =
        glm::ortho(
            -halfWidth,
            halfWidth,
            halfHeight,
            -halfHeight,
            0.1f,
            cameraDistance * 2.0f);

    ImGuizmo::BeginFrame();
    ImGuizmo::SetOrthographic(true);
    ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());
    if (mContext.gameViewport.IsValid()) {
        ImGuizmo::SetRect(
            mContext.gameViewport.x,
            mContext.gameViewport.y,
            mContext.gameViewport.width,
            mContext.gameViewport.height);
    } else {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGuizmo::SetRect(
            viewport->Pos.x,
            viewport->Pos.y,
            viewport->Size.x,
            viewport->Size.y);
    }

    ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE_X | ImGuizmo::TRANSLATE_Y;
    ImGuizmo::MODE mode = ImGuizmo::WORLD;
    if (mOperation == Operation::Rotate) {
        operation = ImGuizmo::ROTATE_Z;
        mode = ImGuizmo::LOCAL;
    } else if (mOperation == Operation::Scale) {
        operation = ImGuizmo::SCALE_X | ImGuizmo::SCALE_Y;
        mode = ImGuizmo::LOCAL;
    }

    ImGuizmo::PushID("UICanvasEditorGizmo");
    ImGuizmo::Manipulate(
        glm::value_ptr(view),
        glm::value_ptr(projection),
        operation,
        mode,
        glm::value_ptr(mEditingMatrix));
    const bool isUsingGizmo = ImGuizmo::IsUsing();
    ImGuizmo::PopID();

    if (isUsingGizmo) {
        if (!mIsUsingGizmo) {
            PushUndo(uiLoadSystem);
            mTransformStartState.customElements = uiLoadSystem->GetCustomElements();
            mTransformStartState.textureInfos = uiLoadSystem->GetEditableTextureInfos();
            mTransformStartState.textInfos = uiLoadSystem->GetEditableTextInfos();
            mTransformStartSelectionMatrix = matrixBeforeManipulation;
            mIsUsingGizmo = true;
        }

        ApplySelectionMatrix(uiLoadSystem, mEditingMatrix);
        return;
    }

    if (mIsUsingGizmo) {
        mIsUsingGizmo = false;
        mTransformStartState = UndoState{};
        const bool savedCustomUI = uiLoadSystem->SaveCustomUI();
        const bool savedExistingUI =
            uiLoadSystem->SaveUIInfo("../assets/data/ui/ui.yaml");
        const bool saved = savedCustomUI && savedExistingUI;
        statusMessage =
            saved ? "UIギズモの変更を保存しました" : "UIギズモの変更を保存できませんでした";
    }
}

bool UICanvasEditorController::ResolveElementTransform(
    const UILoadSystem* uiLoadSystem,
    const ElementReference& element,
    ElementTransform& outTransform) const
{
    if (!uiLoadSystem || !mContext.uiRenderer) {
        return false;
    }

    UIRenderer::CustomElementScreenTransform screenTransform;
    switch (element.source) {
    case SelectionSource::Custom:
    {
        const auto& customElements = uiLoadSystem->GetCustomElements();
        if (element.customIndex >= customElements.size()) {
            return false;
        }
        const UILoadSystem::CustomElement& customElement =
            customElements[element.customIndex];
        if (!mContext.uiRenderer->CalculateCustomElementScreenTransform(
                customElement,
                screenTransform)) {
            return false;
        }
        outTransform.rotationDegrees = customElement.rotationDegrees;
        break;
    }
    case SelectionSource::ExistingTexture:
    {
        const auto& textureInfos = uiLoadSystem->GetEditableTextureInfos();
        const auto textureInfoIt = textureInfos.find(element.existingKey);
        if (textureInfoIt == textureInfos.end() ||
            !mContext.uiRenderer->CalculateTextureInfoScreenTransform(
                textureInfoIt->second,
                screenTransform)) {
            return false;
        }
        outTransform.rotationDegrees = textureInfoIt->second.rotationDegrees;
        break;
    }
    case SelectionSource::ExistingText:
    {
        const auto& textInfos = uiLoadSystem->GetEditableTextInfos();
        const auto textInfoIt = textInfos.find(element.existingKey);
        if (textInfoIt == textInfos.end() ||
            !mContext.uiRenderer->CalculateTextInfoScreenTransform(
                textInfoIt->second,
                screenTransform)) {
            return false;
        }
        outTransform.rotationDegrees = textInfoIt->second.rotationDegrees;
        break;
    }
    case SelectionSource::None:
        return false;
    }

    outTransform.center = screenTransform.center;
    outTransform.size = glm::max(
        screenTransform.size,
        glm::vec2(MinimumElementSizePixels));
    return true;
}

bool UICanvasEditorController::GetSelectionBounds(
    const UILoadSystem* uiLoadSystem,
    glm::vec2& outMin,
    glm::vec2& outMax) const
{
    if (!uiLoadSystem || mSelectedElements.empty()) {
        return false;
    }

    outMin = glm::vec2(std::numeric_limits<float>::max());
    outMax = glm::vec2(std::numeric_limits<float>::lowest());
    bool hasBounds = false;

    for (const ElementReference& element : mSelectedElements) {
        ElementTransform transform;
        if (!ResolveElementTransform(uiLoadSystem, element, transform)) {
            continue;
        }

        const glm::vec2 halfSize = transform.size * 0.5f;
        const glm::vec2 localCorners[] = {
            {-halfSize.x, -halfSize.y},
            {halfSize.x, -halfSize.y},
            {halfSize.x, halfSize.y},
            {-halfSize.x, halfSize.y},
        };

        for (const glm::vec2& localCorner : localCorners) {
            const glm::vec2 corner =
                transform.center +
                RotatePoint(localCorner, transform.rotationDegrees);
            outMin = glm::min(outMin, corner);
            outMax = glm::max(outMax, corner);
        }
        hasBounds = true;
    }

    return hasBounds;
}

std::vector<UICanvasEditorController::ElementReference>
UICanvasEditorController::PickElementsAt(
    const UILoadSystem* uiLoadSystem,
    const glm::vec2& framebufferPoint) const
{
    std::vector<ElementReference> hits;
    if (!uiLoadSystem) {
        return hits;
    }

    const auto& customElements = uiLoadSystem->GetCustomElements();
    const auto& renderedElements =
        mContext.uiRenderer->GetRenderedUIElements();
    for (auto renderedIt = renderedElements.rbegin();
         renderedIt != renderedElements.rend();
         ++renderedIt) {
        const UIRenderer::RenderedUIElement& renderedElement = *renderedIt;

        ElementReference element;
        switch (renderedElement.source) {
        case UIRenderer::RenderedUIElementSource::Custom:
        {
            const auto customIt = std::find_if(
                customElements.begin(),
                customElements.end(),
                [&](const UILoadSystem::CustomElement& candidate) {
                    return candidate.screen == renderedElement.screen &&
                           candidate.id == renderedElement.id;
                });
            if (customIt == customElements.end()) {
                continue;
            }
            element.source = SelectionSource::Custom;
            element.customIndex = static_cast<std::size_t>(
                std::distance(customElements.begin(), customIt));
            break;
        }
        case UIRenderer::RenderedUIElementSource::CodeBoundTexture:
            element.source = SelectionSource::ExistingTexture;
            element.existingKey =
                renderedElement.screen + "." + renderedElement.id;
            break;
        case UIRenderer::RenderedUIElementSource::CodeBoundText:
            element.source = SelectionSource::ExistingText;
            element.existingKey =
                renderedElement.screen + "." + renderedElement.id;
            break;
        }

        ElementTransform transform;
        transform.center = renderedElement.transform.center;
        transform.size = glm::max(
            renderedElement.transform.size,
            glm::vec2(MinimumElementSizePixels));
        transform.rotationDegrees = renderedElement.rotationDegrees;
        if (!IsPointInsideElement(transform, framebufferPoint) ||
            std::find(hits.begin(), hits.end(), element) != hits.end()) {
            continue;
        }
        hits.push_back(std::move(element));
    }
    return hits;
}

bool UICanvasEditorController::IsPointInsideElement(
    const ElementTransform& transform,
    const glm::vec2& framebufferPoint) const
{
    const glm::vec2 local =
        RotatePoint(
            framebufferPoint - transform.center,
            -transform.rotationDegrees);
    const glm::vec2 halfSize = transform.size * 0.5f;
    return std::abs(local.x) <= halfSize.x &&
           std::abs(local.y) <= halfSize.y;
}

glm::vec2 UICanvasEditorController::ImGuiToFramebuffer(const ImVec2& point) const
{
    if (!mContext.uiRenderer) {
        return glm::vec2(0.0f);
    }

    if (mContext.gameViewport.IsValid()) {
        const DebugEditorGameViewport& gameViewport =
            mContext.gameViewport;
        return glm::vec2(
            (point.x - gameViewport.x) /
                gameViewport.width *
                static_cast<float>(gameViewport.sourceWidth),
            (point.y - gameViewport.y) /
                gameViewport.height *
                static_cast<float>(gameViewport.sourceHeight));
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float width = std::max(viewport->Size.x, 1.0f);
    const float height = std::max(viewport->Size.y, 1.0f);
    return glm::vec2(
        (point.x - viewport->Pos.x) / width *
            static_cast<float>(mContext.uiRenderer->GetFbWidth()),
        (point.y - viewport->Pos.y) / height *
            static_cast<float>(mContext.uiRenderer->GetFbHeight()));
}

ImVec2 UICanvasEditorController::FramebufferToImGui(const glm::vec2& point) const
{
    if (!mContext.uiRenderer) {
        return ImVec2(0.0f, 0.0f);
    }

    const float framebufferWidth =
        static_cast<float>(std::max(mContext.uiRenderer->GetFbWidth(), 1));
    const float framebufferHeight =
        static_cast<float>(std::max(mContext.uiRenderer->GetFbHeight(), 1));

    if (mContext.gameViewport.IsValid()) {
        return ImVec2(
            mContext.gameViewport.x +
                point.x / framebufferWidth *
                mContext.gameViewport.width,
            mContext.gameViewport.y +
                point.y / framebufferHeight *
                mContext.gameViewport.height);
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    return ImVec2(
        viewport->Pos.x + point.x / framebufferWidth * viewport->Size.x,
        viewport->Pos.y + point.y / framebufferHeight * viewport->Size.y);
}

glm::mat4 UICanvasEditorController::CreateElementMatrix(
    const ElementTransform& transform) const
{
    return
        glm::translate(
            glm::mat4(1.0f),
            glm::vec3(transform.center, 0.0f)) *
        glm::rotate(
            glm::mat4(1.0f),
            glm::radians(transform.rotationDegrees),
            glm::vec3(0.0f, 0.0f, 1.0f)) *
        glm::scale(
            glm::mat4(1.0f),
            glm::vec3(transform.size, 1.0f));
}

glm::mat4 UICanvasEditorController::CreateSelectionMatrix(
    const UILoadSystem* uiLoadSystem) const
{
    if (!uiLoadSystem || mSelectedElements.empty()) {
        return glm::mat4(1.0f);
    }

    if (mSelectedElements.size() == 1) {
        ElementTransform transform;
        if (ResolveElementTransform(
                uiLoadSystem,
                mSelectedElements.front(),
                transform)) {
            return CreateElementMatrix(transform);
        }
    }

    glm::vec2 boundsMin;
    glm::vec2 boundsMax;
    if (!GetSelectionBounds(uiLoadSystem, boundsMin, boundsMax)) {
        return glm::mat4(1.0f);
    }

    ElementTransform groupTransform;
    groupTransform.center = (boundsMin + boundsMax) * 0.5f;
    groupTransform.size =
        glm::max(boundsMax - boundsMin, glm::vec2(MinimumElementSizePixels));
    return CreateElementMatrix(groupTransform);
}

void UICanvasEditorController::ApplySelectionMatrix(
    UILoadSystem* uiLoadSystem,
    const glm::mat4& selectionMatrix)
{
    if (!uiLoadSystem || !mContext.uiRenderer) {
        return;
    }

    const float framebufferWidth =
        static_cast<float>(mContext.uiRenderer->GetFbWidth());
    const float framebufferHeight =
        static_cast<float>(mContext.uiRenderer->GetFbHeight());
    if (framebufferWidth <= 0.0f || framebufferHeight <= 0.0f) {
        return;
    }

    const glm::mat4 deltaMatrix =
        selectionMatrix * glm::inverse(mTransformStartSelectionMatrix);
    auto& currentElements = uiLoadSystem->GetCustomElements();
    auto& currentTextureInfos = uiLoadSystem->GetEditableTextureInfos();
    auto& currentTextInfos = uiLoadSystem->GetEditableTextInfos();

    const auto resolveStartTransform =
        [&](const ElementReference& element,
            ElementTransform& outTransform) {
            UIRenderer::CustomElementScreenTransform screenTransform;
            switch (element.source) {
            case SelectionSource::Custom:
                if (element.customIndex >=
                    mTransformStartState.customElements.size()) {
                    return false;
                }
                if (!mContext.uiRenderer->CalculateCustomElementScreenTransform(
                        mTransformStartState.customElements[element.customIndex],
                        screenTransform)) {
                    return false;
                }
                outTransform.rotationDegrees =
                    mTransformStartState.customElements[element.customIndex]
                        .rotationDegrees;
                break;
            case SelectionSource::ExistingTexture:
            {
                const auto originalIt =
                    mTransformStartState.textureInfos.find(element.existingKey);
                if (originalIt == mTransformStartState.textureInfos.end() ||
                    !mContext.uiRenderer->CalculateTextureInfoScreenTransform(
                        originalIt->second,
                        screenTransform)) {
                    return false;
                }
                outTransform.rotationDegrees =
                    originalIt->second.rotationDegrees;
                break;
            }
            case SelectionSource::ExistingText:
            {
                const auto originalIt =
                    mTransformStartState.textInfos.find(element.existingKey);
                if (originalIt == mTransformStartState.textInfos.end() ||
                    !mContext.uiRenderer->CalculateTextInfoScreenTransform(
                        originalIt->second,
                        screenTransform)) {
                    return false;
                }
                outTransform.rotationDegrees =
                    originalIt->second.rotationDegrees;
                break;
            }
            case SelectionSource::None:
                return false;
            }

            outTransform.center = screenTransform.center;
            outTransform.size = glm::max(
                screenTransform.size,
                glm::vec2(MinimumElementSizePixels));
            return true;
        };

    for (const ElementReference& element : mSelectedElements) {
        ElementTransform originalTransform;
        if (!resolveStartTransform(element, originalTransform)) {
            continue;
        }

        const glm::mat4 transformedMatrix =
            deltaMatrix * CreateElementMatrix(originalTransform);

        float translation[3];
        float rotation[3];
        float scale[3];
        ImGuizmo::DecomposeMatrixToComponents(
            glm::value_ptr(transformedMatrix),
            translation,
            rotation,
            scale);

        const glm::vec2 newCenter(translation[0], translation[1]);
        const glm::vec2 newSize(
            std::max(std::abs(scale[0]), MinimumElementSizePixels),
            std::max(std::abs(scale[1]), MinimumElementSizePixels));

        const float newRotationDegrees = NormalizeDegrees(rotation[2]);

        if (element.source == SelectionSource::Custom) {
            if (element.customIndex >= currentElements.size() ||
                element.customIndex >=
                    mTransformStartState.customElements.size()) {
                continue;
            }

            UILoadSystem::CustomElement& currentElement =
                currentElements[element.customIndex];
            const UILoadSystem::CustomElement& originalElement =
                mTransformStartState.customElements[element.customIndex];
            currentElement.rotationDegrees = newRotationDegrees;

            if (currentElement.type == UILoadSystem::CustomElementType::Text) {
                const float scaleX =
                    newSize.x / std::max(originalTransform.size.x, 1.0f);
                const float scaleY =
                    newSize.y / std::max(originalTransform.size.y, 1.0f);
                const float uniformScale =
                    std::sqrt(std::max(scaleX * scaleY, 0.0001f));
                currentElement.textScaleRatio =
                    std::clamp(
                        originalElement.textScaleRatio * uniformScale,
                        0.00005f,
                        0.003f);
            } else {
                currentElement.widthRatio = newSize.x / framebufferWidth;
                currentElement.heightRatio = newSize.y / framebufferWidth;
            }

            if (currentElement.centerBased) {
                currentElement.xRatio = newCenter.x / framebufferWidth;
                currentElement.yRatio = newCenter.y / framebufferWidth;
            } else {
                currentElement.xRatio =
                    (newCenter.x - newSize.x * 0.5f) / framebufferWidth;
                currentElement.yRatio =
                    (newCenter.y - newSize.y * 0.5f) / framebufferWidth;
            }
            continue;
        }

        if (element.source == SelectionSource::ExistingTexture) {
            const auto currentIt =
                currentTextureInfos.find(element.existingKey);
            if (currentIt == currentTextureInfos.end()) {
                continue;
            }

            UILoadSystem::TextureInfo& currentTexture = currentIt->second;
            currentTexture.rotationDegrees = newRotationDegrees;
            currentTexture.widthRatio = newSize.x / framebufferWidth;
            currentTexture.heightRatio = newSize.y / framebufferHeight;
            currentTexture.xRatio =
                (newCenter.x - newSize.x * 0.5f) / framebufferWidth;
            currentTexture.yRatio =
                (newCenter.y - newSize.y * 0.5f) / framebufferHeight;
            continue;
        }

        if (element.source == SelectionSource::ExistingText) {
            const auto currentIt = currentTextInfos.find(element.existingKey);
            const auto originalIt =
                mTransformStartState.textInfos.find(element.existingKey);
            if (currentIt == currentTextInfos.end() ||
                originalIt == mTransformStartState.textInfos.end()) {
                continue;
            }

            UILoadSystem::TextInfo& currentText = currentIt->second;
            const UILoadSystem::TextInfo& originalText = originalIt->second;
            currentText.rotationDegrees = newRotationDegrees;
            const float scaleX = newSize.x / std::max(originalTransform.size.x, 1.0f);
            const float scaleY = newSize.y / std::max(originalTransform.size.y, 1.0f);
            const float uniformScale =
                std::sqrt(std::max(scaleX * scaleY, 0.0001f));
            currentText.scaleRatio =
                std::clamp(
                    originalText.scaleRatio * uniformScale,
                    0.00005f,
                    0.005f);

            if (currentText.centerBased) {
                currentText.xRatio = newCenter.x / framebufferWidth;
                currentText.yRatio = newCenter.y / framebufferHeight;
            } else {
                currentText.xRatio =
                    (newCenter.x - newSize.x * 0.5f) / framebufferWidth;
                currentText.yRatio =
                    (newCenter.y - newSize.y * 0.5f) / framebufferHeight;
            }
        }
    }
}

void UICanvasEditorController::PushUndo(const UILoadSystem* uiLoadSystem)
{
    if (!uiLoadSystem) {
        return;
    }

    UndoState undoState;
    undoState.customElements = uiLoadSystem->GetCustomElements();
    undoState.textureInfos = uiLoadSystem->GetEditableTextureInfos();
    undoState.textInfos = uiLoadSystem->GetEditableTextInfos();
    mUndoStack.push_back(std::move(undoState));
    constexpr std::size_t MaxUndoCount = 20;
    if (mUndoStack.size() > MaxUndoCount) {
        mUndoStack.erase(mUndoStack.begin());
    }
}

void UICanvasEditorController::SelectElementsInBox(
    const UILoadSystem* uiLoadSystem,
    const ImVec2& rectMin,
    const ImVec2& rectMax,
    bool additive)
{
    if (!uiLoadSystem) {
        return;
    }

    if (!additive) {
        ClearSelection();
    }

    const auto& customElements = uiLoadSystem->GetCustomElements();
    for (const UIRenderer::RenderedUIElement& renderedElement :
         mContext.uiRenderer->GetRenderedUIElements()) {
        ElementReference element;
        switch (renderedElement.source) {
        case UIRenderer::RenderedUIElementSource::Custom:
        {
            const auto customIt = std::find_if(
                customElements.begin(),
                customElements.end(),
                [&](const UILoadSystem::CustomElement& candidate) {
                    return candidate.screen == renderedElement.screen &&
                           candidate.id == renderedElement.id;
                });
            if (customIt == customElements.end()) {
                continue;
            }
            element.source = SelectionSource::Custom;
            element.customIndex = static_cast<std::size_t>(
                std::distance(customElements.begin(), customIt));
            break;
        }
        case UIRenderer::RenderedUIElementSource::CodeBoundTexture:
            element.source = SelectionSource::ExistingTexture;
            element.existingKey =
                renderedElement.screen + "." + renderedElement.id;
            break;
        case UIRenderer::RenderedUIElementSource::CodeBoundText:
            element.source = SelectionSource::ExistingText;
            element.existingKey =
                renderedElement.screen + "." + renderedElement.id;
            break;
        }

        if (IsSelected(element)) {
            continue;
        }

        const ImVec2 center =
            FramebufferToImGui(renderedElement.transform.center);
        if (center.x >= rectMin.x && center.x <= rectMax.x &&
            center.y >= rectMin.y && center.y <= rectMax.y) {
            mSelectedElements.push_back(element);
            mPrimarySelection = element;
        }
    }
}
