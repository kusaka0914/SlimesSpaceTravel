#include "system/scene/SceneTransitionController.h"

#include "Game.h"
#include "state/GameProgressState.h"
#include "state/UIState.h"
#include "system/AudioSystem.h"

#include <SDL2/SDL_mixer.h>

SceneTransitionController::SceneTransitionController(Game* game, GameProgressState* gameProgressState, UIState* uiState,
                                                     float& fadeTimer, bool& isFadeOut, bool& hasPendingStageChange,
                                                     int& nextStageNum)
    : mGame(game),
      mGameProgressState(gameProgressState),
      mUIState(uiState),
      mFadeTimer(fadeTimer),
      mIsFadeOut(isFadeOut),
      mHasPendingStageChange(hasPendingStageChange),
      mNextStageNum(nextStageNum)
{
}

void SceneTransitionController::UpdateFade(float deltaTime)
{
    if (mFadeTimer > -1.0f) {
        mFadeTimer -= deltaTime;

        if (mFadeTimer >= 0.0f || mIsFadeOut) {
            return;
        }

        ApplySceneChange();
    } else if (mIsFadeOut) {
        mIsFadeOut = false;
    }
}

void SceneTransitionController::StartOpening()
{
    mFadeTimer = 1.0f;
    mIsFadeOut = false;
    mGameProgressState->SetNextSceneState(GameProgressState::SceneState::Opening);
}

void SceneTransitionController::StartFadeIn()
{
    mFadeTimer = 1.0f;
    mIsFadeOut = false;

    mGameProgressState->SetNextSceneState(GameProgressState::SceneState::Playing);
    mUIState->OnFadeIn();
}

void SceneTransitionController::RequestStageChange(int stageNum)
{
    mNextStageNum = stageNum;
    mHasPendingStageChange = true;

    mFadeTimer = 1.0f;
    mIsFadeOut = false;
}

void SceneTransitionController::ApplySceneChange()
{
    mIsFadeOut = true;

    const auto nextSceneState = mGameProgressState->GetNextSceneState();

    switch (nextSceneState) {
    case GameProgressState::SceneState::Opening:
        mGameProgressState->SetCurrentSceneState(GameProgressState::SceneState::Opening);
        mGameProgressState->SetNextSceneState(GameProgressState::SceneState::None);
        break;

    case GameProgressState::SceneState::Playing:
        mGameProgressState->SetCurrentSceneState(GameProgressState::SceneState::Playing);
        mGameProgressState->SetNextSceneState(GameProgressState::SceneState::None);
        mUIState->SetCurrentTalkWith(UIState::TalkWith::None);
        mGame->ChangeStage(0);
        mUIState->SetTalkUIIndex(0);
        mHasPendingStageChange = true;
        break;

    default:
        break;
    }

    mGame->GetAudioSystem()->TryChangeBGM();

    if (mHasPendingStageChange) {
        mHasPendingStageChange = false;

        mGame->ChangeStage(mNextStageNum);
        mNextStageNum = -1;

        Mix_HaltMusic();
        mGame->ReloadCurrentStage();
        mGame->StartPlayingScene();
    }
}
