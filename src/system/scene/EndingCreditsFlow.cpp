#include "system/scene/EndingCreditsFlow.h"

#include "Game.h"
#include "actor/NPC.h"
#include "state/GameProgressState.h"
#include "state/UIState.h"
#include "system/SceneSystem.h"
#include "system/ending/EndingRollConfig.h"

#include <utility>

EndingCreditsFlow::EndingCreditsFlow(
    Game& game,
    SceneSystem& sceneSystem,
    GameProgressState& gameProgressState,
    UIState& uiState)
    : mGame(game),
      mSceneSystem(sceneSystem),
      mGameProgressState(gameProgressState),
      mUIState(uiState)
{
    ReloadConfig();
}

bool EndingCreditsFlow::StartEndingAfterTalkPage(
    NPC* talkingNPC,
    std::size_t sourceTalkPageIndex)
{
    if (!talkingNPC || !mGame.AreAllMainStagesCleared() ||
        mGame.HasCompletedNPCEndingTrigger(
            talkingNPC, sourceTalkPageIndex)) {
        return false;
    }

    mGame.MarkNPCEndingTriggerCompleted(
        talkingNPC, sourceTalkPageIndex);
    mUIState.StartTalkWith(UIState::TalkWith::Ending);
    mSceneSystem.StartEnding();
    return true;
}

void EndingCreditsFlow::FinishEnding()
{
    mSceneSystem.RequestFadeAction([this]() { StartCredits(); });
}

void EndingCreditsFlow::StartCredits()
{
    ReloadConfig();
    mUIState.FinishTalkWith();
    mCreditsElapsed = 0.0f;
    mGameProgressState.SetCurrentSceneState(
        GameProgressState::SceneState::Credits);
}

void EndingCreditsFlow::FinishCredits()
{
    mSceneSystem.RequestFadeAction([this]() {
        mCreditsElapsed = 0.0f;
        mGame.MarkEndingRollCompleted();
        mUIState.FinishTalkWith();
        mGameProgressState.SetCurrentSceneState(
            GameProgressState::SceneState::Title);
    });
}

void EndingCreditsFlow::UpdateCredits(float deltaTime)
{
    if (!mSceneSystem.IsCredits() ||
        mSceneSystem.GetFadeTimer() >= 0.0f) {
        return;
    }

    mCreditsElapsed += deltaTime;
    if (mCreditsElapsed >= mConfig.totalDuration) {
        FinishCredits();
    }
}

void EndingCreditsFlow::ReloadConfig()
{
    EndingRollConfig loadedConfig;
    if (EndingRollConfigIO::Load(loadedConfig)) {
        mConfig = std::move(loadedConfig);
    }
}
