#include "gfx/UIRenderer.h"

#include "Game.h"
#include "actor/Player.h"
#include "gfx/UIShader.h"
#include "gfx/VertexArray.h"
#include "gfx/ui/HudRenderer.h"
#include "gfx/ui/PauseMenuRenderer.h"
#include "gfx/ui/SceneUIRenderer.h"
#include "gfx/ui/StateUIRenderer.h"
#include "gfx/ui/UIDebugEditorBridge.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "system/SceneSystem.h"
#include "system/text/JapaneseRubyGenerator.h"
#include "system/CameraSystem.h"
#include "system/sequence/SequenceSystem.h"
#include <algorithm>
#include <array>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <limits>

void UIRenderer::RegisterCustomUITextures()
{
    if (!mUILoadSystem) {
        return;
    }

    for (const UILoadSystem::CustomElement& element : mUILoadSystem->GetCustomElements()) {
        if (element.type != UILoadSystem::CustomElementType::Image) {
            continue;
        }

        RegisterCustomUITexture(element.texturePath);
        RegisterCustomUITexture(element.keyboardTexturePath);
        RegisterCustomUITexture(element.gameControllerTexturePath);
    }
}

const std::string& UIRenderer::ResolveCustomElementText(
    const UILoadSystem::CustomElement& element) const
{
    const bool usesGameController =
        mGame->GetLastUsedInputDevice() ==
        InputDeviceType::GameController;
    if (mGame->IsInputModifierHeld()) {
        const std::string& modifierText =
            usesGameController
                ? element.gameControllerModifierText
                : element.keyboardModifierText;
        if (!modifierText.empty()) {
            return modifierText;
        }
    }

    if (!element.usesInputDeviceVariants) {
        return element.text;
    }

    const std::string& deviceText =
        usesGameController
            ? element.gameControllerText
            : element.keyboardText;
    return deviceText.empty() ? element.text : deviceText;
}

const std::string& UIRenderer::ResolveCustomElementTexturePath(
    const UILoadSystem::CustomElement& element) const
{
    if (!element.usesInputDeviceVariants) {
        return element.texturePath;
    }

    const bool usesGameController =
        mGame->GetLastUsedInputDevice() ==
        InputDeviceType::GameController;
    const std::string& deviceTexturePath =
        usesGameController
            ? element.gameControllerTexturePath
            : element.keyboardTexturePath;
    return deviceTexturePath.empty()
               ? element.texturePath
               : deviceTexturePath;
}

bool UIRenderer::ResolveCustomElementTextureFlipVertical(
    const UILoadSystem::CustomElement& element) const
{
    if (!element.usesInputDeviceVariants) {
        return element.flipVertical;
    }

    const bool usesGameController =
        mGame->GetLastUsedInputDevice() ==
        InputDeviceType::GameController;
    if (usesGameController &&
        !element.gameControllerTexturePath.empty()) {
        return element.gameControllerFlipVertical;
    }
    if (!usesGameController && !element.keyboardTexturePath.empty()) {
        return element.keyboardFlipVertical;
    }
    return element.flipVertical;
}

void UIRenderer::SetCustomUIElementVisible(
    const std::string& screen,
    const std::string& id,
    bool visible)
{
    if (mUILoadSystem) {
        mUILoadSystem->SetCustomElementVisible(screen, id, visible);
    }
}

void UIRenderer::SetCustomUIScreenVisible(const std::string& screen, bool visible)
{
    if (mUILoadSystem) {
        mUILoadSystem->SetCustomScreenVisible(screen, visible);
    }
}

void UIRenderer::ClearCustomUIVisibilityOverrides()
{
    if (mUILoadSystem) {
        mUILoadSystem->ClearCustomVisibilityOverrides();
    }
}

bool UIRenderer::RegisterCustomUITexture(const std::string& assetRelativePath)
{
    if (assetRelativePath.empty()) {
        return false;
    }

    std::string normalizedPath = assetRelativePath;
    std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');

    const std::string textureName = GetCustomTextureName(normalizedPath);
    if (mTextures.find(textureName) != mTextures.end()) {
        return true;
    }

    const std::filesystem::path fullPath = std::filesystem::path("../assets") / normalizedPath;
    if (!std::filesystem::is_regular_file(fullPath)) {
        return false;
    }

    RegisterTexture(fullPath.string(), textureName);
    return mTextures.find(textureName) != mTextures.end();
}

