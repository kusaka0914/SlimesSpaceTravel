#include "editor/ui/UICanvasTransformController.h"

#include "editor/ui/UICanvasElementGeometry.h"
#include "editor/ui/UICanvasSelectionController.h"
#include "imgui.h"

// ImGuizmoはImGuiの型定義を前提とする。
#include "ImGuizmo.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace {
using ElementReference = UICanvasElementReference;
using ElementTransform = UICanvasElementTransform;
using SelectionSource = UICanvasSelectionSource;
using Operation = UICanvasOperation;

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
} // namespace

UICanvasTransformController::UICanvasTransformController(DebugEditorContext& context,
                                                         const UICanvasElementGeometry& geometry,
                                                         const UICanvasSelectionController& selection,
                                                         UICanvasEditHistory& editHistory)
    : mContext(context),
      mGeometry(geometry),
      mSelection(selection),
      mEditHistory(editHistory)
{
}

void UICanvasTransformController::SetOperation(Operation operation)
{
    if (mIsUsingGizmo) {
        return;
    }
    mOperation = operation;
}

UICanvasOperation UICanvasTransformController::GetOperation() const
{
    return mOperation;
}

void UICanvasTransformController::DrawGizmo(UILoadSystem* uiLoadSystem, std::string& statusMessage)
{
    if (!uiLoadSystem || mSelection.GetElements().empty() || !mContext.uiRenderer) {
        mIsUsingGizmo = false;
        return;
    }

    const int framebufferWidth = mContext.uiRenderer->GetFbWidth();
    const int framebufferHeight = mContext.uiRenderer->GetFbHeight();
    if (framebufferWidth <= 0 || framebufferHeight <= 0) {
        return;
    }

    if (!mIsUsingGizmo) {
        mEditingMatrix = mGeometry.CreateSelectionMatrix(uiLoadSystem, mSelection.GetElements());
    }

    const glm::mat4 matrixBeforeManipulation = mEditingMatrix;
    const float halfWidth = static_cast<float>(framebufferWidth) * 0.5f;
    const float halfHeight = static_cast<float>(framebufferHeight) * 0.5f;
    const float cameraDistance = std::max(static_cast<float>(framebufferWidth), static_cast<float>(framebufferHeight));
    const glm::vec3 cameraTarget(halfWidth, halfHeight, 0.0f);
    const glm::mat4 view =
        glm::lookAt(cameraTarget + glm::vec3(0.0f, 0.0f, cameraDistance), cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 projection =
        glm::ortho(-halfWidth, halfWidth, halfHeight, -halfHeight, 0.1f, cameraDistance * 2.0f);

    ImGuizmo::BeginFrame();
    ImGuizmo::SetOrthographic(true);
    ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());
    if (mContext.gameViewport.IsValid()) {
        ImGuizmo::SetRect(mContext.gameViewport.x, mContext.gameViewport.y, mContext.gameViewport.width,
                          mContext.gameViewport.height);
    } else {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGuizmo::SetRect(viewport->Pos.x, viewport->Pos.y, viewport->Size.x, viewport->Size.y);
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
    ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), operation, mode,
                         glm::value_ptr(mEditingMatrix));
    const bool isUsingGizmo = ImGuizmo::IsUsing();
    ImGuizmo::PopID();

    if (isUsingGizmo) {

        // ドラッグ全体を1回のUndoにし、変換の基準も操作開始時に固定する。
        if (!mIsUsingGizmo) {
            mEditHistory.Capture(*uiLoadSystem);
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
        mTransformStartState = UICanvasEditSnapshot{};
        std::string error;
        const bool wasSaved = uiLoadSystem->SaveEditorUI(error);
        statusMessage = wasSaved ? "UIギズモの変更を保存しました" : "UIギズモの変更を保存できませんでした: " + error;
    }
}

bool UICanvasTransformController::ResolveTransformStartElement(const ElementReference& element,
                                                               ElementTransform& outTransform) const
{
    UIRenderer::CustomElementScreenTransform screenTransform;
    switch (element.source) {
    case SelectionSource::Custom:
        if (element.customIndex >= mTransformStartState.customElements.size()) {
            return false;
        }
        if (!mContext.uiRenderer->CalculateCustomElementScreenTransform(
                mTransformStartState.customElements[element.customIndex], screenTransform)) {
            return false;
        }
        outTransform.rotationDegrees = mTransformStartState.customElements[element.customIndex].rotationDegrees;
        break;
    case SelectionSource::ExistingTexture: {
        const auto originalIterator = mTransformStartState.textureInfos.find(element.existingKey);
        if (originalIterator == mTransformStartState.textureInfos.end() ||
            !mContext.uiRenderer->CalculateTextureInfoScreenTransform(originalIterator->second, screenTransform)) {
            return false;
        }
        outTransform.rotationDegrees = originalIterator->second.rotationDegrees;
        break;
    }
    case SelectionSource::ExistingText: {
        const auto originalIterator = mTransformStartState.textInfos.find(element.existingKey);
        if (originalIterator == mTransformStartState.textInfos.end() ||
            !mContext.uiRenderer->CalculateTextInfoScreenTransform(originalIterator->second, screenTransform)) {
            return false;
        }
        outTransform.rotationDegrees = originalIterator->second.rotationDegrees;
        break;
    }
    case SelectionSource::None:
        return false;
    }

    outTransform.center = screenTransform.center;
    outTransform.size = glm::max(screenTransform.size, glm::vec2(UICanvasElementGeometry::minimumElementSizePixels));
    return true;
}

