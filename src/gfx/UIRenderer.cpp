#include "UIRenderer.h"

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
    Shutdown();
}

void UIRenderer::Shutdown()
{
    ClearTextTextureCache();

    if (!mIsImGuiInitialized) {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    mIsImGuiInitialized = false;
}

void UIRenderer::Initialize()
{
    mUIShaderUnique = std::make_unique<UIShader>();
    mUIShader = mUIShaderUnique.get();

    mUILoadSystemUnique = std::make_unique<UILoadSystem>();
    mUILoadSystem = mUILoadSystemUnique.get();

    mDebugEditorBridge =
        std::make_unique<UIDebugEditorBridge>(mGame, this);
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
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    io.Fonts->AddFontFromFileTTF("../assets/fonts/NotoSansJP-Black.ttf", 18.0f, nullptr,
                                 io.Fonts->GetGlyphRangesJapanese());

    const char* glslVersion = "#version 330";

    ImGui_ImplGlfw_InitForOpenGL(mGame->GetWindow(), true);
    ImGui_ImplOpenGL3_Init(glslVersion);
    mIsImGuiInitialized = true;
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

void UIRenderer::DrawGameContent()
{
    glfwGetFramebufferSize(mGame->GetWindow(), &mFbWidth, &mFbHeight);
    mRenderedUIElements.clear();
    glViewport(0, 0, mFbWidth, mFbHeight);
    glUseProgram(mUIShader->GetShaderProgram());

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    SceneSystem* sceneSystem = mGame->GetSceneSystem();
    SequenceSystem* sequenceSystem = mGame->GetSequenceSystem();
    const bool isStartCinematicPlaying =
        sequenceSystem &&
        sequenceSystem->IsCinematicChainPlaying();

    if (!isStartCinematicPlaying && sceneSystem->IsTitle()) {
        mSceneUIRenderer->DrawTitle();
    }

    if (!isStartCinematicPlaying && sceneSystem->IsOpening()) {
        mSceneUIRenderer->DrawOpening();
    }

    if (!isStartCinematicPlaying && sceneSystem->IsEnding()) {
        mSceneUIRenderer->DrawEnding();
    }

    if (!isStartCinematicPlaying && sceneSystem->IsCredits()) {
        mSceneUIRenderer->DrawCredits();
    }

    if (!isStartCinematicPlaying && sceneSystem->IsGameOver()) {
        mSceneUIRenderer->DrawGameOver();
    }




    const bool isUGCEditing =
        mGame->GetIsUGCMode() &&
        mGame->GetIsDebugEditorShowing();
    const bool isUGCPlaytestActive =
        mGame->GetIsUGCPlaytestActive();
    const bool shouldDrawDefaultUI =
        !isStartCinematicPlaying &&
        !isUGCEditing &&
        !isUGCPlaytestActive &&
        (sceneSystem->IsPlaying() ||
         sceneSystem->IsTutorialActive("jewel_usage"));
    mHudRenderer->UpdateTalkableUIVisibility(
        mGame->GetPlayers(),
        shouldDrawDefaultUI && sceneSystem->IsPlaying());
    if (shouldDrawDefaultUI) {
        mHudRenderer->DrawDefaultUI();
    } else if (!isStartCinematicPlaying && isUGCPlaytestActive) {
        mHudRenderer->DrawUGCPlaytestUI();
    }

    if (!isStartCinematicPlaying && !isUGCPlaytestActive) {
        mStateUIRenderer->DrawStateUI();
    }

    if (!isStartCinematicPlaying) {
        DrawCustomUI();
    }

    if (!isUGCPlaytestActive && !isStartCinematicPlaying &&
        mGame->GetIsPauseMenuOpen()) {
        mPauseMenuRenderer->Draw();
    }



    mStateUIRenderer->DrawTransitionUI();

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

void UIRenderer::DrawDebugEditor(
    GLuint gameViewTexture,
    int gameViewWidth,
    int gameViewHeight)
{
    if (!mGame->GetIsDebugEditorShowing()) {
        return;
    }

    glfwGetFramebufferSize(mGame->GetWindow(), &mFbWidth, &mFbHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, mFbWidth, mFbHeight);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (mGame->GetIsUGCMode() &&
        !mGame->GetIsUGCDebugEditorShowing()) {
        mDebugEditorBridge->DrawEditor(
            gameViewTexture,
            gameViewWidth,
            gameViewHeight,
            true);
    } else {
        mDebugEditorBridge->DrawEditor(
            gameViewTexture,
            gameViewWidth,
            gameViewHeight,
            false);
    }
    EndImGuiFrame();




    mStateUIRenderer->DrawTransitionUI();

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

void UIRenderer::DrawUGCWorkBrowser()
{
    glfwGetFramebufferSize(mGame->GetWindow(), &mFbWidth, &mFbHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, mFbWidth, mFbHeight);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    mDebugEditorBridge->DrawWorkBrowser();
    EndImGuiFrame();

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

bool UIRenderer::CompleteUGCVerification(
    const std::string& workFileName)
{
    return mDebugEditorBridge &&
        mDebugEditorBridge->CompleteUGCVerification(workFileName);
}

void UIRenderer::UndoUGCEdit()
{
    if (mDebugEditorBridge) mDebugEditorBridge->HandleUndo();
}

void UIRenderer::RedoUGCEdit()
{
    if (mDebugEditorBridge) mDebugEditorBridge->HandleRedo();
}

void UIRenderer::ToggleUGCEraser()
{
    if (mDebugEditorBridge) mDebugEditorBridge->ToggleEraser();
}

void UIRenderer::SelectUGCEditorMode()
{
    if (mDebugEditorBridge) mDebugEditorBridge->ActivateSelectionMode();
}

void UIRenderer::OpenUGCEditorMenu()
{
    if (mDebugEditorBridge) mDebugEditorBridge->OpenEditorMenu();
}

void UIRenderer::ZoomUGCEditor(float distanceMultiplier)
{
    if (mDebugEditorBridge) {
        mDebugEditorBridge->AdjustZoom(distanceMultiplier);
    }
}

void UIRenderer::ChangeUGCEditLayer(int layerDelta)
{
    if (mDebugEditorBridge) {
        mDebugEditorBridge->ChangeLayer(layerDelta);
    }
}

void UIRenderer::MoveUGCSelectionByGrid(int gridX, int gridZ)
{
    if (mDebugEditorBridge) {
        mDebugEditorBridge->MoveSelectionOnGrid(gridX, gridZ);
    }
}

void UIRenderer::NotifyUGCEditorTutorialReturnedFromPlaytest()
{
    if (mDebugEditorBridge) {
        mDebugEditorBridge->NotifyTutorialReturnedFromPlaytest();
    }
}

bool UIRenderer::SaveDebugEditorSession(
    const std::string& filePath,
    std::string& outErrorMessage)
{
    if (!mDebugEditorBridge) {
        outErrorMessage = "The debug editor is not initialized.";
        return false;
    }

    return mDebugEditorBridge->SaveSession(
        filePath,
        outErrorMessage);
}

bool UIRenderer::RestoreDebugEditorSession(
    const std::string& filePath,
    std::string& outErrorMessage)
{
    if (!mDebugEditorBridge) {
        outErrorMessage = "The debug editor is not initialized.";
        return false;
    }

    return mDebugEditorBridge->RestoreSession(
        filePath,
        outErrorMessage);
}

void UIRenderer::SetEditorRestartStatus(
    const std::string& message,
    bool isError)
{
    if (mDebugEditorBridge) {
        mDebugEditorBridge->SetBuildRestartStatus(message, isError);
    }
}

void UIRenderer::EndImGuiFrame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
