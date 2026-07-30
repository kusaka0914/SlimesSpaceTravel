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
    mSelectedIndices.clear();
    mSelectedIndices.insert(index);
    mPrimarySelectedIndex = static_cast<int>(index);
}

void UICanvasEditorController::ToggleSelection(std::size_t index)
{
    if (mSelectedIndices.contains(index)) {
        mSelectedIndices.erase(index);
        if (mPrimarySelectedIndex == static_cast<int>(index)) {
            mPrimarySelectedIndex =
                mSelectedIndices.empty() ? -1 : static_cast<int>(*mSelectedIndices.begin());
        }
        return;
    }

    mSelectedIndices.insert(index);
    mPrimarySelectedIndex = static_cast<int>(index);
}

void UICanvasEditorController::SelectFromList(std::size_t index, bool additive)
{
    if (additive) {
        ToggleSelection(index);
    } else {
        SetSingleSelection(index);
    }
}

void UICanvasEditorController::ClearSelection()
{
    mSelectedIndices.clear();
    mPrimarySelectedIndex = -1;
    mHasLastPick = false;
}

bool UICanvasEditorController::IsSelected(std::size_t index) const
{
    return mSelectedIndices.contains(index);
}

int UICanvasEditorController::GetPrimarySelectedIndex() const
{
    return mPrimarySelectedIndex;
}

std::size_t UICanvasEditorController::GetSelectedCount() const
{
    return mSelectedIndices.size();
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
    if (!uiLoadSystem || mSelectedIndices.empty()) {
        return false;
    }

    PushUndo(uiLoadSystem);

    std::vector<std::size_t> sourceIndices(mSelectedIndices.begin(), mSelectedIndices.end());
    std::sort(sourceIndices.begin(), sourceIndices.end());

    std::unordered_set<std::size_t> duplicatedIndices;
    for (std::size_t sourceIndex : sourceIndices) {
        const std::optional<std::size_t> duplicatedIndex =
            uiLoadSystem->DuplicateCustomElement(sourceIndex);
        if (duplicatedIndex) {
            duplicatedIndices.insert(*duplicatedIndex);
        }
    }

    if (duplicatedIndices.empty()) {
        mUndoStack.pop_back();
        statusMessage = "要素の複製に失敗しました";
        return false;
    }

    mSelectedIndices = std::move(duplicatedIndices);
    mPrimarySelectedIndex = static_cast<int>(
        *std::max_element(mSelectedIndices.begin(), mSelectedIndices.end()));

    const bool saved = uiLoadSystem->SaveCustomUI();
    statusMessage =
        saved ? "選択中のUIを複製して保存しました" : "UIは複製しましたが保存に失敗しました";
    return true;
}

