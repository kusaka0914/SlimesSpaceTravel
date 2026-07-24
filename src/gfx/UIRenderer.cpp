#include "UIRenderer.h"

#include "Game.h"
#include "actor/Player.h"
#include "gfx/DebugUIRenderer.h"
#include "gfx/UIShader.h"
#include "gfx/VertexArray.h"
#include "gfx/ui/HudRenderer.h"
#include "gfx/ui/PauseMenuRenderer.h"
#include "gfx/ui/SceneUIRenderer.h"
#include "gfx/ui/StateUIRenderer.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "system/SceneSystem.h"
#include <algorithm>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

UIRenderer::UIRenderer(Game* game)
    : Renderer(game),
      mUIShader(nullptr),
      mUILoadSystem(nullptr),
      mFbWidth(0),
      mFbHeight(0)
{
    Initialize();
}

UIRenderer::~UIRenderer()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
};

void UIRenderer::Initialize()
{
    mUIShaderUnique = std::make_unique<UIShader>();
    mUIShader = mUIShaderUnique.get();

    mUILoadSystemUnique = std::make_unique<UILoadSystem>();
    mUILoadSystem = mUILoadSystemUnique.get();

    mDebugUIRenderer = std::make_unique<DebugUIRenderer>(mGame, this);
    mSceneUIRenderer = std::make_unique<SceneUIRenderer>(mGame, this);
    mHudRenderer = std::make_unique<HudRenderer>(mGame, this);
    mStateUIRenderer = std::make_unique<StateUIRenderer>(mGame, this);
    mPauseMenuRenderer = std::make_unique<PauseMenuRenderer>(mGame, this);

    if (!mUIShader->GetShaderProgram()) {
        glfwTerminate();
        return;
    }

    InitImGui();

    RegisterUITextures();
    RegisterCustomUITextures();
}

void UIRenderer::InitImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    io.Fonts->AddFontFromFileTTF("../assets/fonts/NotoSansJP-Black.ttf", 18.0f, nullptr,
                                 io.Fonts->GetGlyphRangesJapanese());

    const char* glslVersion = "#version 330";

    ImGui_ImplGlfw_InitForOpenGL(mGame->GetWindow(), true);
    ImGui_ImplOpenGL3_Init(glslVersion);
}

void UIRenderer::RegisterUITextures()
{
    std::string basePath = "../assets/textures/";
    RegisterTexture(basePath + "titleBg.png", "titleBg");
    RegisterTexture(basePath + "opening.png", "opening");
    RegisterTexture(basePath + "textBg.png", "textBg");
    RegisterTexture(basePath + "slime.png", "slime");
    RegisterTexture(basePath + "hp.png", "hp");
    RegisterTexture(basePath + "special.png", "special");
    RegisterTexture(basePath + "skyBox.png", "skyBox");
    RegisterTexture(basePath + "jewel.png", "jewel");
}

void UIRenderer::RegisterCustomUITextures()
{
    if (!mUILoadSystem) {
        return;
    }

    for (const UILoadSystem::CustomElement& element : mUILoadSystem->GetCustomElements()) {
        if (element.type == UILoadSystem::CustomElementType::Image && !element.texturePath.empty()) {
            RegisterCustomUITexture(element.texturePath);
        }
    }
}

void UIRenderer::Draw()
{
    glfwGetFramebufferSize(mGame->GetWindow(), &mFbWidth, &mFbHeight);
    glViewport(0, 0, mFbWidth, mFbHeight);
    glUseProgram(mUIShader->GetShaderProgram());

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    SceneSystem* sceneSystem = mGame->GetSceneSystem();

    if (sceneSystem->IsTitle()) {
        mSceneUIRenderer->DrawTitle();
    }

    if (sceneSystem->IsOpening()) {
        mSceneUIRenderer->DrawOpening();
    }

    if (sceneSystem->IsGameOver()) {
        mSceneUIRenderer->DrawGameOver();
    }

    const bool shouldDrawDefaultUI = sceneSystem->IsPlaying() || sceneSystem->IsJewelTutorialShowing();
    if (shouldDrawDefaultUI) {
        mHudRenderer->DrawDefaultUI();
    }

    mStateUIRenderer->DrawStateUI();

    if (mGame->GetIsPauseMenuOpen()) {
        mPauseMenuRenderer->Draw();
    }

    DrawCustomUI();

    if (mGame->GetIsDebugEditorShowing()) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        mDebugUIRenderer->Draw();

        EndImGuiFrame();
    }

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
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

