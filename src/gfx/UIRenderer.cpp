#include "UIRenderer.h"
#include "DebugUIRenderer.h"
#include "Game.h"
#include "VertexArray.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "gfx/UIShader.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "state/UIState.h"
#include "system/SceneSystem.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>

UIRenderer::UIRenderer(Game* game)
    : Renderer(game)
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

    if (!mUIShader->GetShaderProgram()) {
        glfwTerminate();
        return;
    }

    InitImGui();

    RegisterUITextures();
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

void UIRenderer::Draw()
{
    glfwGetFramebufferSize(mGame->GetWindow(), &mFbWidth, &mFbHeight);
    glUseProgram(mUIShader->GetShaderProgram());

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    if (mGame->GetSceneSystem()->IsTitle()) {
        DrawTitle();
    }

    if (mGame->GetSceneSystem()->IsOpening()) {
        DrawOpening();
    }

    if (mGame->GetSceneSystem()->IsGameOver()) {
        DrawGameOver();
    }

    const bool shouldDrawDefaultUI =
        mGame->GetSceneSystem()->IsPlaying() || mGame->GetSceneSystem()->IsJewelTutorialShowing();
    if (shouldDrawDefaultUI) {
        DrawDefaultUI();
    }

    DrawStateUI();

    if (mGame->GetIsDebugMode()) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        mDebugUIRenderer->Draw();

        EndImGuiFrame();
    }

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

void UIRenderer::EndImGuiFrame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void UIRenderer::DrawTitle()
{
    DrawSceneTexture("title", "bgTexture", "titleBg");
    DrawTextDependsOnGameController("title", "startText", true);
}

void UIRenderer::DrawOpening()
{
    if (mGame->GetSceneSystem()->IsTalkWithOpening()) {
        DrawOpeningIntro();
    } else if (mGame->GetSceneSystem()->IsTalkWithMother()) {
        DrawOpeningTalkWithMother();
    } else if (mGame->GetSceneSystem()->IsTalkWithDoctor()) {
        DrawOpeningTalkWithDoctor();
    }
    DrawSkipUI();
}

void UIRenderer::DrawSkipUI()
{
    DrawTextDependsOnGameController("opening", "skipText", false);
}

void UIRenderer::DrawGameOver()
{
    DrawBG(0.0f, 0.0f, mFbWidth, mFbHeight, {0.0f, 0.0f, 0.0f, 0.5f});
    DrawSceneText("gameOver", "gameOverText", true, 0);
    DrawTextDependsOnGameController("gameOver", "restartText", true);
}

void UIRenderer::DrawOpeningIntro()
{
    DrawSceneTexture("opening", "bgTexture", "opening");
    if (DrawSceneTalkUI("opening", "openingText")) {
        return;
    }

    mGame->GetSceneSystem()->GetUIState()->StartTalkWith(UIState::TalkWith::Mother);
}

void UIRenderer::DrawOpeningTalkWithMother()
{
    if (DrawSceneTalkUI("opening", "talkWithMotherText")) {
        return;
    }

    mGame->GetSceneSystem()->GetUIState()->StartTalkWith(UIState::TalkWith::Doctor);
}

void UIRenderer::DrawOpeningTalkWithDoctor()
{
    const UILoadSystem::TextInfo* talkWithDoctorTextInfo = mUILoadSystem->GetTextInfo("opening", "talkWithDoctorText");
    if (!talkWithDoctorTextInfo) {
        return;
    }

    const int talkUIIndex = mGame->GetSceneSystem()->GetTalkUIIndex();
    const std::vector<std::string>& talkTexts = talkWithDoctorTextInfo->texts;
    const bool isTalking = talkUIIndex >= 0 && talkUIIndex < static_cast<int>(talkTexts.size());
    if (isTalking) {
        DrawTalkUI(talkWithDoctorTextInfo);
        return;
    }

    const bool isFinishTalk = talkUIIndex >= static_cast<int>(talkTexts.size());
    if (isFinishTalk) {
        mGame->GetSceneSystem()->StartFadeIn();
    }
}

