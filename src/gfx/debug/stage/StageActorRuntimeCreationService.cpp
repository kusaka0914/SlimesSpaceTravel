#include "gfx/debug/stage/StageActorRuntimeCreationService.h"

#include "Game.h"
#include "actor/Platform.h"
#include "gfx/debug/DebugEditorContext.h"
#include "system/ActorLoadSystem.h"
#include "system/PhysicsSystem.h"

StageActorRuntimeCreationService::StageActorRuntimeCreationService(
    DebugEditorContext& context)
    : mContext(context)
{
}

bool StageActorRuntimeCreationService::CreateActor(
    StageActorType actorType,
    const YAML::Node& actorNode,
    int stageYamlIndex) const
{
    return CreateActor(
        StageActorRef{actorType, stageYamlIndex},
        actorNode,
        stageYamlIndex);
}

bool StageActorRuntimeCreationService::CreateActor(
    const StageActorRef& actorRef,
    const YAML::Node& actorNode,
    int stageYamlIndex) const
{
    ActorLoadSystem* actorLoadSystem =
        mContext.game ? mContext.game->GetActorLoadSystem() : nullptr;
    if (!actorLoadSystem) {
        return false;
    }

    switch (actorRef.type) {
    case StageActorType::Planet:
        return actorLoadSystem->CreatePlanetFromStageNode(actorNode) != nullptr;
    case StageActorType::Enemy:
        return actorLoadSystem->CreateEnemyFromStageNode(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::Platform: {
        Platform* platform =
            actorRef.sequenceName == "movingPlatforms"
                ? actorLoadSystem->CreateLegacyMovingPlatformFromStageNode(
                      actorNode, stageYamlIndex)
                : actorLoadSystem->CreatePlatformFromStageNode(
                      actorNode, stageYamlIndex);
        if (!platform) {
            return false;
        }
        if (!actorRef.sequenceName.empty()) {
            platform->SetStageSequenceName(actorRef.sequenceName);
        }
        return true;
    }
    case StageActorType::Crystal:
        return actorLoadSystem->CreateCrystalFromStageNode(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::NPC:
        return actorLoadSystem->CreateNPCFromStageNode(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::BoatParts:
        return actorLoadSystem->CreateBoatPartsFromStageNode(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::Boat:
        return actorLoadSystem->CreateBoatFromStageNode(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::BoatArrivalPoint:
        return actorLoadSystem->CreateBoatArrivalPointFromStageNode(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::FallRespawnPoint:
        return actorLoadSystem->CreateFallRespawnPointFromStageNode(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::Key:
        return actorLoadSystem->CreateKeyFromStageNode(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::Star:
        return actorLoadSystem->CreateStarFromStageNode(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::StageObject:
        return actorLoadSystem->CreateStageObjectFromStageNode(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::TutorialTrigger:
        return actorLoadSystem->CreateTutorialTriggerFromStageNode(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::JewelItem:
        return actorLoadSystem->CreateJewelItemFromStageNode(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::HazardActor:
        return actorLoadSystem->CreateHazardActorFromStageNode(
                   actorNode, stageYamlIndex) != nullptr;
    }

    return false;
}

void StageActorRuntimeCreationService::RefreshPhysicsWorld() const
{
    if (mContext.game && mContext.game->GetPhysicsSystem()) {
        mContext.game->GetPhysicsSystem()->Initialize();
    }
}
