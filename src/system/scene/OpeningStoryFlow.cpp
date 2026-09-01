#include "system/scene/OpeningStoryFlow.h"

#include "Game.h"
#include "actor/NPC.h"
#include "actor/Player.h"
#include "state/UIState.h"
#include "system/SceneSystem.h"
#include "system/scene/TalkController.h"

#include <utility>
#include <vector>

OpeningStoryFlow::OpeningStoryFlow(
    Game& game,
    SceneSystem& sceneSystem,
    UIState& uiState,
    TalkController& talkController,
    std::function<void()> suppressForcedArrivalTalk)
    : mGame(game),
      mSceneSystem(sceneSystem),
      mUIState(uiState),
      mTalkController(talkController),
      mSuppressForcedArrivalTalk(
          std::move(suppressForcedArrivalTalk))
{
}

bool OpeningStoryFlow::StartAfterTalkPage(
    NPC* talkingNPC,
    Player* talkingPlayer,
    int resumeTalkPageIndex,
    std::size_t sourceTalkPageIndex)
{
    if (!talkingNPC || !talkingPlayer || resumeTalkPageIndex < 0 ||
        mGame.HasCompletedNPCOpeningTrigger(
            talkingNPC, sourceTalkPageIndex)) {
        return false;
    }

    mReturnStageNum = mGame.GetCurrentStageNum();
    mReturnStageYamlPath = mGame.GetCurrentStageYamlPath();
    mResumeNPCConversationId =
        mGame.GetNPCConversationId(talkingNPC);
    mResumePlayerIndex = 0;
    const std::vector<Player*>& players = mGame.GetPlayers();
    for (std::size_t index = 0; index < players.size(); ++index) {
        if (players[index] == talkingPlayer) {
            mResumePlayerIndex = static_cast<int>(index);
            break;
        }
    }

    mGame.MarkNPCOpeningTriggerCompleted(
        talkingNPC, sourceTalkPageIndex);
    mResumeTalkPageIndex = resumeTalkPageIndex;
    mHasResume = true;
    mIsFinishing = false;
    mUIState.StartTalkWith(UIState::TalkWith::Opening);
    mSceneSystem.StartOpening();
    return true;
}

void OpeningStoryFlow::Finish()
{
    if (mIsFinishing) {
        return;
    }
    mIsFinishing = true;

    if (!mHasResume || mReturnStageNum < 0 ||
        mReturnStageYamlPath.empty() ||
        mResumeNPCConversationId.empty() ||
        mResumeTalkPageIndex < 0) {
        mIsFinishing = false;
        mSceneSystem.StartFadeIn();
        return;
    }

    const int resumeTalkPageIndex = mResumeTalkPageIndex;
    const int returnStageNum = mReturnStageNum;
    const int resumePlayerIndex = mResumePlayerIndex;
    const std::string returnStageYamlPath = mReturnStageYamlPath;
    const std::string resumeNPCConversationId =
        mResumeNPCConversationId;
    ClearResume();
    if (mSuppressForcedArrivalTalk) {
        mSuppressForcedArrivalTalk();
    }

    const bool requested = mSceneSystem.RequestFadeAction(
        [this, returnStageNum, returnStageYamlPath,
         resumeNPCConversationId, resumePlayerIndex,
         resumeTalkPageIndex]() {
            const bool isAlreadyOnReturnStage =
                mGame.GetCurrentStageNum() == returnStageNum &&
                mGame.GetCurrentStageYamlPath() == returnStageYamlPath;
            if (!isAlreadyOnReturnStage &&
                !mGame.LoadStageForScene(
                    returnStageNum, returnStageYamlPath)) {
                mIsFinishing = false;
                mSceneSystem.StartPlayingScene();
                return;
            }

            NPC* resumeNPC =
                mGame.FindNPCByConversationId(resumeNPCConversationId);
            const std::vector<Player*>& players = mGame.GetPlayers();
            Player* resumePlayer =
                resumePlayerIndex >= 0 &&
                        resumePlayerIndex < static_cast<int>(players.size())
                    ? players[resumePlayerIndex]
                    : mGame.GetMainPlayer();
            if (!resumeNPC || !resumePlayer ||
                !resumeNPC->GetIsActive() ||
                !resumePlayer->GetIsActive()) {
                mIsFinishing = false;
                mSceneSystem.StartPlayingScene();
                return;
            }

            mIsFinishing = false;
            mTalkController.ResumeTalkWithNPC(
                resumeNPC, resumePlayer, resumeTalkPageIndex);
        });
    if (!requested) {
        mIsFinishing = false;
    }
}

void OpeningStoryFlow::Reset()
{
    ClearResume();
    mIsFinishing = false;
}

void OpeningStoryFlow::ClearResume()
{
    mResumeTalkPageIndex = -1;
    mReturnStageNum = -1;
    mResumePlayerIndex = 0;
    mReturnStageYamlPath.clear();
    mResumeNPCConversationId.clear();
    mHasResume = false;
}
