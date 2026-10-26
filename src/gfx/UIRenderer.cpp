#include "gfx/UIRenderer.h"

#include "app/Game.h"
#include "gfx/UIShader.h"
#include "gfx/VertexArray.h"
#include "gfx/ui/HudRenderer.h"
#include "gfx/ui/PauseMenuRenderer.h"
#include "gfx/ui/SceneUIRenderer.h"
#include "gfx/ui/StateUIRenderer.h"
#include "system/SceneSystem.h"
#include "system/sequence/SequenceSystem.h"
#include <GLFW/glfw3.h>

UIRenderer::UIRenderer(Game* game)
    : mGame(game),
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
    if (mTextRenderer) {
        mTextRenderer->Shutdown();
    }
}

void UIRenderer::Initialize()
{
    if (!mIsInitialized) {
        return;
    }

    mUIShaderUnique = std::make_unique<UIShader>();
    mUIShader = mUIShaderUnique.get();

    if (!mUIShader->GetShaderProgram()) {
        // GameがGLリソースを解放してから終了できるよう、ここではGLFWを終了しない。
        mIsInitialized = false;
        return;
    }

    mTextRenderer = std::make_unique<UITextRenderer>(mFont, *mUIShader, *mVertexArrays.at("quad"));

    mUILoadSystemUnique = std::make_unique<UILoadSystem>();
    mUILoadSystem = mUILoadSystemUnique.get();

    mSceneUIRenderer = std::make_unique<SceneUIRenderer>(mGame, this);
    mHudRenderer = std::make_unique<HudRenderer>(mGame, this);
    mStateUIRenderer = std::make_unique<StateUIRenderer>(mGame, this);
    mPauseMenuRenderer = std::make_unique<PauseMenuRenderer>(mGame, this);

    RegisterUITextures();
    RegisterCustomUITextures();
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
    RegisterTexture(basePath + "guard.png", "guard");
    RegisterTexture(basePath + "tgs/QR_540300.png", "tgsQrCode");
}

void UIRenderer::DrawGameContent()
{
    glfwGetFramebufferSize(mGame->GetWindow(), &mFbWidth, &mFbHeight);
    mRenderedUIElements.clear();
    if (mGame->GetIsGameUIHidden()) {
        return;
    }

    glViewport(0, 0, mFbWidth, mFbHeight);
    glUseProgram(mUIShader->GetShaderProgram());

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    SceneSystem* sceneSystem = mGame->GetSceneSystem();
    SequenceSystem* sequenceSystem = mGame->GetSequenceSystem();
    const bool isStartCinematicPlaying = sequenceSystem && sequenceSystem->IsCinematicChainPlaying();

    if (mGame->IsTgsThankYouScreenVisible()) {
        mSceneUIRenderer->DrawTgsThankYou();
        mStateUIRenderer->DrawTransitionUI();
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        return;
    }

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

    const bool isUGCEditing = mGame->GetIsUGCMode() && mGame->GetIsDebugEditorShowing();
    const bool isUGCPlaytestActive = mGame->GetIsUGCPlaytestActive();
    const bool isUGCClearResultShowing = mGame->GetIsUGCClearResultShowing();
    const bool shouldDrawDefaultUI = !isStartCinematicPlaying && !isUGCEditing && !isUGCPlaytestActive &&
                                     (sceneSystem->IsPlaying() || sceneSystem->IsTutorialActive("jewel_usage"));
    mHudRenderer->UpdateTalkableUIVisibility(mGame->GetPlayers(), shouldDrawDefaultUI && sceneSystem->IsPlaying());
    if (shouldDrawDefaultUI) {
        mHudRenderer->DrawDefaultUI();
    } else if (!isStartCinematicPlaying && isUGCClearResultShowing) {
        mSceneUIRenderer->DrawUGCClearResult();
    } else if (!isStartCinematicPlaying && isUGCPlaytestActive && !sceneSystem->IsStageClear()) {
        mHudRenderer->DrawUGCPlaytestUI();
    }

    const bool shouldDrawUGCStageClear = isUGCPlaytestActive && sceneSystem->IsStageClear() && !isUGCClearResultShowing;
    if (!isStartCinematicPlaying && (!isUGCPlaytestActive || shouldDrawUGCStageClear)) {
        mStateUIRenderer->DrawStateUI();
    }

    if (!isStartCinematicPlaying) {
        DrawCustomUI();
        DrawContextualOperationGuide();
    }

    if (!isUGCPlaytestActive && !isStartCinematicPlaying && mGame->GetIsPauseMenuOpen()) {
        mPauseMenuRenderer->Draw();
        if (!mGame->GetIsUGCMode()) {
            DrawCustomUI(true);
        }
    }

    if (mGame->IsTgsRemainingTimeNoticeVisible()) {
        mSceneUIRenderer->DrawTgsRemainingTimeNotice();
    }

    mStateUIRenderer->DrawTransitionUI();

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

void UIRenderer::DrawTransitionOverlay()
{
    mStateUIRenderer->DrawTransitionUI();
}