void UIRenderer::DrawDefaultUI()
{
    DrawOperationSupportUI();

    const Player* player = mGame->GetPlayers()[0];
    const NPC* talkableNPC = player->GetTalkableNPC();
    if (talkableNPC) {
        if (talkableNPC->GetIsTalkable()) {
            DrawTalkableUI();
        }
    }

    if (mGame->IsInBase()) {
        return;
    }

    const int hp = player->GetHp();
    const bool isAlive = hp > 0;
    if (isAlive) {
        DrawHpUI(hp);
    }

    const bool isDanger = hp <= 3;
    if (isDanger) {
        DrawDangerBg(hp);
    }

    const int jewelCount = player->GetJewelCount();
    const bool haveJewel = jewelCount > 0;
    if (haveJewel) {
        DrawJewelUI(jewelCount);
    }

    const int remainBoatPartsCount = player->GetCurrentPlanet()->GetRemainBoatPartsCount();
    const bool isRemain = remainBoatPartsCount != 0;
    if (isRemain) {
        DrawRemainPartsUI(remainBoatPartsCount);
    }
}

void UIRenderer::DrawOperationSupportUI()
{
    const bool isOperationUIShow = mGame->GetSceneSystem()->GetUIState()->GetIsOperationUIShow();
    if (isOperationUIShow) {
        DrawTextDependsOnGameController("default", "operationSupportText", false);
        return;
    }

    DrawSceneText("default", "operationSupportHiddenText", false, 0);
}

void UIRenderer::DrawHpUI(int hp)
{
    const float hpGap = mFbWidth / 28.0f;
    DrawLinedUpTexture("default", "hpTexture", "hp", hpGap, hp);
}

void UIRenderer::DrawDangerBg(int hp)
{
    // 体力が少なくなるにつれて濃い背景になる
    if (hp == 3) {
        DrawBG(0.0f, 0.0f, mFbWidth, mFbHeight, {1.0f, 0.0f, 0.0f, 0.05f});
    } else if (hp == 2) {
        DrawBG(0.0f, 0.0f, mFbWidth, mFbHeight, {1.0f, 0.0f, 0.0f, 0.1f});
    } else if (hp == 1) {
        DrawBG(0.0f, 0.0f, mFbWidth, mFbHeight, {1.0f, 0.0f, 0.0f, 0.2f});
    }
}

void UIRenderer::DrawJewelUI(int jewelCount)
{
    const float jewelGap = mFbWidth / 20.0f;
    DrawLinedUpTexture("default", "jewelTexture", "jewel", jewelGap, jewelCount);
}

void UIRenderer::DrawTalkableUI()
{
    DrawTextDependsOnGameController("default", "talkableText", true);
}

void UIRenderer::DrawRemainPartsUI(int remainBoatPartsCount)
{
    const auto remainPartsTextInfo = mUILoadSystem->GetTextInfo("default", "remainPartsText");
    if (!remainPartsTextInfo) {
        return;
    }

    const std::string remainText = remainPartsTextInfo->texts[0] + std::to_string(remainBoatPartsCount);
    DrawText(mFbWidth - mFbWidth * remainPartsTextInfo->xRatio, mFbWidth * remainPartsTextInfo->yRatio,
             mFbWidth * remainPartsTextInfo->scaleRatio, remainText, false);
}

void UIRenderer::DrawStateUI()
{
    if (mGame->GetSceneSystem()->IsBattleTutorialShowing()) {
        DrawBattleTutorial();
    } else if (mGame->GetSceneSystem()->IsBreakTutorialShowing()) {
        DrawBreakTutorial();
    } else if (mGame->GetSceneSystem()->IsJewelTutorialShowing()) {
        DrawJewelTutorial();
    } else if (mGame->GetSceneSystem()->IsJustDodgeTutorialShowing()) {
        DrawJustDodgeTutorial();
    }

    if (mGame->GetSceneSystem()->IsTalkWithNPC()) {
        DrawTalkWithNPC();
    }

    if (mGame->GetSceneSystem()->IsStageClear()) {
        DrawStageClear();
    }

    if (mGame->GetPlayers()[0]->GetIsTired()) {
        DrawRecommendReduceTiredUI();
    }

    const float alpha = CalculateAlpha();
    if (alpha > 0.0f) {
        DrawFadeInBg(alpha);
    }

    // ローディング描画はフェードイン背景より後に描画する必要があるため以下動かさない
    const bool isLoading =
        mGame->GetSceneSystem()->GetHasPendingStageChange() && mGame->GetSceneSystem()->GetFadeTimer() <= 0.1f;
    if (isLoading) {
        DrawLoading();
    }
}

