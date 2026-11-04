#include "gfx/UIRenderer.h"

#include "actor/Player.h"
#include "app/Game.h"
#include "gfx/UIShader.h"
#include "gfx/VertexArray.h"
#include "system/SceneSystem.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <utility>

void UIRenderer::DrawSkyBox(int renderWidth, int renderHeight)
{
    if (renderWidth <= 0 || renderHeight <= 0) {
        glfwGetFramebufferSize(mGame->GetWindow(), &renderWidth, &renderHeight);
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

void UIRenderer::DrawSceneText(const std::string& sceneName, const std::string& UIName, int index, glm::vec4 color)
{
    const auto textInfo = mUILoadSystem->GetTextInfo(sceneName, UIName);
    if (!textInfo) {
        return;
    }

    if (index < 0 || index >= static_cast<int>(textInfo->texts.size())) {
        return;
    }

    DrawTextForElement(sceneName, UIName, mFbWidth * textInfo->xRatio, mFbHeight * textInfo->yRatio,
                       mFbWidth * textInfo->scaleRatio, textInfo->texts[index], textInfo->centerBased, color,
                       textInfo->rotationDegrees);
}

void UIRenderer::DrawTalkUI(const std::vector<std::string>& texts, int index,
                            const std::vector<RubyTextSegment>* rubySegments)
{
    DrawSceneTexture("state", "talkBgTexture", "textBg");

    const auto talkTextInfo = mUILoadSystem->GetTextInfo("state", "talkText");
    const auto talkBgTextureInfo = mUILoadSystem->GetTextureInfo("state", "talkBgTexture");
    if (!talkTextInfo || !talkBgTextureInfo) {
        return;
    }

    if (index < 0 || index >= static_cast<int>(texts.size())) {
        return;
    }

    constexpr glm::vec4 talkTextColor{35.0f, 35.0f, 42.0f, 255.0f};
    const float x = mFbWidth * talkTextInfo->xRatio;
    const float y = mFbHeight * talkTextInfo->yRatio;
    const float scale = mFbWidth * talkTextInfo->scaleRatio;

    if (rubySegments && !rubySegments->empty() && JoinRubyBaseText(*rubySegments) == texts[index]) {
        DrawRubyText(x, y, scale, talkTextInfo->rubyScaleRatio, talkTextInfo->rubyGapRatio, *rubySegments,
                     talkTextColor, talkTextInfo->centerBased, talkTextInfo->rotationDegrees);
        RecordRenderedTextElement("state", "talkText", x, y, scale, texts[index], talkTextInfo->centerBased,
                                  talkTextInfo->rotationDegrees);
    } else {
        DrawTextForElement("state", "talkText", x, y, scale, texts[index], talkTextInfo->centerBased, talkTextColor,
                           talkTextInfo->rotationDegrees);
    }

    DrawTalkAdvancePrompt();
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
    const float backgroundX = mFbWidth * (textInfo->xRatio - textureMarginX);
    const float backgroundY = mFbHeight * (textInfo->yRatio - textureMarginY);
    const float backgroundWidth = mFbWidth * talkBgTextureInfo->widthRatio;
    const float backgroundHeight = mFbHeight * talkBgTextureInfo->heightRatio;
    DrawTexture(backgroundX, backgroundY, backgroundWidth, backgroundHeight, "textBg", false,
                talkBgTextureInfo->rotationDegrees);
    RecordRenderedUIElement(RenderedUIElementSource::CodeBoundTexture, "state", "talkBgTexture",
                            glm::vec2(backgroundX + backgroundWidth * 0.5f, backgroundY + backgroundHeight * 0.5f),
                            glm::vec2(backgroundWidth, backgroundHeight), talkBgTextureInfo->rotationDegrees);

    const glm::vec4 talkTextColor{35.0f, 35.0f, 42.0f, 255.0f};
    const int talkUIIndex = mGame->GetSceneSystem()->GetTalkUIIndex();

    if (talkUIIndex < 0 || talkUIIndex >= static_cast<int>(textInfo->texts.size())) {
        return;
    }

    const std::string textInfoKey = mUILoadSystem->FindTextInfoKey(textInfo);
    const std::size_t keySeparator = textInfoKey.find('.');
    const std::string screen =
        keySeparator == std::string::npos ? std::string("state") : textInfoKey.substr(0, keySeparator);
    const std::string id =
        keySeparator == std::string::npos ? std::string("talkText") : textInfoKey.substr(keySeparator + 1);

    if (talkUIIndex < static_cast<int>(textInfo->rubySegments.size()) && !textInfo->rubySegments[talkUIIndex].empty() &&
        JoinRubyBaseText(textInfo->rubySegments[talkUIIndex]) == textInfo->texts[talkUIIndex]) {
        const UILoadSystem::TextInfo* globalTalkTextInfo = mUILoadSystem->GetTextInfo("state", "talkText");
        const float rubyScaleRatio = globalTalkTextInfo ? globalTalkTextInfo->rubyScaleRatio : 0.48f;
        const float rubyGapRatio = globalTalkTextInfo ? globalTalkTextInfo->rubyGapRatio : 0.0f;
        DrawRubyText(mFbWidth * textInfo->xRatio, mFbHeight * textInfo->yRatio, mFbWidth * textInfo->scaleRatio,
                     rubyScaleRatio, rubyGapRatio, textInfo->rubySegments[talkUIIndex], talkTextColor,
                     textInfo->centerBased, textInfo->rotationDegrees);
        RecordRenderedTextElement(screen, id, mFbWidth * textInfo->xRatio, mFbHeight * textInfo->yRatio,
                                  mFbWidth * textInfo->scaleRatio, textInfo->texts[talkUIIndex], textInfo->centerBased,
                                  textInfo->rotationDegrees);
    } else {
        DrawTextForElement(screen, id, mFbWidth * textInfo->xRatio, mFbHeight * textInfo->yRatio,
                           mFbWidth * textInfo->scaleRatio, textInfo->texts[talkUIIndex], textInfo->centerBased,
                           talkTextColor, textInfo->rotationDegrees);
    }

    DrawTalkAdvancePrompt();
}

void UIRenderer::DrawTalkAdvancePrompt()
{
    SceneSystem* sceneSystem = mGame->GetSceneSystem();
    if (!sceneSystem || sceneSystem->IsWaitingForTutorialPlayerAction()) {
        return;
    }

    const UILoadSystem::TextureInfo* promptTextureInfo = mUILoadSystem->GetTextureInfo("state", "talkAdvancePrompt");
    if (!promptTextureInfo) {
        return;
    }

    const bool usesController =
        mGame->IsGameControllerConnected() && mGame->GetLastUsedInputDevice() == InputDeviceType::GameController;
    const std::string texturePath =
        usesController ? "textures/Inputs/switch_button_a.png" : "textures/Inputs/Keyboard/keyboard_space.png";
    if (!RegisterCustomUITexture(texturePath)) {
        return;
    }

    const GLuint textureHandle = GetCustomUITextureHandle(texturePath);
    if (textureHandle == 0) {
        return;
    }

    const float iconX = mFbWidth * promptTextureInfo->xRatio;
    const float iconY = mFbHeight * promptTextureInfo->yRatio;
    const float iconWidth = mFbWidth * promptTextureInfo->widthRatio;
    const float iconHeight = mFbHeight * promptTextureInfo->heightRatio;

    constexpr glm::vec4 talkPromptColor{35.0f / 255.0f, 35.0f / 255.0f, 42.0f / 255.0f, 0.9f};
    DrawTextureHandle(iconX, iconY, iconWidth, iconHeight, textureHandle, false, promptTextureInfo->rotationDegrees,
                      1.0f, talkPromptColor);
    RecordRenderedUIElement(RenderedUIElementSource::CodeBoundTexture, "state", "talkAdvancePrompt",
                            glm::vec2(iconX + iconWidth * 0.5f, iconY + iconHeight * 0.5f),
                            glm::vec2(iconWidth, iconHeight), promptTextureInfo->rotationDegrees);
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

    DrawTextForElement(sceneName, resolvedUIName, x, y, scale, textInfo->texts[0], textInfo->centerBased,
                       {255, 255, 255, 255}, textInfo->rotationDegrees);
}

void UIRenderer::DrawTextDependsOnPlayerInput(const Player* player, const std::string& sceneName,
                                              const std::string& UIName, float screenTopY, float screenHeight)
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

    DrawTextForElement(sceneName, resolvedUIName, x, y, scale, textInfo->texts[0], textInfo->centerBased,
                       {255, 255, 255, 255}, textInfo->rotationDegrees);
}

bool UIRenderer::UsesControllerUI(const Player* player) const
{
    if (!player) {
        return false;
    }

    if (!mGame->IsGameControllerConnected()) {
        return false;
    }

    return !mGame->GetIsPlayer2Joined() || mGame->HasGameControllerForPlayer(player->GetPlayerNum());
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
    RecordRenderedUIElement(RenderedUIElementSource::CodeBoundTexture, sceneName, UIName,
                            glm::vec2(x + width * 0.5f, y + height * 0.5f), glm::vec2(width, height),
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
    DrawTexture(x, y, width, height, textureName, false, textureInfo->rotationDegrees);
    RecordRenderedUIElement(RenderedUIElementSource::CodeBoundTexture, sceneName, UIName,
                            glm::vec2(x + width * 0.5f, y + height * 0.5f), glm::vec2(width, height),
                            textureInfo->rotationDegrees);
}

void UIRenderer::DrawLinedUpTexture(const std::string& sceneName, const std::string& UIName,
                                    const std::string& textureName, int count, float screenTopY, float uiScale)
{
    DrawLinedUpTextureSlots(sceneName, UIName, textureName, count, count, 1.0f, screenTopY, uiScale);
}

void UIRenderer::DrawLinedUpTextureSlots(const std::string& sceneName, const std::string& UIName,
                                         const std::string& textureName, int activeSlotCount, int totalSlotCount,
                                         float inactiveSlotBrightness, float screenTopY, float uiScale,
                                         const TextureSlotFlash* flash, float activeSlotOpacity)
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

    const float textureGap = mFbWidth * textureInfo->repeatSpacingRatio.value_or(textureInfo->widthRatio) * uiScale;

    const int visibleActiveSlotCount = std::clamp(activeSlotCount, 0, totalSlotCount);
    const float inactiveBrightness = std::clamp(inactiveSlotBrightness, 0.0f, 1.0f);
    const auto texture = mTextures.find(textureName);
    if (texture == mTextures.end()) {
        return;
    }

    for (int slotIndex = 0; slotIndex < totalSlotCount; ++slotIndex) {
        const float currentX = textureX + textureGap * slotIndex;
        const bool isDepleted = slotIndex >= visibleActiveSlotCount;
        const float strength = flash && slotIndex >= flash->firstSlot && slotIndex < flash->endSlot && isDepleted
                                   ? std::clamp(flash->strength, 0.0f, 1.0f)
                                   : 0.0f;
        const float brightness = isDepleted ? glm::mix(inactiveBrightness, 1.0f, strength) : 1.0f;
        DrawTextureHandle(currentX, textureY, textureWidth, textureHeight, texture->second, false,
                          textureInfo->rotationDegrees, isDepleted ? 1.0f : activeSlotOpacity,
                          glm::vec4(brightness, brightness, brightness, 1.0f), isDepleted, strength,
                          flash ? flash->color : glm::vec3(1.0f));
        RecordRenderedUIElement(RenderedUIElementSource::CodeBoundTexture, sceneName, UIName,
                                glm::vec2(currentX + textureWidth * 0.5f, textureY + textureHeight * 0.5f),
                                glm::vec2(textureWidth, textureHeight), textureInfo->rotationDegrees);
    }
}

void UIRenderer::DrawBG(float x, float y, float width, float height, std::vector<GLfloat> color, float rotationDegrees,
                        float cornerRadius, float borderWidth, glm::vec4 borderColor)
{
    glUseProgram(mUIShader->GetShaderProgram());

    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x + width * 0.5f, y + height * 0.5f, 0.0f)) *
                      glm::rotate(glm::mat4(1.0f), glm::radians(rotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f)) *
                      glm::scale(glm::mat4(1.0f), glm::vec3(width, height, 1.0f));
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projMat =
        glm::ortho(0.0f, static_cast<float>(mFbWidth), static_cast<float>(mFbHeight), 0.0f, -1.0f, 1.0f);

    glUniformMatrix4fv(mUIShader->GetLocModel(), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(mUIShader->GetLocView(), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(mUIShader->GetLocProj(), 1, GL_FALSE, glm::value_ptr(projMat));
    glUniform1i(mUIShader->GetLocUseTexture(), 0);
    glUniform2f(mUIShader->GetLocPanelSize(), width, height);
    glUniform1f(mUIShader->GetLocBorderWidth(),
                std::clamp(borderWidth, 0.0f, std::max(0.0f, std::min(width, height) * 0.5f)));
    glUniform4fv(mUIShader->GetLocBorderColor(), 1, glm::value_ptr(borderColor));
    glUniform1f(mUIShader->GetLocCornerRadius(),
                std::clamp(cornerRadius, 0.0f, std::max(0.0f, std::min(width, height) * 0.5f)));
    glUniform4fv(mUIShader->GetLocObjectColor(), 1, color.data());

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    mVertexArrays.at("quad")->SetActive();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glUniform1f(mUIShader->GetLocCornerRadius(), 0.0f);
    glUniform1f(mUIShader->GetLocBorderWidth(), 0.0f);
}

void UIRenderer::DrawDamageVignette(float viewportTop, float viewportHeight, float opacity)
{
    if (opacity <= 0.0f || viewportHeight <= 0.0f) {
        return;
    }
    glUseProgram(mUIShader->GetShaderProgram());
    glUniform1i(mUIShader->GetLocDamageVignette(), 1);
    DrawBG(0.0f, viewportTop, static_cast<float>(mFbWidth), viewportHeight,
           {1.0f, 0.04f, 0.04f, std::clamp(opacity, 0.0f, 1.0f)});
    glUniform1i(mUIShader->GetLocDamageVignette(), 0);
}

void UIRenderer::DrawTexture(float x, float y, float width, float height, const std::string& textureName,
                             bool flipVertical, float rotationDegrees, float opacity)
{
    const auto textureIterator = mTextures.find(textureName);
    if (textureIterator == mTextures.end()) {
        return;
    }

    DrawTextureHandle(x, y, width, height, textureIterator->second, flipVertical, rotationDegrees, opacity);
}

void UIRenderer::DrawTextureHandle(float x, float y, float width, float height, GLuint textureHandle, bool flipVertical,
                                   float rotationDegrees, float opacity, glm::vec4 tint, bool grayscale,
                                   float flashStrength, glm::vec3 flashColor)
{
    if (textureHandle == 0) {
        return;
    }

    glUseProgram(mUIShader->GetShaderProgram());

    const glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x + width * 0.5f, y + height * 0.5f, 0.0f)) *
                            glm::rotate(glm::mat4(1.0f), glm::radians(rotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f)) *
                            glm::scale(glm::mat4(1.0f), glm::vec3(width, height, 1.0f));
    const glm::mat4 view = glm::mat4(1.0f);
    const glm::mat4 projMat =
        glm::ortho(0.0f, static_cast<float>(mFbWidth), static_cast<float>(mFbHeight), 0.0f, -1.0f, 1.0f);

    glUniformMatrix4fv(mUIShader->GetLocModel(), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(mUIShader->GetLocView(), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(mUIShader->GetLocProj(), 1, GL_FALSE, glm::value_ptr(projMat));
    glUniform1i(mUIShader->GetLocDiffuseTexture(), 0);
    glUniform1i(mUIShader->GetLocUseTexture(), 1);
    glUniform1i(mUIShader->GetLocGrayscale(), grayscale ? 1 : 0);
    glUniform1f(mUIShader->GetLocFlashStrength(), std::clamp(flashStrength, 0.0f, 1.0f));
    glUniform3fv(mUIShader->GetLocFlashColor(), 1, glm::value_ptr(flashColor));
    tint.a *= std::clamp(opacity, 0.0f, 1.0f);
    glUniform4fv(mUIShader->GetLocObjectColor(), 1, glm::value_ptr(tint));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);

    glBindTexture(GL_TEXTURE_2D, textureHandle);

    mVertexArrays.at(flipVertical ? "quadFlipVertical" : "quad")->SetActive();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    // 文字など、同じシェーダーを使う後続のUIへグレー化を持ち越さない。
    glUniform1i(mUIShader->GetLocGrayscale(), 0);
    glUniform1f(mUIShader->GetLocFlashStrength(), 0.0f);
}
void UIRenderer::DrawText(float x, float y, float scale, const std::string& message, bool isCenterBase, glm::vec4 color,
                          float rotationDegrees, const TextEffect* effect)
{
    if (mTextRenderer) {
        mTextRenderer->DrawText({mFbWidth, mFbHeight}, x, y, scale, message, isCenterBase, color, rotationDegrees,
                                effect);
    }
}

bool UIRenderer::SplitText(const std::string& message, std::string& firstLine, std::string& secondLine) const
{
    return UITextRenderer::SplitText(message, firstLine, secondLine);
}

void UIRenderer::DrawRubyText(float x, float y, float scale, float rubyScaleRatio, float rubyGapRatio,
                              const std::vector<RubyTextSegment>& segments, glm::vec4 color, bool centerBased,
                              float rotationDegrees, float outlineWidth, glm::vec4 outlineColor)
{
    if (mTextRenderer) {
        mTextRenderer->DrawRubyText({mFbWidth, mFbHeight}, x, y, scale, rubyScaleRatio, rubyGapRatio, segments, color,
                                    centerBased, rotationDegrees, outlineWidth, outlineColor);
    }
}
