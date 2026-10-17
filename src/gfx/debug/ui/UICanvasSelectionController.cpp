#include "gfx/debug/ui/UICanvasSelectionController.h"

// GLFWがOpenGLヘッダを読み込む前にGLEWを読み込む。
#include <GL/glew.h>

#include "Game.h"
#include "ImGuizmo.h"
#include "gfx/debug/ui/UICanvasElementGeometry.h"

#include <algorithm>
#include <iterator>
#include <utility>

namespace {
using ElementReference = UICanvasElementReference;
using ElementTransform = UICanvasElementTransform;
using SelectionSource = UICanvasSelectionSource;
} // namespace

UICanvasSelectionController::UICanvasSelectionController(DebugEditorContext& context,
                                                         const UICanvasElementGeometry& geometry)
    : mContext(context),
      mGeometry(geometry)
{
}

void UICanvasSelectionController::ClearSelection()
{
    mSelectedElements.clear();
    mPrimarySelection.reset();
    mHasLastPick = false;
}

void UICanvasSelectionController::SetSingleSelection(const ElementReference& element)
{
    mSelectedElements.assign(1, element);
    mPrimarySelection = element;
}

void UICanvasSelectionController::ToggleSelection(const ElementReference& element)
{
    const auto selectedIterator = std::find(mSelectedElements.begin(), mSelectedElements.end(), element);
    if (selectedIterator != mSelectedElements.end()) {
        const bool removedPrimary = mPrimarySelection && *mPrimarySelection == element;
        mSelectedElements.erase(selectedIterator);
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

bool UICanvasSelectionController::IsSelected(const ElementReference& element) const
{
    return std::find(mSelectedElements.begin(), mSelectedElements.end(), element) != mSelectedElements.end();
}

void UICanvasSelectionController::ValidateSelection(const UILoadSystem* uiLoadSystem)
{
    if (!uiLoadSystem) {
        ClearSelection();
        return;
    }

    const std::size_t customElementCount = uiLoadSystem->GetCustomElements().size();
    const auto& textureInfos = uiLoadSystem->GetEditableTextureInfos();
    const auto& textInfos = uiLoadSystem->GetEditableTextInfos();
    std::erase_if(mSelectedElements, [&](const ElementReference& element) {
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

void UICanvasSelectionController::UpdateCanvasSelection(const UILoadSystem* uiLoadSystem, bool isUsingGizmo)
{
    if (!mContext.game || !mContext.game->GetWindow()) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse || isUsingGizmo || ImGuizmo::IsOver() || ImGuizmo::IsUsing()) {
        return;
    }

    const ImVec2 mousePosition = ImGui::GetMousePos();
    if (mContext.gameViewport.IsValid() && !mIsBoxMouseDown) {
        const DebugEditorGameViewport& gameViewport = mContext.gameViewport;
        const bool isInsideGameViewport =
            mousePosition.x >= gameViewport.x && mousePosition.x <= gameViewport.x + gameViewport.width &&
            mousePosition.y >= gameViewport.y && mousePosition.y <= gameViewport.y + gameViewport.height;
        if (!isInsideGameViewport) {
            return;
        }
    }
    const bool isAdditive = io.KeyCtrl || io.KeySuper || io.KeyShift;

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        const std::vector<ElementReference> hits =
            PickElementsAt(uiLoadSystem, mGeometry.ImGuiToFramebuffer(mousePosition));

        if (!hits.empty()) {
            std::size_t hitIndex = 0;

            // 同じ位置を繰り返しクリックした場合は、重なった要素を順番に選べるようにする。
            if (!isAdditive && mHasLastPick) {
                const float deltaX = mousePosition.x - mLastPickPosition.x;
                const float deltaY = mousePosition.y - mLastPickPosition.y;
                constexpr float pickCycleRadius = 6.0f;
                if (deltaX * deltaX + deltaY * deltaY <= pickCycleRadius * pickCycleRadius) {
                    const auto current =
                        mPrimarySelection ? std::find(hits.begin(), hits.end(), *mPrimarySelection) : hits.end();
                    if (current != hits.end()) {
                        hitIndex = (static_cast<std::size_t>(std::distance(hits.begin(), current)) + 1) % hits.size();
                    }
                }
            }

            mLastPickPosition = mousePosition;
            mHasLastPick = true;

            if (isAdditive) {
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
        if (!isAdditive) {
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
            const ImVec2 rectMin(std::min(mBoxStart.x, mBoxEnd.x), std::min(mBoxStart.y, mBoxEnd.y));
            const ImVec2 rectMax(std::max(mBoxStart.x, mBoxEnd.x), std::max(mBoxStart.y, mBoxEnd.y));
            SelectElementsInBox(uiLoadSystem, rectMin, rectMax, isAdditive);
        }
        mIsBoxMouseDown = false;
        mIsBoxSelecting = false;
    }
}

void UICanvasSelectionController::DrawSelectionOverlay(const UILoadSystem* uiLoadSystem) const
{
    if (!uiLoadSystem) {
        return;
    }

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    for (const ElementReference& element : mSelectedElements) {
        ElementTransform transform;
        if (!mGeometry.ResolveElementTransform(uiLoadSystem, element, transform)) {
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
                mGeometry.FramebufferToImGui(transform.center + UICanvasElementGeometry::RotatePoint(
                                                                    localCorners[corner], transform.rotationDegrees));
        }

        const ImU32 color = mPrimarySelection && *mPrimarySelection == element ? IM_COL32(255, 190, 40, 255)
                                                                               : IM_COL32(80, 190, 255, 235);
        for (int corner = 0; corner < 4; ++corner) {
            drawList->AddLine(points[corner], points[(corner + 1) % 4], color, 2.0f);
        }
    }

    if (mIsBoxSelecting) {
        const ImVec2 rectMin(std::min(mBoxStart.x, mBoxEnd.x), std::min(mBoxStart.y, mBoxEnd.y));
        const ImVec2 rectMax(std::max(mBoxStart.x, mBoxEnd.x), std::max(mBoxStart.y, mBoxEnd.y));
        drawList->AddRectFilled(rectMin, rectMax, IM_COL32(80, 190, 255, 35));
        drawList->AddRect(rectMin, rectMax, IM_COL32(80, 190, 255, 230), 0.0f, 0, 2.0f);
    }
}

std::optional<UICanvasElementReference>
UICanvasSelectionController::ResolveElementReference(const UILoadSystem& uiLoadSystem,
                                                     const UIRenderer::RenderedUIElement& renderedElement)
{
    switch (renderedElement.source) {
    case UIRenderer::RenderedUIElementSource::Custom: {
        const auto& customElements = uiLoadSystem.GetCustomElements();
        const auto customIterator = std::find_if(
            customElements.begin(), customElements.end(), [&](const UILoadSystem::CustomElement& candidate) {
                return candidate.screen == renderedElement.screen && candidate.id == renderedElement.id;
            });
        if (customIterator == customElements.end()) {
            return std::nullopt;
        }
        return ElementReference{SelectionSource::Custom,
                                static_cast<std::size_t>(std::distance(customElements.begin(), customIterator)),
                                {}};
    }
    case UIRenderer::RenderedUIElementSource::CodeBoundTexture:
        return ElementReference{SelectionSource::ExistingTexture, 0, renderedElement.screen + "." + renderedElement.id};
    case UIRenderer::RenderedUIElementSource::CodeBoundText:
        return ElementReference{SelectionSource::ExistingText, 0, renderedElement.screen + "." + renderedElement.id};
    }
    return std::nullopt;
}

std::vector<UICanvasElementReference>
UICanvasSelectionController::PickElementsAt(const UILoadSystem* uiLoadSystem, const glm::vec2& framebufferPoint) const
{
    std::vector<ElementReference> hits;
    if (!uiLoadSystem) {
        return hits;
    }

    const auto& renderedElements = mContext.uiRenderer->GetRenderedUIElements();

    // 描画順を逆にたどり、手前に描かれた要素をクリックの第1候補にする。
    for (auto renderedIterator = renderedElements.rbegin(); renderedIterator != renderedElements.rend();
         ++renderedIterator) {
        const UIRenderer::RenderedUIElement& renderedElement = *renderedIterator;

        std::optional<ElementReference> element = ResolveElementReference(*uiLoadSystem, renderedElement);
        if (!element) {
            continue;
        }

        ElementTransform transform;
        transform.center = renderedElement.transform.center;
        transform.size =
            glm::max(renderedElement.transform.size, glm::vec2(UICanvasElementGeometry::minimumElementSizePixels));
        transform.rotationDegrees = renderedElement.rotationDegrees;
        if (!UICanvasElementGeometry::IsPointInsideElement(transform, framebufferPoint) ||
            std::find(hits.begin(), hits.end(), *element) != hits.end()) {
            continue;
        }
        hits.push_back(std::move(*element));
    }
    return hits;
}

void UICanvasSelectionController::SelectElementsInBox(const UILoadSystem* uiLoadSystem, const ImVec2& rectMin,
                                                      const ImVec2& rectMax, bool isAdditive)
{
    if (!uiLoadSystem) {
        return;
    }

    if (!isAdditive) {
        ClearSelection();
    }

    for (const UIRenderer::RenderedUIElement& renderedElement : mContext.uiRenderer->GetRenderedUIElements()) {
        const std::optional<ElementReference> element = ResolveElementReference(*uiLoadSystem, renderedElement);
        if (!element) {
            continue;
        }

        if (IsSelected(*element)) {
            continue;
        }

        // 範囲選択は、描画矩形の中心が選択枠に入る要素を対象にする。
        const ImVec2 center = mGeometry.FramebufferToImGui(renderedElement.transform.center);
        if (center.x >= rectMin.x && center.x <= rectMax.x && center.y >= rectMin.y && center.y <= rectMax.y) {
            mSelectedElements.push_back(*element);
            mPrimarySelection = *element;
        }
    }
}

void UICanvasSelectionController::SetElements(std::vector<UICanvasElementReference> elements)
{
    mSelectedElements = std::move(elements);
    mPrimarySelection = mSelectedElements.empty() ? std::nullopt : std::optional(mSelectedElements.back());
}

const std::vector<UICanvasElementReference>& UICanvasSelectionController::GetElements() const
{
    return mSelectedElements;
}

const std::optional<UICanvasElementReference>& UICanvasSelectionController::GetPrimarySelection() const
{
    return mPrimarySelection;
}