GLuint UIRenderer::GetCustomUITextureHandle(const std::string& assetRelativePath) const
{
    const std::string textureName = GetCustomTextureName(assetRelativePath);
    const auto it = mTextures.find(textureName);
    return it != mTextures.end() ? it->second : 0;
}

bool UIRenderer::CalculateCustomElementScreenTransform(
    const UILoadSystem::CustomElement& element,
    CustomElementScreenTransform& outTransform) const
{
    if (mFbWidth <= 0 || mFbHeight <= 0) {
        return false;
    }

    const float x = mFbWidth * element.xRatio;
    const float y = mFbWidth * element.yRatio;

    float width = mFbWidth * element.widthRatio;
    float height = mFbWidth * element.heightRatio;

    if (element.type == UILoadSystem::CustomElementType::Text) {
        const std::string& resolvedText = ResolveCustomElementText(element);
        std::string firstLine = resolvedText;
        std::string secondLine;
        const bool hasSecondLine = SplitText(resolvedText, firstLine, secondLine);
        const float textScale = mFbWidth * element.textScaleRatio;

        int firstWidth = 0;
        int firstHeight = 0;
        MeasureText(firstLine, textScale, firstWidth, firstHeight);

        int secondWidth = 0;
        int secondHeight = 0;
        if (hasSecondLine) {
            MeasureText(secondLine, textScale, secondWidth, secondHeight);
        }

        width = static_cast<float>(std::max(firstWidth, secondWidth));
        height = static_cast<float>(std::max(firstHeight, secondHeight));
        if (hasSecondLine) {
            height += mFbHeight * 0.0666f;
        }
    }

    width = std::max(width, 1.0f);
    height = std::max(height, 1.0f);

    outTransform.size = glm::vec2(width, height);
    if (element.centerBased) {
        outTransform.center = glm::vec2(x, y);
    } else {
        outTransform.center = glm::vec2(x + width * 0.5f, y + height * 0.5f);
    }

    return true;
}

bool UIRenderer::CalculateTextureInfoScreenTransform(
    const UILoadSystem::TextureInfo& textureInfo,
    CustomElementScreenTransform& outTransform) const
{
    if (mFbWidth <= 0 || mFbHeight <= 0) {
        return false;
    }

    outTransform.size = glm::max(
        glm::vec2(
            mFbWidth * textureInfo.widthRatio,
            mFbHeight * textureInfo.heightRatio),
        glm::vec2(1.0f));
    outTransform.center =
        glm::vec2(
            mFbWidth * textureInfo.xRatio,
            mFbHeight * textureInfo.yRatio) +
        outTransform.size * 0.5f;
    return true;
}

bool UIRenderer::CalculateTextInfoScreenTransform(
    const UILoadSystem::TextInfo& textInfo,
    CustomElementScreenTransform& outTransform) const
{
    if (mFbWidth <= 0 || mFbHeight <= 0) {
        return false;
    }

    const std::string text =
        textInfo.texts.empty() ? std::string("Text") : textInfo.texts.front();
    std::string firstLine = text;
    std::string secondLine;
    const bool hasSecondLine = SplitText(text, firstLine, secondLine);
    const float textScale = mFbWidth * textInfo.scaleRatio;

    int firstWidth = 0;
    int firstHeight = 0;
    MeasureText(firstLine, textScale, firstWidth, firstHeight);

    int secondWidth = 0;
    int secondHeight = 0;
    if (hasSecondLine) {
        MeasureText(secondLine, textScale, secondWidth, secondHeight);
    }

    outTransform.size = glm::max(
        glm::vec2(
            static_cast<float>(std::max(firstWidth, secondWidth)),
            static_cast<float>(std::max(firstHeight, secondHeight)) +
                (hasSecondLine ? mFbHeight * 0.0666f : 0.0f)),
        glm::vec2(1.0f));

    const glm::vec2 position(
        mFbWidth * textInfo.xRatio,
        mFbHeight * textInfo.yRatio);
    outTransform.center =
        textInfo.centerBased
            ? position
            : position + outTransform.size * 0.5f;
    return true;
}