void UICanvasTransformController::ApplySelectionMatrix(UILoadSystem* uiLoadSystem, const glm::mat4& selectionMatrix)
{
    if (!uiLoadSystem || !mContext.uiRenderer) {
        return;
    }

    const float framebufferWidth = static_cast<float>(mContext.uiRenderer->GetFbWidth());
    const float framebufferHeight = static_cast<float>(mContext.uiRenderer->GetFbHeight());
    if (framebufferWidth <= 0.0f || framebufferHeight <= 0.0f) {
        return;
    }

    // 毎フレームの変換を累積せず、開始時点からの差分を元の各要素へ適用する。
    const glm::mat4 deltaMatrix = selectionMatrix * glm::inverse(mTransformStartSelectionMatrix);
    auto& currentElements = uiLoadSystem->GetCustomElements();
    auto& currentTextureInfos = uiLoadSystem->GetEditableTextureInfos();
    auto& currentTextInfos = uiLoadSystem->GetEditableTextInfos();

    for (const ElementReference& element : mSelection.GetElements()) {
        ElementTransform originalTransform;
        if (!ResolveTransformStartElement(element, originalTransform)) {
            continue;
        }

        const glm::mat4 transformedMatrix =
            deltaMatrix * UICanvasElementGeometry::CreateElementMatrix(originalTransform);

        float translation[3];
        float rotation[3];
        float scale[3];
        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(transformedMatrix), translation, rotation, scale);

        const glm::vec2 newCenter(translation[0], translation[1]);
        const glm::vec2 newSize(std::max(std::abs(scale[0]), UICanvasElementGeometry::minimumElementSizePixels),
                                std::max(std::abs(scale[1]), UICanvasElementGeometry::minimumElementSizePixels));

        const float newRotationDegrees = NormalizeDegrees(rotation[2]);

        // CustomElementはY位置と高さも画面幅基準。既存UIの縦方向は画面高さを基準に戻す。
        if (element.source == SelectionSource::Custom) {
            if (element.customIndex >= currentElements.size() ||
                element.customIndex >= mTransformStartState.customElements.size()) {
                continue;
            }

            UILoadSystem::CustomElement& currentElement = currentElements[element.customIndex];
            const UILoadSystem::CustomElement& originalElement =
                mTransformStartState.customElements[element.customIndex];
            currentElement.rotationDegrees = newRotationDegrees;

            if (currentElement.type == UILoadSystem::CustomElementType::Text) {
                const float scaleX = newSize.x / std::max(originalTransform.size.x, 1.0f);
                const float scaleY = newSize.y / std::max(originalTransform.size.y, 1.0f);

                // 文字倍率は1つの値で保持するため、縦横の倍率の幾何平均を使う。
                const float uniformScale = std::sqrt(std::max(scaleX * scaleY, 0.0001f));
                currentElement.textScaleRatio =
                    std::clamp(originalElement.textScaleRatio * uniformScale, 0.00005f, 0.003f);
            } else {
                currentElement.widthRatio = newSize.x / framebufferWidth;
                currentElement.heightRatio = newSize.y / framebufferWidth;
            }

            if (currentElement.centerBased) {
                currentElement.xRatio = newCenter.x / framebufferWidth;
                currentElement.yRatio = newCenter.y / framebufferWidth;
            } else {
                currentElement.xRatio = (newCenter.x - newSize.x * 0.5f) / framebufferWidth;
                currentElement.yRatio = (newCenter.y - newSize.y * 0.5f) / framebufferWidth;
            }
            continue;
        }

        if (element.source == SelectionSource::ExistingTexture) {
            const auto currentIterator = currentTextureInfos.find(element.existingKey);
            if (currentIterator == currentTextureInfos.end()) {
                continue;
            }

            UILoadSystem::TextureInfo& currentTexture = currentIterator->second;
            currentTexture.rotationDegrees = newRotationDegrees;
            currentTexture.widthRatio = newSize.x / framebufferWidth;
            currentTexture.heightRatio = newSize.y / framebufferHeight;
            currentTexture.xRatio = (newCenter.x - newSize.x * 0.5f) / framebufferWidth;
            currentTexture.yRatio = (newCenter.y - newSize.y * 0.5f) / framebufferHeight;
            continue;
        }

        if (element.source == SelectionSource::ExistingText) {
            const auto currentIterator = currentTextInfos.find(element.existingKey);
            const auto originalIterator = mTransformStartState.textInfos.find(element.existingKey);
            if (currentIterator == currentTextInfos.end() || originalIterator == mTransformStartState.textInfos.end()) {
                continue;
            }

            UILoadSystem::TextInfo& currentText = currentIterator->second;
            const UILoadSystem::TextInfo& originalText = originalIterator->second;
            currentText.rotationDegrees = newRotationDegrees;
            const float scaleX = newSize.x / std::max(originalTransform.size.x, 1.0f);
            const float scaleY = newSize.y / std::max(originalTransform.size.y, 1.0f);
            const float uniformScale = std::sqrt(std::max(scaleX * scaleY, 0.0001f));
            currentText.scaleRatio = std::clamp(originalText.scaleRatio * uniformScale, 0.00005f, 0.005f);

            if (currentText.centerBased) {
                currentText.xRatio = newCenter.x / framebufferWidth;
                currentText.yRatio = newCenter.y / framebufferHeight;
            } else {
                currentText.xRatio = (newCenter.x - newSize.x * 0.5f) / framebufferWidth;
                currentText.yRatio = (newCenter.y - newSize.y * 0.5f) / framebufferHeight;
            }
        }
    }
}

bool UICanvasTransformController::IsUsingGizmo() const
{
    return mIsUsingGizmo;
}
