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

void UIRenderer::DrawSkyBox(int renderWidth, int renderHeight)
{
    if (renderWidth <= 0 || renderHeight <= 0) {
        glfwGetFramebufferSize(
            mGame->GetWindow(),
            &renderWidth,
            &renderHeight);
    }

    const int previousFramebufferWidth = mFbWidth;
    const int previousFramebufferHeight = mFbHeight;
    mFbWidth = renderWidth;
    mFbHeight = renderHeight;
    glUseProgram(mUIShader->GetShaderProgram());
    glUniform1i(mUIShader->GetLocConvertSrgbToLinear(), 1);

    DrawTexture(0.0f, 0.0f, mFbWidth, mFbHeight, "skyBox");
    glUniform1i(mUIShader->GetLocConvertSrgbToLinear(), 0);

    mFbWidth = previousFramebufferWidth;
    mFbHeight = previousFramebufferHeight;
}

void UIRenderer::DrawSceneText(const std::string& sceneName, const std::string& UIName, int index,
                               glm::vec4 color)
{
    const auto textInfo = mUILoadSystem->GetTextInfo(sceneName, UIName);
    if (!textInfo) {
        return;
    }

    if (index < 0 || index >= static_cast<int>(textInfo->texts.size())) {
        return;
    }

    DrawTextForElement(
        sceneName,
        UIName,
        mFbWidth * textInfo->xRatio,
        mFbHeight * textInfo->yRatio,
        mFbWidth * textInfo->scaleRatio,
        textInfo->texts[index],
        textInfo->centerBased,
        color,
        textInfo->rotationDegrees);
}

void UIRenderer::DrawTalkUI(const std::vector<std::string>& texts, int index,
                            const std::vector<RubyTextSegment>* rubySegments)
{
    DrawSceneTexture("state", "talkBgTexture", "textBg");

    const auto talkTextInfo = mUILoadSystem->GetTextInfo("state", "talkText");
    if (!talkTextInfo) {
        return;
    }

    if (index < 0 || index >= static_cast<int>(texts.size())) {
        return;
    }

    constexpr glm::vec4 talkTextColor{35.0f, 35.0f, 42.0f, 255.0f};
    const float x = mFbWidth * talkTextInfo->xRatio;
    const float y = mFbHeight * talkTextInfo->yRatio;
    const float scale = mFbWidth * talkTextInfo->scaleRatio;

    if (rubySegments && !rubySegments->empty() &&
        JoinRubyBaseText(*rubySegments) == texts[index]) {
        DrawRubyText(
            x,
            y,
            scale,
            talkTextInfo->rubyScaleRatio,
            talkTextInfo->rubyGapRatio,
            *rubySegments,
            talkTextColor,
            talkTextInfo->centerBased,
            talkTextInfo->rotationDegrees);
        RecordRenderedTextElement(
            "state",
            "talkText",
            x,
            y,
            scale,
            texts[index],
            talkTextInfo->centerBased,
            talkTextInfo->rotationDegrees);
        return;
    }

    DrawTextForElement(
        "state",
        "talkText",
        x,
        y,
        scale,
        texts[index],
        talkTextInfo->centerBased,
        talkTextColor,
        talkTextInfo->rotationDegrees);
}

