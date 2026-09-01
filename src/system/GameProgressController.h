#pragma once

#include "actor/player/PlayerTypes.h"
#include "system/StageProgressSystem.h"

#include <cstddef>
#include <string>

class GameWorld;
class NPC;
class PhysicsSystem;

class GameProgressController {
public:
    GameProgressController(
        GameWorld& world,
        PhysicsSystem& physicsSystem,
        const std::string& currentStageYamlPath);

    bool Load();
    bool Save() const;

    bool HasSelectedPlayerControlStyle() const;
    PlayerControlStyle GetSelectedPlayerControlStyle() const;
    void SetSelectedPlayerControlStyle(PlayerControlStyle controlStyle);

    bool IsStageCleared(int stageNumber) const;
    void SetStageCleared(int stageNumber, bool isCleared);
    bool AreAllMainStagesCleared() const;

    bool HasCompletedTutorial(const std::string& tutorialId) const;
    void MarkTutorialCompleted(const std::string& tutorialId);
    bool HasSeenBaseIntro() const;
    void MarkBaseIntroSeen();
    bool HasCompletedEndingRoll() const;
    void MarkEndingRollCompleted();

    bool HasShownNPCConversation(const NPC* npc) const;
    void MarkNPCConversationShown(const NPC* npc);
    bool HasCompletedNPCOpeningTrigger(
        const NPC* npc,
        std::size_t talkPageIndex) const;
    void MarkNPCOpeningTriggerCompleted(
        const NPC* npc,
        std::size_t talkPageIndex);
    bool HasCompletedNPCEndingTrigger(
        const NPC* npc,
        std::size_t talkPageIndex) const;
    void MarkNPCEndingTriggerCompleted(
        const NPC* npc,
        std::size_t talkPageIndex);
    std::string BuildNPCConversationId(const NPC* npc) const;

    bool HasProgressFlag(const std::string& progressId) const;
    void MarkProgressFlag(const std::string& progressId);

private:
    std::string BuildNPCOpeningTriggerId(
        const NPC* npc,
        std::size_t talkPageIndex) const;
    std::string BuildNPCEndingTriggerId(
        const NPC* npc,
        std::size_t talkPageIndex) const;

    GameWorld& mWorld;
    PhysicsSystem& mPhysicsSystem;
    const std::string& mCurrentStageYamlPath;
    StageProgressSystem mProgress;
};
