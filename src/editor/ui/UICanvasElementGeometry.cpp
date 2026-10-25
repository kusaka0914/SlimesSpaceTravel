#include "editor/ui/UICanvasElementGeometry.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>

namespace {
using ElementReference = UICanvasElementReference;
using ElementTransform = UICanvasElementTransform;
using SelectionSource = UICanvasSelectionSource;
} // namespace

UICanvasElementGeometry::UICanvasElementGeometry(DebugEditorContext& context)
    : mContext(context)
{
}

glm::vec2 UICanvasElementGeometry::RotatePoint(const glm::vec2& point, float degrees)
{
    const float radians = glm::radians(degrees);
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    return glm::vec2(point.x * cosine - point.y * sine, point.x * sine + point.y * cosine);
}

bool UICanvasElementGeometry::ResolveElementTransform(const UILoadSystem* uiLoadSystem, const ElementReference& element,
                                                      ElementTransform& outTransform) const
{
    if (!uiLoadSystem || !mContext.uiRenderer) {
        return false;
    }

    UIRenderer::CustomElementScreenTransform screenTransform;
    switch (element.source) {
    case SelectionSource::Custom: {
        const auto& customElements = uiLoadSystem->GetCustomElements();
        if (element.customIndex >= customElements.size()) {
            return false;
        }
        const UILoadSystem::CustomElement& customElement = customElements[element.customIndex];
        if (!mContext.uiRenderer->CalculateCustomElementScreenTransform(customElement, screenTransform)) {
            return false;
        }
        outTransform.rotationDegrees = customElement.rotationDegrees;
        break;
    }
    case SelectionSource::ExistingTexture: {
        const auto& textureInfos = uiLoadSystem->GetEditableTextureInfos();
        const auto textureInfoIterator = textureInfos.find(element.existingKey);
        if (textureInfoIterator == textureInfos.end() ||
            !mContext.uiRenderer->CalculateTextureInfoScreenTransform(textureInfoIterator->second, screenTransform)) {
            return false;
        }
        outTransform.rotationDegrees = textureInfoIterator->second.rotationDegrees;
        break;
    }
    case SelectionSource::ExistingText: {
        const auto& textInfos = uiLoadSystem->GetEditableTextInfos();
        const auto textInfoIterator = textInfos.find(element.existingKey);
        if (textInfoIterator == textInfos.end() ||
            !mContext.uiRenderer->CalculateTextInfoScreenTransform(textInfoIterator->second, screenTransform)) {
            return false;
        }
        outTransform.rotationDegrees = textInfoIterator->second.rotationDegrees;
        break;
    }
    case SelectionSource::None:
        return false;
    }

    outTransform.center = screenTransform.center;
    outTransform.size = glm::max(screenTransform.size, glm::vec2(minimumElementSizePixels));
    return true;
}

bool UICanvasElementGeometry::GetSelectionBounds(const UILoadSystem* uiLoadSystem,
                                                 const std::vector<UICanvasElementReference>& elements,
                                                 glm::vec2& outMin, glm::vec2& outMax) const
{
    if (!uiLoadSystem || elements.empty()) {
        return false;
    }

    outMin = glm::vec2(std::numeric_limits<float>::max());
    outMax = glm::vec2(std::numeric_limits<float>::lowest());
    bool hasBounds = false;

    for (const ElementReference& element : elements) {
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
            const glm::vec2 corner = transform.center + RotatePoint(localCorner, transform.rotationDegrees);
            outMin = glm::min(outMin, corner);
            outMax = glm::max(outMax, corner);
        }
        hasBounds = true;
    }

    return hasBounds;
}

bool UICanvasElementGeometry::IsPointInsideElement(const ElementTransform& transform, const glm::vec2& framebufferPoint)
{
    // クリック位置を要素のローカル座標へ戻し、回転前の矩形で判定する。
    const glm::vec2 local = RotatePoint(framebufferPoint - transform.center, -transform.rotationDegrees);
    const glm::vec2 halfSize = transform.size * 0.5f;
    return std::abs(local.x) <= halfSize.x && std::abs(local.y) <= halfSize.y;
}

glm::vec2 UICanvasElementGeometry::ImGuiToFramebuffer(const ImVec2& point) const
{
    if (!mContext.uiRenderer) {
        return glm::vec2(0.0f);
    }

    if (mContext.gameViewport.IsValid()) {
        const DebugEditorGameViewport& gameViewport = mContext.gameViewport;
        return glm::vec2((point.x - gameViewport.x) / gameViewport.width * static_cast<float>(gameViewport.sourceWidth),
                         (point.y - gameViewport.y) / gameViewport.height *
                             static_cast<float>(gameViewport.sourceHeight));
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float width = std::max(viewport->Size.x, 1.0f);
    const float height = std::max(viewport->Size.y, 1.0f);
    return glm::vec2((point.x - viewport->Pos.x) / width * static_cast<float>(mContext.uiRenderer->GetFbWidth()),
                     (point.y - viewport->Pos.y) / height * static_cast<float>(mContext.uiRenderer->GetFbHeight()));
}

ImVec2 UICanvasElementGeometry::FramebufferToImGui(const glm::vec2& point) const
{
    if (!mContext.uiRenderer) {
        return ImVec2(0.0f, 0.0f);
    }

    const float framebufferWidth = static_cast<float>(std::max(mContext.uiRenderer->GetFbWidth(), 1));
    const float framebufferHeight = static_cast<float>(std::max(mContext.uiRenderer->GetFbHeight(), 1));

    if (mContext.gameViewport.IsValid()) {
        return ImVec2(mContext.gameViewport.x + point.x / framebufferWidth * mContext.gameViewport.width,
                      mContext.gameViewport.y + point.y / framebufferHeight * mContext.gameViewport.height);
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    return ImVec2(viewport->Pos.x + point.x / framebufferWidth * viewport->Size.x,
                  viewport->Pos.y + point.y / framebufferHeight * viewport->Size.y);
}

glm::mat4 UICanvasElementGeometry::CreateElementMatrix(const ElementTransform& transform)
{
    return glm::translate(glm::mat4(1.0f), glm::vec3(transform.center, 0.0f)) *
           glm::rotate(glm::mat4(1.0f), glm::radians(transform.rotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f)) *
           glm::scale(glm::mat4(1.0f), glm::vec3(transform.size, 1.0f));
}

glm::mat4 UICanvasElementGeometry::CreateSelectionMatrix(const UILoadSystem* uiLoadSystem,
                                                         const std::vector<UICanvasElementReference>& elements) const
{
    if (!uiLoadSystem || elements.empty()) {
        return glm::mat4(1.0f);
    }

    if (elements.size() == 1) {
        ElementTransform transform;
        if (ResolveElementTransform(uiLoadSystem, elements.front(), transform)) {
            return CreateElementMatrix(transform);
        }
    }

    glm::vec2 boundsMin;
    glm::vec2 boundsMax;
    if (!GetSelectionBounds(uiLoadSystem, elements, boundsMin, boundsMax)) {
        return glm::mat4(1.0f);
    }

    ElementTransform groupTransform;
    groupTransform.center = (boundsMin + boundsMax) * 0.5f;
    groupTransform.size = glm::max(boundsMax - boundsMin, glm::vec2(minimumElementSizePixels));
    return CreateElementMatrix(groupTransform);
}