std::string UIRenderer::GetCustomTextureName(const std::string& assetRelativePath)
{
    std::string normalizedPath = assetRelativePath;
    std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');
    return "custom-ui:" + normalizedPath;
}

void UIRenderer::DrawCustomUI()
{
    if (!mUILoadSystem) {
        return;
    }

    const auto& elements = mUILoadSystem->GetCustomElements();
    std::vector<const UILoadSystem::CustomElement*> sortedElements;
    sortedElements.reserve(elements.size());

    for (const UILoadSystem::CustomElement& element : elements) {
        sortedElements.push_back(&element);
    }

    std::stable_sort(
        sortedElements.begin(),
        sortedElements.end(),
        [](const UILoadSystem::CustomElement* lhs, const UILoadSystem::CustomElement* rhs) {
            return lhs->zOrder < rhs->zOrder;
        });

    const bool previewAll = mCustomUIPreviewEnabled && mGame->GetIsDebugEditorShowing();

    for (const UILoadSystem::CustomElement* element : sortedElements) {
        if (!element || (!previewAll && !mUILoadSystem->IsCustomElementVisible(*element))) {
            continue;
        }

        const float x = mFbWidth * element->xRatio;
        const float y = mFbWidth * element->yRatio;
        const float width = mFbWidth * element->widthRatio;
        const float height = mFbWidth * element->heightRatio;
        const float topLeftX = element->centerBased ? x - width * 0.5f : x;
        const float topLeftY = element->centerBased ? y - height * 0.5f : y;

        switch (element->type) {
        case UILoadSystem::CustomElementType::Text:
            DrawText(
                x,
                y,
                mFbWidth * element->textScaleRatio,
                element->text,
                element->centerBased,
                glm::vec4(
                    element->color[0] * 255.0f,
                    element->color[1] * 255.0f,
                    element->color[2] * 255.0f,
                    element->color[3] * 255.0f));
            break;
        case UILoadSystem::CustomElementType::Image:
            if (RegisterCustomUITexture(element->texturePath)) {
                DrawTexture(
                    topLeftX,
                    topLeftY,
                    width,
                    height,
                    GetCustomTextureName(element->texturePath),
                    element->flipVertical);
            }
            break;
        case UILoadSystem::CustomElementType::Panel:
            DrawBG(
                topLeftX,
                topLeftY,
                width,
                height,
                {element->color[0], element->color[1], element->color[2], element->color[3]});
            break;
        }
    }
}

void UIRenderer::EndImGuiFrame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UIRenderer::DrawSkyBox()
{
    glfwGetFramebufferSize(mGame->GetWindow(), &mFbWidth, &mFbHeight);
    glUseProgram(mUIShader->GetShaderProgram());

    DrawTexture(0.0f, 0.0f, mFbWidth, mFbHeight, "skyBox");
}

void UIRenderer::DrawSceneText(const std::string& sceneName, const std::string& UIName, bool isCenterBase, int index,
                               glm::vec4 color)
{
    const auto textInfo = mUILoadSystem->GetTextInfo(sceneName, UIName);
    if (!textInfo) {
        return;
    }

    if (index < 0 || index >= static_cast<int>(textInfo->texts.size())) {
        return;
    }

    DrawText(mFbWidth * textInfo->xRatio, mFbHeight * textInfo->yRatio, mFbWidth * textInfo->scaleRatio,
             textInfo->texts[index], isCenterBase, color);
}

