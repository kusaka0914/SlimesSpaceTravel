#include "system/GameSessionController.h"

#include "actor/Player.h"
#include "system/AudioSystem.h"
#include "system/GameProgressController.h"
#include "system/GameWorld.h"
#include "system/InputSystem.h"
#include "system/PauseMenuController.h"
#include "system/PlayerConfigurationController.h"
#include "system/SceneSystem.h"
#include "system/UGCModeController.h"
#include "system/UserDataPaths.h"
#include "system/sequence/SequenceSystem.h"
#include "system/tgs/TgsExperienceController.h"

#include <GLFW/glfw3.h>

namespace {

std::string ResolveControlStyleLogName(PlayerControlStyle controlStyle)
{
    return controlStyle == PlayerControlStyle::Assist ? "assist" : "standard";
}

} // namespace

GameSessionController::GameSessionController(const GameSessionDependencies& dependencies, bool isTgsBuild)
    : mWorld(dependencies.world),
      mSceneSystem(dependencies.sceneSystem),
      mSequenceSystem(dependencies.sequenceSystem),
      mPlayerConfigurationController(dependencies.playerConfigurationController),
      mProgressController(dependencies.progressController),
      mPauseMenuController(dependencies.pauseMenuController),
      mInputSystem(dependencies.inputSystem),
      mAudioSystem(dependencies.audioSystem),
      mUGCModeController(dependencies.ugcModeController),
      mPlayerControlStyle(dependencies.playerControlStyle)
{
    if (isTgsBuild) {
        mTgsExperienceController = std::make_unique<TgsExperienceController>(
            TgsExperienceConfig{}, UserDataPaths::ResolveTgsPlayLogDirectory());
    }
}

GameSessionController::~GameSessionController() = default;

void GameSessionController::ProcessInput()
{
    const bool isReturnConfirmPressed = mInputSystem.IsKeyPressed(GLFW_KEY_ENTER) ||
                                        mInputSystem.IsKeyPressed(GLFW_KEY_KP_ENTER) ||
                                        mInputSystem.IsKeyPressed(GLFW_KEY_SPACE);
    const bool canReturnFromThankYou =
        IsTgsThankYouScreenVisible() && mSceneSystem.GetFadeTimer() <= -1.0f && !mSceneSystem.IsFadingOut();
    if (canReturnFromThankYou && isReturnConfirmPressed && !mWasTgsReturnConfirmPressed) {
        mTgsExperienceController->RequestReturnToTitle();
    }
    mWasTgsReturnConfirmPressed = isReturnConfirmPressed;
}

void GameSessionController::Update(float elapsedSeconds)
{
    if (!mTgsExperienceController) {
        return;
    }

    if (mTgsExperienceController->GetPhase() == TgsExperienceController::Phase::WaitingForSession &&
        mSceneSystem.IsPlaying() && mSceneSystem.GetFadeTimer() <= -1.0f && !mSceneSystem.IsFadingOut()) {
        mTgsExperienceController->StartSession(mPlayerConfigurationController.IsSecondPlayerJoined() ? 2 : 1,
                                               ResolveControlStyleLogName(mPlayerControlStyle));
    }

    const Player* mainPlayer = mPlayerConfigurationController.GetMainPlayer();
    const int planetNumber = mainPlayer ? mainPlayer->GetCurrentPlanetNum() : -1;
    if (mTgsExperienceController->IsPlaying()) {
        mTgsExperienceController->UpdateProgress(mWorld.GetCurrentStageNum(), planetNumber,
                                                 mPlayerConfigurationController.IsSecondPlayerJoined() ? 2 : 1,
                                                 ResolveControlStyleLogName(mPlayerControlStyle));
    }
    const bool isSceneTransitionActive = mSceneSystem.GetFadeTimer() > -1.0f || mSceneSystem.IsFadingOut();
    const bool shouldPauseThankYouTimer =
        mTgsExperienceController->IsThankYouScreenVisible() && isSceneTransitionActive;
    if (!shouldPauseThankYouTimer) {
        mTgsExperienceController->Update(elapsedSeconds);
    }

    if (mTgsExperienceController->IsEndingSession()) {
        mTgsExperienceController->SaveSessionLog();
        mPauseMenuController.Close();
        mSceneSystem.RequestFadeAction([this]() { mTgsExperienceController->ShowThankYouScreen(); });
        return;
    }

    if (mTgsExperienceController->ShouldReturnToTitle()) {
        mSceneSystem.RequestFadeAction([this]() { EnterTitleAtFadeMidpoint(); });
    }
}

