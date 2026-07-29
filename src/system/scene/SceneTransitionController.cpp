#include "system/scene/SceneTransitionController.h"

#include "Game.h"
#include "actor/Player.h"
#include "state/GameProgressState.h"
#include "state/UIState.h"
#include "system/AudioSystem.h"
#include "system/sequence/SequenceSystem.h"

#include <SDL2/SDL_mixer.h>

#include <utility>

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
    mMidpointAction = {};
    mFadeTimer = 1.0f;
    mIsFadeOut = false;
    mGameProgressState->SetNextSceneState(GameProgressState::SceneState::Opening);
}

void SceneTransitionController::StartFadeIn()
{
    mMidpointAction = {};
    mFadeTimer = 1.0f;
    mIsFadeOut = false;

    mGameProgressState->SetNextSceneState(GameProgressState::SceneState::Playing);
    mUIState->OnFadeIn();
}

void SceneTransitionController::RequestStageChange(int stageNum)
{
    mMidpointAction = {};
    mNextStageNum = stageNum;
    mHasPendingStageChange = true;

    mFadeTimer = 1.0f;
    mIsFadeOut = false;
}

bool SceneTransitionController::RequestFadeAction(std::function<void()> midpointAction)
{
    if (!midpointAction || mFadeTimer > -1.0f || mIsFadeOut || mHasPendingStageChange) {
        return false;
    }

    mMidpointAction = std::move(midpointAction);
    mFadeTimer = 1.0f;
    mIsFadeOut = false;
    return true;
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
        mUIState->SetTalkUIIndex(0);
        mNextStageNum = 0;
        mHasPendingStageChange = true;
        break;

    default:
        break;
    }

    if (mMidpointAction) {
        std::function<void()> midpointAction = std::move(mMidpointAction);
        mMidpointAction = {};
        midpointAction();
    }

    mGame->GetAudioSystem()->TryChangeBGM();

    if (mHasPendingStageChange) {
        mHasPendingStageChange = false;

        const int destinationStageNum = mNextStageNum;
        const bool shouldPlayBaseArrival = destinationStageNum == 0;

        mGame->ChangeStage(destinationStageNum);
        mNextStageNum = -1;

        Mix_HaltMusic();
        mGame->ReloadCurrentStage();
        mGame->StartPlayingScene();

        if (shouldPlayBaseArrival && mGame->GetSequenceSystem()) {
            SequenceSystem* sequenceSystem = mGame->GetSequenceSystem();
            const bool shouldPlayBaseIntro = !mHasPlayedBaseIntroThisSession;

            if (shouldPlayBaseIntro &&
                sequenceSystem->PlayCinematicChainThenSequence(
                    {"base_sequence", "base_second_sequence"},
                    "base_arrival_template")) {
                mHasPlayedBaseIntroThisSession = true;
                if (Player* player = mGame->GetMainPlayer()) {
                    player->SetIsActive(false);
                }
            } else {
                sequenceSystem->Play("base_arrival_template");
            }
        }
    }
}
