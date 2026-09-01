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
        if (mFadeCompletionAction) {
            std::function<void()> completionAction =
                std::move(mFadeCompletionAction);
            mFadeCompletionAction = {};
            completionAction();
        }
    }
}

void SceneTransitionController::StartOpening()
{
    mMidpointAction = {};
    mFadeCompletionAction = {};
    mFadeTimer = 1.0f;
    mIsFadeOut = false;
    mGameProgressState->SetNextSceneState(GameProgressState::SceneState::Opening);
}

void SceneTransitionController::StartEnding()
{
    mMidpointAction = {};
    mFadeCompletionAction = {};
    mFadeTimer = 1.0f;
    mIsFadeOut = false;
    mGameProgressState->SetNextSceneState(GameProgressState::SceneState::Ending);
}

void SceneTransitionController::StartFadeIn()
{
    mMidpointAction = {};
    mFadeCompletionAction = {};
    mFadeTimer = 1.0f;
    mIsFadeOut = false;

    mGameProgressState->SetNextSceneState(GameProgressState::SceneState::Playing);
    mUIState->OnFadeIn();
}

void SceneTransitionController::StartBattleStyleSelection()
{
    mMidpointAction = {};
    mFadeCompletionAction = {};
    mFadeTimer = 1.0f;
    mIsFadeOut = false;
    mGameProgressState->SetNextSceneState(
        GameProgressState::SceneState::BattleStyleSelection);
}

void SceneTransitionController::CancelPendingTransition()
{
    mMidpointAction = {};
    mFadeCompletionAction = {};
    mFadeTimer = -1.0f;
    mIsFadeOut = false;
    mHasPendingStageChange = false;
    mNextStageNum = -1;
    mGameProgressState->SetNextSceneState(
        GameProgressState::SceneState::None);
}

void SceneTransitionController::RequestStageChange(int stageNum)
{
    mMidpointAction = {};
    mFadeCompletionAction = {};
    mNextStageNum = stageNum;
    mHasPendingStageChange = true;

    mFadeTimer = 1.0f;
    mIsFadeOut = false;
}

bool SceneTransitionController::RequestFadeAction(
    std::function<void()> midpointAction,
    std::function<void()> completionAction)
{
    if (!midpointAction || mFadeTimer > -1.0f || mIsFadeOut || mHasPendingStageChange) {
        return false;
    }

    mMidpointAction = std::move(midpointAction);
    mFadeCompletionAction = std::move(completionAction);
    mFadeTimer = 1.0f;
    mIsFadeOut = false;
    return true;
}

void SceneTransitionController::ApplySceneChange()
{
    mIsFadeOut = true;

    const auto nextSceneState = mGameProgressState->GetNextSceneState();

    switch (nextSceneState) {
    case GameProgressState::SceneState::BattleStyleSelection:
        mGameProgressState->SetCurrentSceneState(
            GameProgressState::SceneState::BattleStyleSelection);
        mGameProgressState->SetNextSceneState(
            GameProgressState::SceneState::None);
        break;

    case GameProgressState::SceneState::Opening:
        mGameProgressState->SetCurrentSceneState(GameProgressState::SceneState::Opening);
        mGameProgressState->SetNextSceneState(GameProgressState::SceneState::None);
        break;

    case GameProgressState::SceneState::Ending:
        mGameProgressState->SetCurrentSceneState(GameProgressState::SceneState::Ending);
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
        const bool shouldPlayStageIntro =
            mGame->HasStageIntroCinematic(destinationStageNum);
        const bool shouldPlayBaseIntro =
            shouldPlayBaseArrival &&
            !mGame->HasSeenBaseIntro();
        const bool shouldDeferStageMusic =
            shouldPlayStageIntro;

        if (shouldDeferStageMusic) {
            mGame->GetAudioSystem()->BeginStageMusicDeferral();
        }

        mGame->ChangeStage(destinationStageNum);
        mNextStageNum = -1;

        Mix_HaltMusic();
        mGame->ReloadCurrentStage();
        mGame->StartPlayingScene();

        // ReloadCurrentStage中は、遷移前のタイトル状態でBGM判定が
        // 行われる。Playingへ切り替えた後にも判定し直さないと、
        // 操作スタイル選択を省略して拠点へ入った場合にタイトルBGMが
        // そのまま残ってしまう。
        mGame->GetAudioSystem()->TryChangeBGM();

        if (shouldPlayBaseArrival && mGame->GetSequenceSystem()) {
            SequenceSystem* sequenceSystem = mGame->GetSequenceSystem();
            if (shouldPlayBaseIntro &&
                sequenceSystem->PlayCinematicChainThenSequence(
                    {"base_sequence"},
                    "base_arrival_template")) {
                mGame->MarkBaseIntroSeen();
                if (Player* player = mGame->GetMainPlayer()) {
                    player->SetIsActive(false);
                }
            } else {
                sequenceSystem->Play("base_arrival_template");
            }
        } else if (shouldPlayStageIntro) {
            mGame->StartStageIntroCinematic(destinationStageNum);
        }

        if (shouldDeferStageMusic &&
            (!mGame->GetSequenceSystem() ||
             !mGame->GetSequenceSystem()->IsCinematicChainPlaying())) {
            mGame->GetAudioSystem()->ResumeDeferredStageMusic();
        }
    }
}