void GameSessionController::MoveTitleMenuSelection(int delta)
{
    if (!mSceneSystem.IsTitle() || mSceneSystem.IsTransitionActive() || delta == 0) {
        return;
    }

    constexpr int titleMenuItemCount = 3;
    mTitleMenuSelection = (mTitleMenuSelection + delta + titleMenuItemCount) % titleMenuItemCount;
}

void GameSessionController::ExecuteTitleMenuSelection()
{
    if (!mSceneSystem.IsTitle() || mSceneSystem.IsTransitionActive()) {
        return;
    }

    switch (mTitleMenuSelection) {
    case 0:
        mSceneSystem.StartBattleStyleSelection();
        break;
    case 1:
        mSceneSystem.RequestFadeAction([this]() { mUGCModeController.StartModeFromTitleSelection(); });
        break;
    case 2:
        mUGCModeController.OpenWorkBrowser();
        break;
    default:
        break;
    }
}

void GameSessionController::ResetTitleMenuSelection()
{
    mTitleMenuSelection = 0;
}

void GameSessionController::ReturnToTitle()
{
    mPauseMenuController.Close();
    if (mTgsExperienceController) {
        mTgsExperienceController->SaveSessionLogBeforeTitleReturn();
    }
    mProgressController.Save();

    if (mUGCModeController.IsModeActive()) {
        mUGCModeController.ExitMode();
        return;
    }

    mSceneSystem.RequestFadeAction([this]() { EnterTitleAtFadeMidpoint(); });
}

void GameSessionController::EnterTitleAtFadeMidpoint()
{
    if (mTgsExperienceController) {
        mPauseMenuController.Close();
    }
    mSequenceSystem.Stop(true);
    mPlayerConfigurationController.ReturnToSinglePlayer();
    mPlayerConfigurationController.Reset();
    if (mTgsExperienceController) {
        mProgressController.ResetForNewSession();
        mSceneSystem.ResetTutorialsForNewSession();
        mPlayerControlStyle = PlayerControlStyle::Standard;
    }

    ResetTitleMenuSelection();
    mSceneSystem.EnterTitleAtFadeMidpoint();
    if (mTgsExperienceController) {
        mTgsExperienceController->ResetForNextSession();
    } else {
        mAudioSystem.TryChangeBGM();
    }
    mInputSystem.SuppressOneShotInputUntilReleased();
}

void GameSessionController::RecordPlayerDeath()
{
    if (mTgsExperienceController) {
        mTgsExperienceController->RecordPlayerDeath();
    }
}

void GameSessionController::RecordStageCleared(int stageNum)
{
    if (mTgsExperienceController) {
        mTgsExperienceController->RecordStageCleared(stageNum);
    }
}

bool GameSessionController::IsTgsThankYouScreenVisible() const
{
    return mTgsExperienceController && mTgsExperienceController->IsThankYouScreenVisible();
}

bool GameSessionController::IsTgsRemainingTimeNoticeVisible() const
{
    return mTgsExperienceController && mTgsExperienceController->IsRemainingTimeNoticeVisible();
}

bool GameSessionController::ShouldBlockInputForTgsPresentation() const
{
    return mTgsExperienceController && mTgsExperienceController->ShouldBlockGameInput();
}

void GameSessionController::SkipTgsExperienceToRemainingTimeNotice()
{
    if (mTgsExperienceController) {
        mTgsExperienceController->SkipToRemainingTimeNotice();
    }
}

void GameSessionController::EndTgsExperienceNow()
{
    if (mTgsExperienceController) {
        mTgsExperienceController->EndSessionNow();
    }
}