void UIRenderer::DrawTalkUI(const UILoadSystem::TextInfo* textInfo)
{
    if (!textInfo) {
        return;
    }

    const auto talkBgTextureInfo = mUILoadSystem->GetTextureInfo("state", "talkBgTexture");
    if (!talkBgTextureInfo) {
        return;
    }

    constexpr float textureMarginX = 0.0275f;
    constexpr float textureMarginY = 0.0845f;
    const float backgroundX =
        mFbWidth * (textInfo->xRatio - textureMarginX);
    const float backgroundY =
        mFbHeight * (textInfo->yRatio - textureMarginY);
    const float backgroundWidth =
        mFbWidth * talkBgTextureInfo->widthRatio;
    const float backgroundHeight =
        mFbHeight * talkBgTextureInfo->heightRatio;
    DrawTexture(
        backgroundX,
        backgroundY,
        backgroundWidth,
        backgroundHeight,
        "textBg",
        false,
        talkBgTextureInfo->rotationDegrees);
    RecordRenderedUIElement(
        RenderedUIElementSource::CodeBoundTexture,
        "state",
        "talkBgTexture",
        glm::vec2(
            backgroundX + backgroundWidth * 0.5f,
            backgroundY + backgroundHeight * 0.5f),
        glm::vec2(backgroundWidth, backgroundHeight),
        talkBgTextureInfo->rotationDegrees);

    const glm::vec4 talkTextColor{35.0f, 35.0f, 42.0f, 255.0f};
    const int talkUIIndex = mGame->GetSceneSystem()->GetTalkUIIndex();

    if (talkUIIndex < 0 || talkUIIndex >= static_cast<int>(textInfo->texts.size())) {
        return;
    }

    const std::string textInfoKey =
        mUILoadSystem->FindTextInfoKey(textInfo);
    const std::size_t keySeparator = textInfoKey.find('.');
    const std::string screen =
        keySeparator == std::string::npos
            ? std::string("state")
            : textInfoKey.substr(0, keySeparator);
    const std::string id =
        keySeparator == std::string::npos
            ? std::string("talkText")
            : textInfoKey.substr(keySeparator + 1);

    if (talkUIIndex < static_cast<int>(textInfo->rubySegments.size()) &&
        !textInfo->rubySegments[talkUIIndex].empty() &&
        JoinRubyBaseText(textInfo->rubySegments[talkUIIndex]) ==
            textInfo->texts[talkUIIndex]) {
        const UILoadSystem::TextInfo* globalTalkTextInfo =
            mUILoadSystem->GetTextInfo("state", "talkText");
        const float rubyScaleRatio =
            globalTalkTextInfo ? globalTalkTextInfo->rubyScaleRatio : 0.48f;
        const float rubyGapRatio =
            globalTalkTextInfo ? globalTalkTextInfo->rubyGapRatio : 0.0f;
        DrawRubyText(
            mFbWidth * textInfo->xRatio,
            mFbHeight * textInfo->yRatio,
            mFbWidth * textInfo->scaleRatio,
            rubyScaleRatio,
            rubyGapRatio,
            textInfo->rubySegments[talkUIIndex],
            talkTextColor,
            textInfo->centerBased,
            textInfo->rotationDegrees);
        RecordRenderedTextElement(
            screen,
            id,
            mFbWidth * textInfo->xRatio,
            mFbHeight * textInfo->yRatio,
            mFbWidth * textInfo->scaleRatio,
            textInfo->texts[talkUIIndex],
            textInfo->centerBased,
            textInfo->rotationDegrees);
        return;
    }

    DrawTextForElement(
        screen,
        id,
        mFbWidth * textInfo->xRatio,
        mFbHeight * textInfo->yRatio,
        mFbWidth * textInfo->scaleRatio,
        textInfo->texts[talkUIIndex],
        textInfo->centerBased,
        talkTextColor,
        textInfo->rotationDegrees);
}

bool UIRenderer::DrawSceneTalkUI(const std::string& sceneName, const std::string& UIName)
{
    const UILoadSystem::TextInfo* textInfo = mUILoadSystem->GetTextInfo(sceneName, UIName);
    if (!textInfo) {
        return false;
    }

    const bool isTalking = mGame->GetSceneSystem()->GetTalkUIIndex() < static_cast<int>(textInfo->texts.size());
    if (isTalking) {
        DrawTalkUI(textInfo);
        return true;
    }
    return false;
}