void UIRenderer::DrawBattleTutorial()
{
    if (DrawSceneTalkUIDependsOnGameController("state", "battleTutorialText")) {
        return;
    }

    mGame->StartPlayingScene();
    mGame->GetSceneSystem()->GetUIState()->FinishTutorial();
}

void UIRenderer::DrawBreakTutorial()
{
    if (DrawSceneTalkUI("state", "breakTutorialText")) {
        return;
    }

    mGame->StartPlayingScene();
    mGame->GetSceneSystem()->GetUIState()->FinishTutorial();
}

void UIRenderer::DrawJewelTutorial()
{
    if (DrawSceneTalkUIDependsOnGameController("state", "jewelTutorialText")) {
        return;
    }

    mGame->StartPlayingScene();
    mGame->GetSceneSystem()->GetUIState()->FinishTutorial();
}

void UIRenderer::DrawJustDodgeTutorial()
{
    if (DrawSceneTalkUI("state", "justDodgeTutorialText")) {
        return;
    }

    mGame->StartPlayingScene();
    mGame->GetSceneSystem()->GetUIState()->FinishTutorial();
}

void UIRenderer::DrawTalkWithNPC()
{
    const std::vector<std::string> talkTexts = mGame->GetPlayers()[0]->GetTalkableNPC()->GetTalkTexts();
    const int talkUIIndex = mGame->GetSceneSystem()->GetTalkUIIndex();
    const bool isTalking = talkUIIndex < talkTexts.size();
    if (isTalking) {
        DrawTalkUI(talkTexts, talkUIIndex);
        return;
    }

    mGame->StartPlayingScene();
    mGame->GetSceneSystem()->GetUIState()->FinishTalkWith();
}

void UIRenderer::DrawStageClear()
{
    DrawSceneText("state", "stageClearText", true, 0);
}

void UIRenderer::DrawRecommendReduceTiredUI()
{
    DrawTextDependsOnGameController("state", "recommendReduceTiredText", false);
}

float UIRenderer::CalculateAlpha() const
{
    const float fadeInTimer = mGame->GetSceneSystem()->GetFadeTimer();
    if (fadeInTimer >= 0.0f) {
        return 1.0f - fadeInTimer;
    }
    return 1.0f + fadeInTimer;
}

void UIRenderer::DrawFadeInBg(float alpha)
{
    DrawBG(0.0f, 0.0f, mFbWidth, mFbHeight, {0.0f, 0.0f, 0.0f, alpha});
}

void UIRenderer::DrawLoading()
{
    DrawSceneText("state", "loadingText", false, 0);
    DrawSceneTexture("state", "loadingTexture", "slime");
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

    constexpr glm::vec4 black{0.0f, 0.0f, 0.0f, 255.0f};
    DrawText(mFbWidth * talkTextInfo->xRatio, mFbHeight * talkTextInfo->yRatio, mFbWidth * talkTextInfo->scaleRatio,
             texts[index], false, black);
}

void UIRenderer::DrawTalkUI(const UILoadSystem::TextInfo* textInfo)
{
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
    DrawText(mFbWidth * textInfo->xRatio, mFbHeight * textInfo->yRatio, mFbWidth * textInfo->scaleRatio,
             textInfo->texts[talkUIIndex], false, black);
}

bool UIRenderer::DrawSceneTalkUI(const std::string& sceneName, const std::string& UIName)
{
    const UILoadSystem::TextInfo* textInfo = mUILoadSystem->GetTextInfo(sceneName, UIName);
    if (!textInfo) {
        return false;
    }

    const bool isTalking = mGame->GetSceneSystem()->GetTalkUIIndex() < textInfo->texts.size();
    if (isTalking) {
        DrawTalkUI(textInfo);
        return true;
    }
    return false;
}