void UIRenderer::DrawTalkUI(const std::vector<std::string>& texts, int index)
{
    DrawSceneTexture("state", "talkBgTexture", "textBg");

    const auto talkTextInfo = mUILoadSystem->GetTextInfo("state", "talkText");
    if (!talkTextInfo) {
        return;
    }

    if (index < 0 || index >= static_cast<int>(texts.size())) {
        return;
    }

    constexpr glm::vec4 black{0.0f, 0.0f, 0.0f, 255.0f};
    DrawText(mFbWidth * talkTextInfo->xRatio, mFbHeight * talkTextInfo->yRatio, mFbWidth * talkTextInfo->scaleRatio,
             texts[index], false, black);
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
    DrawTexture(mFbWidth * (textInfo->xRatio - textureMarginX), mFbHeight * (textInfo->yRatio - textureMarginY),
                mFbWidth * talkBgTextureInfo->widthRatio, mFbHeight * talkBgTextureInfo->heightRatio, "textBg");

    const glm::vec4 black{0.0f, 0.0f, 0.0f, 255.0f};
    const int talkUIIndex = mGame->GetSceneSystem()->GetTalkUIIndex();

    if (talkUIIndex < 0 || talkUIIndex >= static_cast<int>(textInfo->texts.size())) {
        return;
    }

    DrawText(mFbWidth * textInfo->xRatio, mFbHeight * textInfo->yRatio, mFbWidth * textInfo->scaleRatio,
             textInfo->texts[talkUIIndex], false, black);
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
                                                 bool isCenterBase, float screenTopY, float uiScale)
{
    const UILoadSystem::TextInfo* textInfo;
    if (mGame->IsGameControllerConnected()) {
        textInfo = mUILoadSystem->GetTextInfo(sceneName, UIName + "ForGameController");
    } else {
        textInfo = mUILoadSystem->GetTextInfo(sceneName, UIName + "ForKeyBoard");
    }

    if (!textInfo) {
        return;
    }

    const float screenHeight = mFbHeight * uiScale;

    const float x = mFbWidth * textInfo->xRatio;
    const float y = screenTopY + screenHeight * textInfo->yRatio;
    const float scale = mFbWidth * textInfo->scaleRatio * uiScale;

    DrawText(x, y, scale, textInfo->texts[0], isCenterBase);
}

void UIRenderer::DrawTextDependsOnPlayerInput(const Player* player, const std::string& sceneName,
                                              const std::string& UIName, bool isCenterBase, float screenTopY,
                                              float uiScale)
{
    const UILoadSystem::TextInfo* textInfo = nullptr;

    if (UsesControllerUI(player)) {
        textInfo = mUILoadSystem->GetTextInfo(sceneName, UIName + "ForGameController");
    } else {
        textInfo = mUILoadSystem->GetTextInfo(sceneName, UIName + "ForKeyBoard");
    }

    if (!textInfo) {
        return;
    }

    const float screenHeight = mFbHeight * uiScale;

    const float x = mFbWidth * textInfo->xRatio;
    const float y = screenTopY + screenHeight * textInfo->yRatio;
    const float scale = mFbWidth * textInfo->scaleRatio * uiScale;

    DrawText(x, y, scale, textInfo->texts[0], isCenterBase);
}

bool UIRenderer::UsesControllerUI(const Player* player) const
{
    if (!player) {
        return false;
    }

    return mGame->IsGameControllerConnected() && player->GetPlayerNum() == 1;
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

    DrawBG(mFbWidth * textureInfo->xRatio, mFbHeight * textureInfo->yRatio, mFbWidth * textureInfo->widthRatio,
           mFbHeight * textureInfo->heightRatio, color);
}

void UIRenderer::DrawSceneTexture(const std::string& sceneName, const std::string& UIName,
                                  const std::string& textureName)
{
    const auto textureInfo = mUILoadSystem->GetTextureInfo(sceneName, UIName);
    if (!textureInfo) {
        return;
    }

    DrawTexture(mFbWidth * textureInfo->xRatio, mFbHeight * textureInfo->yRatio, mFbWidth * textureInfo->widthRatio,
                mFbHeight * textureInfo->heightRatio, textureName);
}

void UIRenderer::DrawLinedUpTexture(const std::string& sceneName, const std::string& UIName,
                                    const std::string& textureName, float gap, int count, float screenTopY,
                                    float uiScale)
{
    const auto textureInfo = mUILoadSystem->GetTextureInfo(sceneName, UIName);
    if (!textureInfo) {
        return;
    }

    const float screenHeight = mFbHeight * uiScale;

    const float textureX = mFbWidth * textureInfo->xRatio;
    const float textureY = screenTopY + screenHeight * textureInfo->yRatio;

    const float textureWidth = mFbWidth * textureInfo->widthRatio * uiScale;
    const float textureHeight = mFbWidth * textureInfo->heightRatio * uiScale;

    const float textureGap = gap * uiScale;

    float currentX = textureX;

    while (count > 0) {
        DrawTexture(currentX, textureY, textureWidth, textureHeight, textureName);
        currentX += textureGap;
        count--;
    }
}