void UIRenderer::DrawTextDependsOnGameController(const std::string& sceneName, const std::string& UIName,
                                                 float screenTopY, float uiScale)
{
    const UILoadSystem::TextInfo* textInfo;
    std::string resolvedUIName;
    if (mGame->IsGameControllerConnected()) {
        resolvedUIName = UIName + "ForGameController";
    } else {
        resolvedUIName = UIName + "ForKeyBoard";
    }
    textInfo = mUILoadSystem->GetTextInfo(sceneName, resolvedUIName);

    if (!textInfo) {
        return;
    }

    const float screenHeight = mFbHeight * uiScale;

    const float x = mFbWidth * textInfo->xRatio;
    const float y = screenTopY + screenHeight * textInfo->yRatio;
    const float scale = mFbWidth * textInfo->scaleRatio * uiScale;

    DrawTextForElement(
        sceneName,
        resolvedUIName,
        x,
        y,
        scale,
        textInfo->texts[0],
        textInfo->centerBased,
        {255, 255, 255, 255},
        textInfo->rotationDegrees);
}

void UIRenderer::DrawTextDependsOnPlayerInput(const Player* player, const std::string& sceneName,
                                              const std::string& UIName, float screenTopY,
                                              float screenHeight)
{
    const UILoadSystem::TextInfo* textInfo = nullptr;
    std::string resolvedUIName;

    if (UsesControllerUI(player)) {
        resolvedUIName = UIName + "ForGameController";
    } else {
        resolvedUIName = UIName + "ForKeyBoard";
    }
    textInfo = mUILoadSystem->GetTextInfo(sceneName, resolvedUIName);

    if (!textInfo) {
        return;
    }

    const float x = mFbWidth * textInfo->xRatio;
    const float y = screenTopY + screenHeight * textInfo->yRatio;
    const float scale = mFbWidth * textInfo->scaleRatio;

    DrawTextForElement(
        sceneName,
        resolvedUIName,
        x,
        y,
        scale,
        textInfo->texts[0],
        textInfo->centerBased,
        {255, 255, 255, 255},
        textInfo->rotationDegrees);
}

bool UIRenderer::UsesControllerUI(const Player* player) const
{
    if (!player) {
        return false;
    }

    if (!mGame->IsGameControllerConnected()) {
        return false;
    }




    return !mGame->GetIsPlayer2Joined() ||
           mGame->HasGameControllerForPlayer(player->GetPlayerNum());
}

bool UIRenderer::DrawSceneTalkUIDependsOnGameController(const std::string& sceneName, const std::string& UIName)
{
    const UILoadSystem::TextInfo* textInfo;
    if (mGame->IsGameControllerConnected()) {
        textInfo = mUILoadSystem->GetTextInfo(sceneName, UIName + "ForGameController");
    } else {
        textInfo = mUILoadSystem->GetTextInfo(sceneName, UIName + "ForKeyBoard");
    }

    if (!textInfo) {
        return false;
    }

    const bool isTalking = mGame->GetSceneSystem()->GetTalkUIIndex() < static_cast<int>(textInfo->texts.size());
    if (isTalking) {
        DrawTalkUI(textInfo);
        return true;
    }
    return false;
}

void UIRenderer::DrawBGFromUIInfo(const std::string& sceneName, const std::string& UIName, std::vector<GLfloat> color)
{
    const UILoadSystem::TextureInfo* textureInfo = mUILoadSystem->GetTextureInfo(sceneName, UIName);
    if (!textureInfo) {
        return;
    }

    const float x = mFbWidth * textureInfo->xRatio;
    const float y = mFbHeight * textureInfo->yRatio;
    const float width = mFbWidth * textureInfo->widthRatio;
    const float height = mFbHeight * textureInfo->heightRatio;
    DrawBG(x, y, width, height, color, textureInfo->rotationDegrees);
    RecordRenderedUIElement(
        RenderedUIElementSource::CodeBoundTexture,
        sceneName,
        UIName,
        glm::vec2(x + width * 0.5f, y + height * 0.5f),
        glm::vec2(width, height),
        textureInfo->rotationDegrees);
}