void UIRenderer::DrawTextDependsOnGameController(const std::string& sceneName, const std::string& UIName,
                                                 bool isCenterBase)
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

    DrawText(mFbWidth * textInfo->xRatio, mFbHeight * textInfo->yRatio, mFbWidth * textInfo->scaleRatio,
             textInfo->texts[0], isCenterBase);
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

    const bool isTalking = mGame->GetSceneSystem()->GetTalkUIIndex() < textInfo->texts.size();
    if (isTalking) {
        DrawTalkUI(textInfo);
        return true;
    }
    return false;
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
                                    const std::string& textureName, float gap, int count)
{
    const auto textureInfo = mUILoadSystem->GetTextureInfo(sceneName, UIName);
    if (!textureInfo) {
        return;
    }

    float textureX = mFbWidth * textureInfo->xRatio;
    while (count > 0) {
        DrawTexture(textureX, mFbWidth * textureInfo->yRatio, mFbWidth * textureInfo->widthRatio,
                    mFbWidth * textureInfo->heightRatio, textureName);
        textureX += gap;
        count--;
    }
}

void UIRenderer::DrawBG(float x, float y, float width, float height, std::vector<GLfloat> color)
{
    glUseProgram(mUIShader->GetShaderProgram());

    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x + width * 0.5f, y + height * 0.5f, 0.0f)) *
                      glm::scale(glm::mat4(1.0f), glm::vec3(width, height, 1.0f));
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 proj = glm::ortho(0.0f, (float)mFbWidth, (float)mFbHeight, 0.0f, -1.0f, 1.0f);

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
    const SDL_Color textColor{static_cast<Uint8>(color.x), static_cast<Uint8>(color.y), static_cast<Uint8>(color.z),
                              static_cast<Uint8>(color.w)};

    SDL_Surface* surf = TTF_RenderUTF8_Blended(mFont, message.c_str(), textColor);
    if (!surf) {
        return;
    }

    SDL_Surface* rgba = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(surf);
    if (!rgba) {
        return;
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba->w, rgba->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    const float textWidth = static_cast<float>(rgba->w) * scale;
    const float textHeight = static_cast<float>(rgba->h) * scale;
    SDL_FreeSurface(rgba);

    glm::vec3 pos;
    if (isCenterBase) {
        pos = glm::vec3(x, y + yOffset, 0.0f);
    } else {
        pos = glm::vec3(x + textWidth * 0.5f, y + textHeight * 0.5f + yOffset, 0.0f);
    }

    const glm::mat4 model =
        glm::translate(glm::mat4(1.0f), pos) * glm::scale(glm::mat4(1.0f), glm::vec3(textWidth, textHeight, 1.0f));
    const glm::mat4 view = glm::mat4(1.0f);
    const glm::mat4 proj = glm::ortho(0.0f, (float)mFbWidth, (float)mFbHeight, 0.0f, -1.0f, 1.0f);

    glUniformMatrix4fv(mUIShader->GetLocModel(), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(mUIShader->GetLocView(), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(mUIShader->GetLocProj(), 1, GL_FALSE, glm::value_ptr(proj));
    glUniform1i(mUIShader->GetLocDiffuseTexture(), 0);
    glUniform1i(mUIShader->GetLocUseTexture(), 1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);

    mVertexArrays.at("quad")->SetActive();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDeleteTextures(1, &tex);
}

void UIRenderer::DrawTexture(float x, float y, float width, float height, const std::string& textureName)
{
    glUseProgram(mUIShader->GetShaderProgram());

    const glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x + width * 0.5f, y + height * 0.5f, 0.0f)) *
                            glm::scale(glm::mat4(1.0f), glm::vec3(width, height, 1.0f));
    const glm::mat4 view = glm::mat4(1.0f);
    const glm::mat4 proj = glm::ortho(0.0f, (float)mFbWidth, (float)mFbHeight, 0.0f, -1.0f, 1.0f);

    glUniformMatrix4fv(mUIShader->GetLocModel(), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(mUIShader->GetLocView(), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(mUIShader->GetLocProj(), 1, GL_FALSE, glm::value_ptr(proj));
    glUniform1i(mUIShader->GetLocDiffuseTexture(), 0);
    glUniform1i(mUIShader->GetLocUseTexture(), 1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);

    glBindTexture(GL_TEXTURE_2D, mTextures.at(textureName));

    mVertexArrays.at("quad")->SetActive();
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}