bool UICanvasEditorController::DeleteSelected(
    UILoadSystem* uiLoadSystem,
    std::string& statusMessage)
{
    if (!uiLoadSystem || mSelectedIndices.empty()) {
        return false;
    }

    PushUndo(uiLoadSystem);

    std::vector<std::size_t> deleteIndices(mSelectedIndices.begin(), mSelectedIndices.end());
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

    uiLoadSystem->GetCustomElements() = std::move(mUndoStack.back());
    mUndoStack.pop_back();
    uiLoadSystem->ClearCustomVisibilityOverrides();
    ClearSelection();

    const bool saved = uiLoadSystem->SaveCustomUI();
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

    const std::size_t elementCount = uiLoadSystem->GetCustomElements().size();
    std::erase_if(
        mSelectedIndices,
        [elementCount](std::size_t index) { return index >= elementCount; });

    if (mPrimarySelectedIndex < 0 ||
        static_cast<std::size_t>(mPrimarySelectedIndex) >= elementCount ||
        !mSelectedIndices.contains(static_cast<std::size_t>(mPrimarySelectedIndex))) {
        mPrimarySelectedIndex =
            mSelectedIndices.empty() ? -1 : static_cast<int>(*mSelectedIndices.begin());
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
    const bool additive = io.KeyCtrl || io.KeySuper || io.KeyShift;

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        const std::vector<std::size_t> hits =
            PickElementsAt(uiLoadSystem, ImGuiToFramebuffer(mousePosition));

        if (!hits.empty()) {
            std::size_t hitIndex = 0;
            if (!additive && mHasLastPick) {
                const float deltaX = mousePosition.x - mLastPickPosition.x;
                const float deltaY = mousePosition.y - mLastPickPosition.y;
                constexpr float CycleRadius = 6.0f;
                if (deltaX * deltaX + deltaY * deltaY <= CycleRadius * CycleRadius) {
                    const auto current =
                        std::find(hits.begin(), hits.end(), static_cast<std::size_t>(std::max(0, mPrimarySelectedIndex)));
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
    const auto& elements = uiLoadSystem->GetCustomElements();

    for (std::size_t index : mSelectedIndices) {
        if (index >= elements.size()) {
            continue;
        }

        ElementTransform transform;
        if (!GetElementTransform(elements[index], transform)) {
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
            static_cast<int>(index) == mPrimarySelectedIndex
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
    if (!uiLoadSystem || mSelectedIndices.empty() || !mContext.uiRenderer) {
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

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGuizmo::BeginFrame();
    ImGuizmo::SetOrthographic(true);
    ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());
    ImGuizmo::SetRect(
        viewport->Pos.x,
        viewport->Pos.y,
        viewport->Size.x,
        viewport->Size.y);

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
            mTransformStartElements = uiLoadSystem->GetCustomElements();
            mTransformStartSelectionMatrix = matrixBeforeManipulation;
            mIsUsingGizmo = true;
        }

        ApplySelectionMatrix(uiLoadSystem, mEditingMatrix);
        return;
    }

    if (mIsUsingGizmo) {
        mIsUsingGizmo = false;
        mTransformStartElements.clear();
        const bool saved = uiLoadSystem->SaveCustomUI();
        statusMessage =
            saved ? "UIギズモの変更を保存しました" : "UIギズモの変更を保存できませんでした";
    }
}

bool UICanvasEditorController::GetElementTransform(
    const UILoadSystem::CustomElement& element,
    ElementTransform& outTransform) const
{
    if (!mContext.uiRenderer) {
        return false;
    }

    UIRenderer::CustomElementScreenTransform screenTransform;
    if (!mContext.uiRenderer->CalculateCustomElementScreenTransform(element, screenTransform)) {
        return false;
    }

    outTransform.center = screenTransform.center;
    outTransform.size = glm::max(
        screenTransform.size,
        glm::vec2(MinimumElementSizePixels));
    outTransform.rotationDegrees = element.rotationDegrees;
    return true;
}

bool UICanvasEditorController::GetSelectionBounds(
    const UILoadSystem* uiLoadSystem,
    glm::vec2& outMin,
    glm::vec2& outMax) const
{
    if (!uiLoadSystem || mSelectedIndices.empty()) {
        return false;
    }

    const auto& elements = uiLoadSystem->GetCustomElements();
    outMin = glm::vec2(std::numeric_limits<float>::max());
    outMax = glm::vec2(std::numeric_limits<float>::lowest());
    bool hasBounds = false;

    for (std::size_t index : mSelectedIndices) {
        if (index >= elements.size()) {
            continue;
        }

        ElementTransform transform;
        if (!GetElementTransform(elements[index], transform)) {
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

std::vector<std::size_t> UICanvasEditorController::PickElementsAt(
    const UILoadSystem* uiLoadSystem,
    const glm::vec2& framebufferPoint) const
{
    std::vector<std::size_t> hits;
    if (!uiLoadSystem) {
        return hits;
    }

    const auto& elements = uiLoadSystem->GetCustomElements();
    for (std::size_t index = 0; index < elements.size(); ++index) {
        ElementTransform transform;
        if (GetElementTransform(elements[index], transform) &&
            IsPointInsideElement(transform, framebufferPoint)) {
            hits.push_back(index);
        }
    }

    std::stable_sort(
        hits.begin(),
        hits.end(),
        [&elements](std::size_t lhs, std::size_t rhs) {
            if (elements[lhs].zOrder != elements[rhs].zOrder) {
                return elements[lhs].zOrder > elements[rhs].zOrder;
            }
            return lhs > rhs;
        });
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

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float width =
        std::max(viewport->Size.x, 1.0f);
    const float height =
        std::max(viewport->Size.y, 1.0f);
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

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float framebufferWidth =
        static_cast<float>(std::max(mContext.uiRenderer->GetFbWidth(), 1));
    const float framebufferHeight =
        static_cast<float>(std::max(mContext.uiRenderer->GetFbHeight(), 1));
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
    if (!uiLoadSystem || mSelectedIndices.empty()) {
        return glm::mat4(1.0f);
    }

    if (mSelectedIndices.size() == 1) {
        const std::size_t index = *mSelectedIndices.begin();
        const auto& elements = uiLoadSystem->GetCustomElements();
        if (index < elements.size()) {
            ElementTransform transform;
            if (GetElementTransform(elements[index], transform)) {
                return CreateElementMatrix(transform);
            }
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
    if (!uiLoadSystem || !mContext.uiRenderer ||
        mTransformStartElements.empty()) {
        return;
    }

    const float framebufferWidth =
        static_cast<float>(mContext.uiRenderer->GetFbWidth());
    if (framebufferWidth <= 0.0f) {
        return;
    }

    const glm::mat4 deltaMatrix =
        selectionMatrix * glm::inverse(mTransformStartSelectionMatrix);
    auto& currentElements = uiLoadSystem->GetCustomElements();

    for (std::size_t index : mSelectedIndices) {
        if (index >= currentElements.size() ||
            index >= mTransformStartElements.size()) {
            continue;
        }

        const UILoadSystem::CustomElement& originalElement =
            mTransformStartElements[index];
        ElementTransform originalTransform;
        if (!GetElementTransform(originalElement, originalTransform)) {
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

        UILoadSystem::CustomElement& currentElement = currentElements[index];
        const glm::vec2 newCenter(translation[0], translation[1]);
        const glm::vec2 newSize(
            std::max(std::abs(scale[0]), MinimumElementSizePixels),
            std::max(std::abs(scale[1]), MinimumElementSizePixels));

        currentElement.rotationDegrees = NormalizeDegrees(rotation[2]);

        if (currentElement.type == UILoadSystem::CustomElementType::Text) {
            const float scaleX = newSize.x / std::max(originalTransform.size.x, 1.0f);
            const float scaleY = newSize.y / std::max(originalTransform.size.y, 1.0f);
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
    }
}

void UICanvasEditorController::PushUndo(const UILoadSystem* uiLoadSystem)
{
    if (!uiLoadSystem) {
        return;
    }

    mUndoStack.push_back(uiLoadSystem->GetCustomElements());
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

    const auto& elements = uiLoadSystem->GetCustomElements();
    for (std::size_t index = 0; index < elements.size(); ++index) {
        ElementTransform transform;
        if (!GetElementTransform(elements[index], transform)) {
            continue;
        }

        const ImVec2 center = FramebufferToImGui(transform.center);
        if (center.x >= rectMin.x && center.x <= rectMax.x &&
            center.y >= rectMin.y && center.y <= rectMax.y) {
            mSelectedIndices.insert(index);
            mPrimarySelectedIndex = static_cast<int>(index);
        }
    }
}