void UIRenderer::DrawSceneTexture(const std::string& sceneName, const std::string& UIName,
                                  const std::string& textureName)
{
    const auto textureInfo = mUILoadSystem->GetTextureInfo(sceneName, UIName);
    if (!textureInfo) {
        return;
    }

    const float x = mFbWidth * textureInfo->xRatio;
    const float y = mFbHeight * textureInfo->yRatio;
    const float width = mFbWidth * textureInfo->widthRatio;
    const float height = mFbHeight * textureInfo->heightRatio;
    DrawTexture(
        x,
        y,
        width,
        height,
        textureName,
        false,
        textureInfo->rotationDegrees);
    RecordRenderedUIElement(
        RenderedUIElementSource::CodeBoundTexture,
        sceneName,
        UIName,
        glm::vec2(x + width * 0.5f, y + height * 0.5f),
        glm::vec2(width, height),
        textureInfo->rotationDegrees);
}

void UIRenderer::DrawLinedUpTexture(const std::string& sceneName, const std::string& UIName,
                                    const std::string& textureName, float gap, int count, float screenTopY,
                                    float uiScale)
{
    DrawLinedUpTextureSlots(
        sceneName,
        UIName,
        textureName,
        gap,
        count,
        count,
        1.0f,
        screenTopY,
        uiScale);
}

void UIRenderer::DrawLinedUpTextureSlots(const std::string& sceneName, const std::string& UIName,
                                         const std::string& textureName, float gap, int activeSlotCount,
                                         int totalSlotCount, float inactiveSlotOpacity, float screenTopY,
                                         float uiScale)
{
    const auto textureInfo = mUILoadSystem->GetTextureInfo(sceneName, UIName);
    if (!textureInfo || totalSlotCount <= 0) {
        return;
    }

    const float screenHeight = mFbHeight * uiScale;

    const float textureX = mFbWidth * textureInfo->xRatio;
    const float textureY = screenTopY + screenHeight * textureInfo->yRatio;

    const float textureWidth = mFbWidth * textureInfo->widthRatio * uiScale;
    const float textureHeight = mFbWidth * textureInfo->heightRatio * uiScale;

    const float textureGap = gap * uiScale;

    const int visibleActiveSlotCount = std::clamp(activeSlotCount, 0, totalSlotCount);
    const float visibleInactiveSlotOpacity = std::clamp(inactiveSlotOpacity, 0.0f, 1.0f);

    for (int slotIndex = 0; slotIndex < totalSlotCount; ++slotIndex) {
        const float currentX = textureX + textureGap * slotIndex;
        const float opacity = slotIndex < visibleActiveSlotCount ? 1.0f : visibleInactiveSlotOpacity;
        DrawTexture(
            currentX,
            textureY,
            textureWidth,
            textureHeight,
            textureName,
            false,
            textureInfo->rotationDegrees,
            opacity);
        RecordRenderedUIElement(
            RenderedUIElementSource::CodeBoundTexture,
            sceneName,
            UIName,
            glm::vec2(
                currentX + textureWidth * 0.5f,
                textureY + textureHeight * 0.5f),
            glm::vec2(textureWidth, textureHeight),
            textureInfo->rotationDegrees);
    }
}