void UIRenderer::DrawBG(float x, float y, float width, float height, std::vector<GLfloat> color)
{
    glUseProgram(mUIShader->GetShaderProgram());

    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x + width * 0.5f, y + height * 0.5f, 0.0f)) *
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

void UIRenderer::DrawText(float x, float y, float scale, const std::string& message, bool isCenterBase, glm::vec4 color)
{
    glUseProgram(mUIShader->GetShaderProgram());

    std::string message1 = message;
    std::string message2;
    const bool isNewLine = SplitText(message, message1, message2);

    DrawTextLine(message1, x, y, scale, isCenterBase, isNewLine ? -mFbHeight * 0.0222f : 0.0f, color);

    if (!isNewLine) {
        return;
    }

    DrawTextLine(message2, x, y, scale, isCenterBase, mFbHeight * 0.0444f, color);
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

void UIRenderer::DrawTextLine(const std::string& message, float x, float y, float scale, bool isCenterBase,
                              float yOffset, glm::vec4 color)
{
    int textWidth = 0;
    int textHeight = 0;

    const SDL_Color textColor{static_cast<Uint8>(color.x), static_cast<Uint8>(color.y), static_cast<Uint8>(color.z),
                              static_cast<Uint8>(color.w)};

    GLuint textTexture = CreateTextTexture(message, textWidth, textHeight, textColor, scale);

    if (textTexture == 0 || textWidth <= 0 || textHeight <= 0) {
        return;
    }

    glm::vec3 pos;
    if (isCenterBase) {
        pos = glm::vec3(x, y + yOffset, 0.0f);
    } else {
        pos = glm::vec3(x + textWidth * 0.5f, y + textHeight * 0.5f + yOffset, 0.0f);
    }

    const glm::mat4 model =
        glm::translate(glm::mat4(1.0f), pos) * glm::scale(glm::mat4(1.0f), glm::vec3(textWidth, textHeight, 1.0f));
    const glm::mat4 view = glm::mat4(1.0f);
    const glm::mat4 proj =
        glm::ortho(0.0f, static_cast<float>(mFbWidth), static_cast<float>(mFbHeight), 0.0f, -1.0f, 1.0f);

    glUniformMatrix4fv(mUIShader->GetLocModel(), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(mUIShader->GetLocView(), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(mUIShader->GetLocProj(), 1, GL_FALSE, glm::value_ptr(proj));
    glUniform1i(mUIShader->GetLocDiffuseTexture(), 0);
    glUniform1i(mUIShader->GetLocUseTexture(), 1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textTexture);

    mVertexArrays.at("quad")->SetActive();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDeleteTextures(1, &textTexture);
}

void UIRenderer::DrawTexture(
    float x,
    float y,
    float width,
    float height,
    const std::string& textureName,
    bool flipVertical)
{
    glUseProgram(mUIShader->GetShaderProgram());

    const glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x + width * 0.5f, y + height * 0.5f, 0.0f)) *
                            glm::scale(glm::mat4(1.0f), glm::vec3(width, height, 1.0f));
    const glm::mat4 view = glm::mat4(1.0f);
    const glm::mat4 proj =
        glm::ortho(0.0f, static_cast<float>(mFbWidth), static_cast<float>(mFbHeight), 0.0f, -1.0f, 1.0f);

    glUniformMatrix4fv(mUIShader->GetLocModel(), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(mUIShader->GetLocView(), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(mUIShader->GetLocProj(), 1, GL_FALSE, glm::value_ptr(proj));
    glUniform1i(mUIShader->GetLocDiffuseTexture(), 0);
    glUniform1i(mUIShader->GetLocUseTexture(), 1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);

    auto textureIt = mTextures.find(textureName);
    if (textureIt == mTextures.end()) {
        return;
    }

    glBindTexture(GL_TEXTURE_2D, textureIt->second);

    mVertexArrays.at(flipVertical ? "quadFlipVertical" : "quad")->SetActive();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