void UIRenderer::RecordRenderedUIElement(
    RenderedUIElementSource source,
    const std::string& screen,
    const std::string& id,
    const glm::vec2& center,
    const glm::vec2& size,
    float rotationDegrees)
{
    RenderedUIElement renderedElement;
    renderedElement.source = source;
    renderedElement.screen = screen;
    renderedElement.id = id;
    renderedElement.transform.center = center;
    renderedElement.transform.size = glm::max(size, glm::vec2(1.0f));
    renderedElement.rotationDegrees = rotationDegrees;
    mRenderedUIElements.push_back(std::move(renderedElement));
}

void UIRenderer::RecordCustomUIElementForEditor(
    const UILoadSystem::CustomElement& element)
{
    CustomElementScreenTransform screenTransform;
    if (!CalculateCustomElementScreenTransform(
            element,
            screenTransform)) {
        return;
    }

    RecordRenderedUIElement(
        RenderedUIElementSource::Custom,
        element.screen,
        element.id,
        screenTransform.center,
        screenTransform.size,
        element.rotationDegrees);
}

void UIRenderer::RecordRenderedTextElement(
    const std::string& screen,
    const std::string& id,
    float x,
    float y,
    float scale,
    const std::string& message,
    bool centerBased,
    float rotationDegrees)
{
    std::string firstLine = message;
    std::string secondLine;
    const bool hasSecondLine = SplitText(message, firstLine, secondLine);

    int firstWidth = 0;
    int firstHeight = 0;
    MeasureText(firstLine, scale, firstWidth, firstHeight);

    int secondWidth = 0;
    int secondHeight = 0;
    if (hasSecondLine) {
        MeasureText(secondLine, scale, secondWidth, secondHeight);
    }

    const glm::vec2 size(
        static_cast<float>(std::max(firstWidth, secondWidth)),
        static_cast<float>(std::max(firstHeight, secondHeight)) +
            (hasSecondLine ? mFbHeight * 0.0666f : 0.0f));
    const glm::vec2 position(x, y);
    const glm::vec2 center =
        centerBased ? position : position + size * 0.5f;
    RecordRenderedUIElement(
        RenderedUIElementSource::CodeBoundText,
        screen,
        id,
        center,
        size,
        rotationDegrees);
}

void UIRenderer::DrawTextForElement(
    const std::string& screen,
    const std::string& id,
    float x,
    float y,
    float scale,
    const std::string& message,
    bool centerBased,
    glm::vec4 color,
    float rotationDegrees)
{
    const std::vector<RubyTextSegment>& rubySegments =
        ResolveCustomElementRuby(message);
    const bool hasRuby = std::any_of(
        rubySegments.begin(),
        rubySegments.end(),
        [](const RubyTextSegment& segment) {
            return segment.showsRuby && !segment.reading.empty();
        });
    if (hasRuby) {
        DrawRubyText(
            x,
            y,
            scale,
            0.42f,
            -0.12f,
            rubySegments,
            color,
            centerBased,
            rotationDegrees);
    } else {
        DrawText(
            x,
            y,
            scale,
            message,
            centerBased,
            color,
            rotationDegrees);
    }
    RecordRenderedTextElement(
        screen,
        id,
        x,
        y,
        scale,
        message,
        centerBased,
        rotationDegrees);
}

std::string UIRenderer::GetCustomTextureName(const std::string& assetRelativePath)
{
    std::string normalizedPath = assetRelativePath;
    std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');
    return "custom-ui:" + normalizedPath;
}

const std::vector<RubyTextSegment>& UIRenderer::ResolveCustomElementRuby(
    const std::string& text)
{
    const auto cached = mCustomTextRubyCache.find(text);
    if (cached != mCustomTextRubyCache.end()) {
        return cached->second;
    }

    std::vector<RubyTextSegment> segments;
    std::string errorMessage;
    if (!JapaneseRubyGenerator::Generate(text, segments, errorMessage) ||
        JoinRubyBaseText(segments) != text) {
        segments.clear();
    }
    return mCustomTextRubyCache.emplace(text, std::move(segments))
        .first->second;
}