void UIRenderer::DrawBG(
    float x,
    float y,
    float width,
    float height,
    std::vector<GLfloat> color,
    float rotationDegrees)
{
    glUseProgram(mUIShader->GetShaderProgram());

    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x + width * 0.5f, y + height * 0.5f, 0.0f)) *
                      glm::rotate(
                          glm::mat4(1.0f),
                          glm::radians(rotationDegrees),
                          glm::vec3(0.0f, 0.0f, 1.0f)) *
                      glm::scale(glm::mat4(1.0f), glm::vec3(width, height, 1.0f));
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 proj = glm::ortho(0.0f, static_cast<float>(mFbWidth), static_cast<float>(mFbHeight), 0.0f, -1.0f, 1.0f);

    glUniformMatrix4fv(mUIShader->GetLocModel(), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(mUIShader->GetLocView(), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(mUIShader->GetLocProj(), 1, GL_FALSE, glm::value_ptr(proj));
    glUniform1i(mUIShader->GetLocUseTexture(), 0);
    glUniform4fv(mUIShader->GetLocObjectColor(), 1, color.data());

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    mVertexArrays.at("quad")->SetActive();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void UIRenderer::DrawText(
    float x,
    float y,
    float scale,
    const std::string& message,
    bool isCenterBase,
    glm::vec4 color,
    float rotationDegrees,
    const TextEffect* effect)
{
    glUseProgram(mUIShader->GetShaderProgram());

    std::string message1 = message;
    std::string message2;
    const bool isNewLine = SplitText(message, message1, message2);

    glm::vec2 rotationPivot(x, y);
    if (!isCenterBase) {
        int firstWidth = 0;
        int firstHeight = 0;
        int secondWidth = 0;
        int secondHeight = 0;
        MeasureText(message1, scale, firstWidth, firstHeight);
        if (isNewLine) {
            MeasureText(message2, scale, secondWidth, secondHeight);
        }

        const float textWidth =
            static_cast<float>(std::max(firstWidth, secondWidth));
        float textHeight =
            static_cast<float>(std::max(firstHeight, secondHeight));
        if (isNewLine) {
            textHeight += mFbHeight * 0.0666f;
        }
        rotationPivot += glm::vec2(textWidth, textHeight) * 0.5f;
    }

    const auto drawStyledLine =
        [&](const std::string& line, float lineOffset) {
            if (effect && effect->shadowEnabled && effect->shadowColor.a > 0.0f) {
                DrawTextLine(
                    line,
                    x + effect->shadowOffset.x,
                    y + effect->shadowOffset.y,
                    scale,
                    isCenterBase,
                    lineOffset,
                    effect->shadowColor,
                    rotationDegrees,
                    rotationPivot);
            }

            if (effect && effect->outlineEnabled &&
                effect->outlineWidth > 0.0f && effect->outlineColor.a > 0.0f) {
                DrawTextLine(
                    line,
                    x,
                    y,
                    scale,
                    isCenterBase,
                    lineOffset,
                    effect->outlineColor,
                    rotationDegrees,
                    rotationPivot,
                    effect->outlineWidth);
            }

            DrawTextLine(
                line,
                x,
                y,
                scale,
                isCenterBase,
                lineOffset,
                color,
                rotationDegrees,
                rotationPivot);
        };

    drawStyledLine(message1, isNewLine ? -mFbHeight * 0.0222f : 0.0f);

    if (!isNewLine) {
        return;
    }

    drawStyledLine(message2, mFbHeight * 0.0444f);
}

bool UIRenderer::SplitText(const std::string& message, std::string& message1, std::string& message2) const
{
    size_t newline = message.find('\n');
    if (newline != std::string::npos) {
        message1 = message.substr(0, newline);
        message2 = message.substr(newline + 1);
        return true;
    }

    newline = message.find("\\n");
    if (newline != std::string::npos) {
        message1 = message.substr(0, newline);
        message2 = message.substr(newline + 2);
        return true;
    }

    message1 = message;
    message2.clear();
    return false;
}

void UIRenderer::DrawTextLine(
    const std::string& message,
    float x,
    float y,
    float scale,
    bool isCenterBase,
    float yOffset,
    glm::vec4 color,
    float rotationDegrees,
    glm::vec2 rotationPivot,
    float outlineWidth)
{
    const SDL_Color textColor{static_cast<Uint8>(color.x), static_cast<Uint8>(color.y), static_cast<Uint8>(color.z),
                              static_cast<Uint8>(color.w)};

    const int outlinePixels =
        outlineWidth > 0.0f && scale > 0.0f
            ? std::max(1, static_cast<int>(std::round(outlineWidth / scale)))
            : 0;
    const float actualOutlineWidth = static_cast<float>(outlinePixels) * scale;
    const CachedTextTexture* cachedTexture =
        FindOrCreateTextTexture(message, textColor, outlinePixels);
    if (!cachedTexture) {
        return;
    }

    const int textWidth =
        static_cast<int>(static_cast<float>(cachedTexture->unscaledWidth) * scale);
    const int textHeight =
        static_cast<int>(static_cast<float>(cachedTexture->unscaledHeight) * scale);
    if (textWidth <= 0 || textHeight <= 0) {
        return;
    }

    glm::vec3 pos;
    if (isCenterBase) {
        pos = glm::vec3(x, y + yOffset, 0.0f);
    } else {
        pos = glm::vec3(
            x + textWidth * 0.5f - actualOutlineWidth,
            y + textHeight * 0.5f + yOffset - actualOutlineWidth,
            0.0f);
    }

    if (rotationPivot == glm::vec2(0.0f)) {
        rotationPivot = glm::vec2(pos.x, pos.y);
    }

    const glm::vec3 pivot(rotationPivot.x, rotationPivot.y, 0.0f);
    const glm::mat4 model =
        glm::translate(glm::mat4(1.0f), pivot) *
        glm::rotate(
            glm::mat4(1.0f),
            glm::radians(rotationDegrees),
            glm::vec3(0.0f, 0.0f, 1.0f)) *
        glm::translate(glm::mat4(1.0f), pos - pivot) *
        glm::scale(glm::mat4(1.0f), glm::vec3(textWidth, textHeight, 1.0f));
    const glm::mat4 view = glm::mat4(1.0f);
    const glm::mat4 proj =
        glm::ortho(0.0f, static_cast<float>(mFbWidth), static_cast<float>(mFbHeight), 0.0f, -1.0f, 1.0f);

    glUniformMatrix4fv(mUIShader->GetLocModel(), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(mUIShader->GetLocView(), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(mUIShader->GetLocProj(), 1, GL_FALSE, glm::value_ptr(proj));
    glUniform1i(mUIShader->GetLocDiffuseTexture(), 0);
    glUniform1i(mUIShader->GetLocUseTexture(), 1);
    glUniform4f(mUIShader->GetLocObjectColor(), 1.0f, 1.0f, 1.0f, 1.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cachedTexture->handle);

    mVertexArrays.at("quad")->SetActive();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

}

std::size_t UIRenderer::TextTextureCacheKeyHash::operator()(
    const TextTextureCacheKey& key) const
{
    const std::size_t textHash = std::hash<std::string>{}(key.text);
    const std::size_t colorHash = std::hash<std::uint32_t>{}(key.rgba);
    const std::size_t outlineHash = std::hash<int>{}(key.outlinePixels);
    return textHash ^ (colorHash << 1) ^ (outlineHash << 2);
}

const UIRenderer::CachedTextTexture* UIRenderer::FindOrCreateTextTexture(
    const std::string& text,
    const SDL_Color& color,
    int outlinePixels)
{
    const std::uint32_t rgba =
        static_cast<std::uint32_t>(color.r) << 24 |
        static_cast<std::uint32_t>(color.g) << 16 |
        static_cast<std::uint32_t>(color.b) << 8 |
        static_cast<std::uint32_t>(color.a);
    const TextTextureCacheKey key{text, rgba, outlinePixels};

    const auto existingTexture = mTextTextureCache.find(key);
    if (existingTexture != mTextTextureCache.end()) {
        existingTexture->second.lastUseOrder = ++mNextTextTextureUseOrder;
        return &existingTexture->second;
    }

    constexpr std::size_t maxTextTextureCount = 256;
    if (mTextTextureCache.size() >= maxTextTextureCount) {
        EvictLeastRecentlyUsedTextTexture();
    }

    int unscaledWidth = 0;
    int unscaledHeight = 0;
    const GLuint textureHandle =
        CreateTextTexture(text, unscaledWidth, unscaledHeight, color, 1.0f, outlinePixels);
    if (textureHandle == 0 || unscaledWidth <= 0 || unscaledHeight <= 0) {
        return nullptr;
    }

    const auto [insertedTexture, wasInserted] = mTextTextureCache.emplace(
        key,
        CachedTextTexture{
            textureHandle,
            unscaledWidth,
            unscaledHeight,
            ++mNextTextTextureUseOrder});
    if (!wasInserted) {
        glDeleteTextures(1, &textureHandle);
    }
    return &insertedTexture->second;
}

void UIRenderer::EvictLeastRecentlyUsedTextTexture()
{
    if (mTextTextureCache.empty()) {
        return;
    }

    const auto leastRecentlyUsedTexture = std::min_element(
        mTextTextureCache.begin(),
        mTextTextureCache.end(),
        [](const auto& left, const auto& right) {
            return left.second.lastUseOrder < right.second.lastUseOrder;
        });
    glDeleteTextures(1, &leastRecentlyUsedTexture->second.handle);
    mTextTextureCache.erase(leastRecentlyUsedTexture);
}

void UIRenderer::ClearTextTextureCache()
{
    for (const auto& cacheEntry : mTextTextureCache) {
        glDeleteTextures(1, &cacheEntry.second.handle);
    }
    mTextTextureCache.clear();
}

void UIRenderer::DrawRubyText(float x, float y, float scale,
                              float rubyScaleRatio,
                              float rubyGapRatio,
                              const std::vector<RubyTextSegment>& segments,
                              glm::vec4 color,
                              bool centerBased,
                              float rotationDegrees,
                              float outlineWidth,
                              glm::vec4 outlineColor)
{
    std::vector<std::vector<RubyTextSegment>> lines(1);

    for (const RubyTextSegment& sourceSegment : segments) {
        std::size_t position = 0;
        while (position < sourceSegment.text.size()) {
            const std::size_t actualNewline = sourceSegment.text.find('\n', position);
            const std::size_t escapedNewline = sourceSegment.text.find("\\n", position);

            std::size_t newline = std::string::npos;
            std::size_t delimiterLength = 0;
            if (actualNewline != std::string::npos &&
                (escapedNewline == std::string::npos || actualNewline < escapedNewline)) {
                newline = actualNewline;
                delimiterLength = 1;
            } else if (escapedNewline != std::string::npos) {
                newline = escapedNewline;
                delimiterLength = 2;
            }

            const std::size_t end =
                newline == std::string::npos ? sourceSegment.text.size() : newline;
            if (end > position) {
                RubyTextSegment part = sourceSegment;
                part.text = sourceSegment.text.substr(position, end - position);
                lines.back().emplace_back(std::move(part));
            }

            if (newline == std::string::npos) {
                break;
            }

            lines.emplace_back();
            position = newline + delimiterLength;
        }
    }

    if (lines.empty()) {
        return;
    }

    glUseProgram(mUIShader->GetShaderProgram());

    const float firstLineOffset =
        lines.size() > 1 ? -mFbHeight * 0.0222f : 0.0f;
    const float lineSpacing = mFbHeight * 0.0666f;

    float maximumLineWidth = 0.0f;
    float maximumBaseHeight = 0.0f;
    for (const auto& line : lines) {
        float lineWidth = 0.0f;
        for (const RubyTextSegment& segment : line) {
            int baseWidth = 0;
            int baseHeight = 0;
            if (MeasureText(segment.text, scale, baseWidth, baseHeight)) {
                lineWidth += static_cast<float>(baseWidth);
                maximumBaseHeight =
                    std::max(maximumBaseHeight, static_cast<float>(baseHeight));
            }
        }
        maximumLineWidth = std::max(maximumLineWidth, lineWidth);
    }
    const float totalTextHeight =
        maximumBaseHeight +
        (lines.size() > 1
             ? lineSpacing * static_cast<float>(lines.size() - 1)
             : 0.0f);
    const glm::vec2 rotationPivot =
        centerBased
            ? glm::vec2(x, y)
            : glm::vec2(
                  x + maximumLineWidth * 0.5f,
                  y + totalTextHeight * 0.5f);

    // 本文の中央基準と左上基準で描くルビが同じ位置に揃うよう、共通の回転基準を使う。




    const float baseStartX =
        centerBased ? x - maximumLineWidth * 0.5f : x;
    const float baseStartY =
        centerBased ? y - maximumBaseHeight * 0.5f : y;

    for (std::size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex) {
        float currentX = baseStartX;
        const float lineY =
            baseStartY + firstLineOffset +
            lineSpacing * static_cast<float>(lineIndex);

        for (const RubyTextSegment& segment : lines[lineIndex]) {
            int baseWidth = 0;
            int baseHeight = 0;
            if (!MeasureText(segment.text, scale, baseWidth, baseHeight)) {
                continue;
            }

            if (outlineWidth > 0.0f && outlineColor.a > 0.0f) {
                DrawTextLine(
                    segment.text,
                    currentX,
                    lineY,
                    scale,
                    false,
                    0.0f,
                    outlineColor,
                    rotationDegrees,
                    rotationPivot,
                    outlineWidth);
            }
            DrawTextLine(
                segment.text,
                currentX,
                lineY,
                scale,
                false,
                0.0f,
                color,
                rotationDegrees,
                rotationPivot);

            if (segment.showsRuby && !segment.reading.empty()) {
                const float rubyScale = scale * rubyScaleRatio;
                int rubyWidth = 0;
                int rubyHeight = 0;
                if (MeasureText(segment.reading, rubyScale, rubyWidth, rubyHeight)) {
                    const float rubyX =
                        currentX + (static_cast<float>(baseWidth - rubyWidth) * 0.5f);
                    const float rubyY =
                        lineY -
                        static_cast<float>(rubyHeight) *
                            (0.9f + rubyGapRatio);

                    // 小さいルビに縁取りを付けると文字内部が潰れて読みにくくなるため、縁取りしない。



                    DrawTextLine(
                        segment.reading,
                        rubyX,
                        rubyY,
                        rubyScale,
                        false,
                        0.0f,
                        color,
                        rotationDegrees,
                        rotationPivot);
                }
            }

            currentX += static_cast<float>(baseWidth);
        }
    }
}

void UIRenderer::DrawTexture(
    float x,
    float y,
    float width,
    float height,
    const std::string& textureName,
    bool flipVertical,
    float rotationDegrees,
    float opacity)
{
    const auto textureIt = mTextures.find(textureName);
    if (textureIt == mTextures.end()) {
        return;
    }

    DrawTextureHandle(
        x,
        y,
        width,
        height,
        textureIt->second,
        flipVertical,
        rotationDegrees,
        opacity);
}

void UIRenderer::DrawTextureHandle(
    float x,
    float y,
    float width,
    float height,
    GLuint textureHandle,
    bool flipVertical,
    float rotationDegrees,
    float opacity)
{
    if (textureHandle == 0) {
        return;
    }

    glUseProgram(mUIShader->GetShaderProgram());

    const glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x + width * 0.5f, y + height * 0.5f, 0.0f)) *
                            glm::rotate(
                                glm::mat4(1.0f),
                                glm::radians(rotationDegrees),
                                glm::vec3(0.0f, 0.0f, 1.0f)) *
                            glm::scale(glm::mat4(1.0f), glm::vec3(width, height, 1.0f));
    const glm::mat4 view = glm::mat4(1.0f);
    const glm::mat4 proj =
        glm::ortho(0.0f, static_cast<float>(mFbWidth), static_cast<float>(mFbHeight), 0.0f, -1.0f, 1.0f);

    glUniformMatrix4fv(mUIShader->GetLocModel(), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(mUIShader->GetLocView(), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(mUIShader->GetLocProj(), 1, GL_FALSE, glm::value_ptr(proj));
    glUniform1i(mUIShader->GetLocDiffuseTexture(), 0);
    glUniform1i(mUIShader->GetLocUseTexture(), 1);
    glUniform4f(
        mUIShader->GetLocObjectColor(), 1.0f, 1.0f, 1.0f,
        std::clamp(opacity, 0.0f, 1.0f));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);

    glBindTexture(GL_TEXTURE_2D, textureHandle);

    mVertexArrays.at(flipVertical ? "quadFlipVertical" : "quad")->SetActive();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

