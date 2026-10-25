#include "system/GameProgressController.h"

#include "actor/NPC.h"
#include "system/PhysicsSystem.h"
#include "system/UserDataPaths.h"
#include "world/GameWorld.h"

namespace {

constexpr int firstMainStageNumber = 1;
constexpr int lastMainStageNumber = 5;
constexpr const char* baseIntroProgressId = "cinematic:base_intro";

std::string BuildCompletedTutorialId(const std::string& tutorialId)
{
    return tutorialId.empty() ? std::string{} : "tutorial:completed:" + tutorialId;
}
} // namespace

GameProgressController::GameProgressController(GameWorld& world, PhysicsSystem& physicsSystem,
                                               const std::string& currentStageYamlPath, bool isPersistenceEnabled)
    : mWorld(world),
      mPhysicsSystem(physicsSystem),
      mCurrentStageYamlPath(currentStageYamlPath),
      mProgress(UserDataPaths::ResolveStageProgressFile(), isPersistenceEnabled)
{
}

bool GameProgressController::Load()
{
    return mProgress.Load();
}

bool GameProgressController::Save() const
{
    return mProgress.Save();
}

void GameProgressController::ResetForNewSession()
{
    mProgress.Reset();
}

bool GameProgressController::HasSelectedPlayerControlStyle() const
{
    return mProgress.HasSelectedPlayerControlStyle();
}

PlayerControlStyle GameProgressController::GetSelectedPlayerControlStyle() const
{
    return mProgress.IsAssistControlStyleSelected() ? PlayerControlStyle::Assist : PlayerControlStyle::Standard;
}

ProgressUpdateResult GameProgressController::SetSelectedPlayerControlStyle(PlayerControlStyle controlStyle)
{
    return mProgress.SetSelectedPlayerControlStyle(controlStyle == PlayerControlStyle::Assist);
}

bool GameProgressController::IsStageCleared(int stageNumber) const
{
    return mProgress.IsStageCleared(stageNumber);
}

ProgressUpdateResult GameProgressController::SetStageCleared(int stageNumber, bool isCleared)
{
    const auto result = mProgress.SetStageCleared(stageNumber, isCleared);
    if (result.changed) {
        mWorld.RefreshActorProgressVisibility();
        mPhysicsSystem.Initialize();
    }
    return result;
}

bool GameProgressController::AreAllMainStagesCleared() const
{
    for (int stageNumber = firstMainStageNumber; stageNumber <= lastMainStageNumber; ++stageNumber) {
        if (!IsStageCleared(stageNumber)) {
            return false;
        }
    }
    return true;
}

bool GameProgressController::HasCompletedTutorial(const std::string& tutorialId) const
{
    const std::string completedTutorialId = BuildCompletedTutorialId(tutorialId);
    return !completedTutorialId.empty() && HasProgressFlag(completedTutorialId);
}

ProgressUpdateResult GameProgressController::MarkTutorialCompleted(const std::string& tutorialId)
{
    return MarkProgressFlag(BuildCompletedTutorialId(tutorialId));
}

bool GameProgressController::HasSeenBaseIntro() const
{
    return HasProgressFlag(baseIntroProgressId);
}

ProgressUpdateResult GameProgressController::MarkBaseIntroSeen()
{
    return MarkProgressFlag(baseIntroProgressId);
}

bool GameProgressController::HasCompletedEndingRoll() const
{
    return mProgress.HasCompletedEndingRoll();
}

ProgressUpdateResult GameProgressController::MarkEndingRollCompleted()
{
    return mProgress.SetEndingRollCompleted();
}

bool GameProgressController::HasShownNPCConversation(const NPC* npc) const
{
    return HasProgressFlag(BuildNPCConversationId(npc));
}

ProgressUpdateResult GameProgressController::MarkNPCConversationShown(const NPC* npc)
{
    return MarkProgressFlag(BuildNPCConversationId(npc));
}

bool GameProgressController::HasCompletedNPCOpeningTrigger(const NPC* npc, std::size_t talkPageIndex) const
{
    return HasProgressFlag(BuildNPCOpeningTriggerId(npc, talkPageIndex));
}

ProgressUpdateResult GameProgressController::MarkNPCOpeningTriggerCompleted(const NPC* npc, std::size_t talkPageIndex)
{
    return MarkProgressFlag(BuildNPCOpeningTriggerId(npc, talkPageIndex));
}

bool GameProgressController::HasCompletedNPCEndingTrigger(const NPC* npc, std::size_t talkPageIndex) const
{
    return HasProgressFlag(BuildNPCEndingTriggerId(npc, talkPageIndex));
}

ProgressUpdateResult GameProgressController::MarkNPCEndingTriggerCompleted(const NPC* npc, std::size_t talkPageIndex)
{
    return MarkProgressFlag(BuildNPCEndingTriggerId(npc, talkPageIndex));
}

std::string GameProgressController::BuildNPCConversationId(const NPC* npc) const
{
    if (!npc || npc->GetStageYamlIndex() < 0) {
        return {};
    }

    return mCurrentStageYamlPath + "|" + npc->GetStageSequenceName() + ":" + std::to_string(npc->GetStageYamlIndex()) +
           "|clear:" + std::to_string(npc->ResolveTalkStageClearCondition());
}

bool GameProgressController::HasProgressFlag(const std::string& progressId) const
{
    return !progressId.empty() && mProgress.HasShownConversation(progressId);
}

ProgressUpdateResult GameProgressController::MarkProgressFlag(const std::string& progressId)
{
    return mProgress.MarkConversationShown(progressId);
}

std::string GameProgressController::BuildNPCOpeningTriggerId(const NPC* npc, std::size_t talkPageIndex) const
{
    const std::string conversationId = BuildNPCConversationId(npc);
    return conversationId.empty() ? std::string{} : conversationId + "|opening:" + std::to_string(talkPageIndex);
}

std::string GameProgressController::BuildNPCEndingTriggerId(const NPC* npc, std::size_t talkPageIndex) const
{
    const std::string conversationId = BuildNPCConversationId(npc);
    return conversationId.empty() ? std::string{} : conversationId + "|ending:" + std::to_string(talkPageIndex);
}
