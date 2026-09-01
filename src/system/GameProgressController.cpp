#include "system/GameProgressController.h"

#include "actor/NPC.h"
#include "system/GameWorld.h"
#include "system/PhysicsSystem.h"

namespace {
constexpr int firstMainStageNumber = 1;
constexpr int lastMainStageNumber = 5;
constexpr const char* baseIntroProgressId = "cinematic:base_intro";

std::string BuildCompletedTutorialId(const std::string& tutorialId)
{
    return tutorialId.empty()
        ? std::string{}
        : "tutorial:completed:" + tutorialId;
}
}

GameProgressController::GameProgressController(
    GameWorld& world,
    PhysicsSystem& physicsSystem,
    const std::string& currentStageYamlPath)
    : mWorld(world),
      mPhysicsSystem(physicsSystem),
      mCurrentStageYamlPath(currentStageYamlPath)
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

bool GameProgressController::HasSelectedPlayerControlStyle() const
{
    return mProgress.HasSelectedPlayerControlStyle();
}

PlayerControlStyle
GameProgressController::GetSelectedPlayerControlStyle() const
{
    return mProgress.IsAssistControlStyleSelected()
        ? PlayerControlStyle::Assist
        : PlayerControlStyle::Standard;
}

void GameProgressController::SetSelectedPlayerControlStyle(
    PlayerControlStyle controlStyle)
{
    mProgress.SetSelectedPlayerControlStyle(
        controlStyle == PlayerControlStyle::Assist);
}

bool GameProgressController::IsStageCleared(int stageNumber) const
{
    return mProgress.IsStageCleared(stageNumber);
}

void GameProgressController::SetStageCleared(
    int stageNumber,
    bool isCleared)
{
    const bool didProgressChange =
        mProgress.SetStageCleared(stageNumber, isCleared);
    if (didProgressChange) {
        mWorld.RefreshActorProgressVisibility();
        mPhysicsSystem.Initialize();
    }
}

bool GameProgressController::AreAllMainStagesCleared() const
{
    for (int stageNumber = firstMainStageNumber;
         stageNumber <= lastMainStageNumber;
         ++stageNumber) {
        if (!IsStageCleared(stageNumber)) {
            return false;
        }
    }
    return true;
}

bool GameProgressController::HasCompletedTutorial(
    const std::string& tutorialId) const
{
    const std::string completedTutorialId =
        BuildCompletedTutorialId(tutorialId);
    return !completedTutorialId.empty() &&
           HasProgressFlag(completedTutorialId);
}

void GameProgressController::MarkTutorialCompleted(
    const std::string& tutorialId)
{
    MarkProgressFlag(BuildCompletedTutorialId(tutorialId));
}

bool GameProgressController::HasSeenBaseIntro() const
{
    return HasProgressFlag(baseIntroProgressId);
}

void GameProgressController::MarkBaseIntroSeen()
{
    MarkProgressFlag(baseIntroProgressId);
}

bool GameProgressController::HasCompletedEndingRoll() const
{
    return mProgress.HasCompletedEndingRoll();
}

void GameProgressController::MarkEndingRollCompleted()
{
    mProgress.SetEndingRollCompleted();
}

bool GameProgressController::HasShownNPCConversation(const NPC* npc) const
{
    return HasProgressFlag(BuildNPCConversationId(npc));
}

void GameProgressController::MarkNPCConversationShown(const NPC* npc)
{
    MarkProgressFlag(BuildNPCConversationId(npc));
}

bool GameProgressController::HasCompletedNPCOpeningTrigger(
    const NPC* npc,
    std::size_t talkPageIndex) const
{
    return HasProgressFlag(
        BuildNPCOpeningTriggerId(npc, talkPageIndex));
}

void GameProgressController::MarkNPCOpeningTriggerCompleted(
    const NPC* npc,
    std::size_t talkPageIndex)
{
    MarkProgressFlag(
        BuildNPCOpeningTriggerId(npc, talkPageIndex));
}

bool GameProgressController::HasCompletedNPCEndingTrigger(
    const NPC* npc,
    std::size_t talkPageIndex) const
{
    return HasProgressFlag(
        BuildNPCEndingTriggerId(npc, talkPageIndex));
}

void GameProgressController::MarkNPCEndingTriggerCompleted(
    const NPC* npc,
    std::size_t talkPageIndex)
{
    MarkProgressFlag(
        BuildNPCEndingTriggerId(npc, talkPageIndex));
}

std::string GameProgressController::BuildNPCConversationId(
    const NPC* npc) const
{
    if (!npc || npc->GetStageYamlIndex() < 0) {
        return {};
    }

    return mCurrentStageYamlPath + "|" +
           npc->GetStageSequenceName() + ":" +
           std::to_string(npc->GetStageYamlIndex()) +
           "|clear:" +
           std::to_string(npc->ResolveTalkStageClearCondition());
}

bool GameProgressController::HasProgressFlag(
    const std::string& progressId) const
{
    return !progressId.empty() &&
           mProgress.HasShownConversation(progressId);
}

void GameProgressController::MarkProgressFlag(
    const std::string& progressId)
{
    if (!progressId.empty()) {
        mProgress.MarkConversationShown(progressId);
    }
}

std::string GameProgressController::BuildNPCOpeningTriggerId(
    const NPC* npc,
    std::size_t talkPageIndex) const
{
    const std::string conversationId = BuildNPCConversationId(npc);
    return conversationId.empty()
        ? std::string{}
        : conversationId + "|opening:" +
              std::to_string(talkPageIndex);
}

std::string GameProgressController::BuildNPCEndingTriggerId(
    const NPC* npc,
    std::size_t talkPageIndex) const
{
    const std::string conversationId = BuildNPCConversationId(npc);
    return conversationId.empty()
        ? std::string{}
        : conversationId + "|ending:" +
              std::to_string(talkPageIndex);
}
