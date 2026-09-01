#include "gfx/debug/stage/StageActorRuntimeCreationService.h"

#include "Game.h"
#include "actor/Platform.h"
#include "gfx/debug/DebugEditorContext.h"
#include "system/ActorLoadSystem.h"
#include "system/PhysicsSystem.h"
#include "system/actor_loader/StageActorCreationService.h"

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
    StageActorCreationService& creationService =
        actorLoadSystem->GetCreationService();

    switch (actorRef.type) {
    case StageActorType::Planet:
        return creationService.CreatePlanet(actorNode) != nullptr;
    case StageActorType::Enemy:
        return creationService.CreateEnemy(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::Platform: {
        Platform* platform =
            actorRef.sequenceName == "movingPlatforms"
                ? creationService.CreateLegacyMovingPlatform(
                      actorNode, stageYamlIndex)
                : creationService.CreatePlatform(
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
        return creationService.CreateCrystal(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::NPC:
        return creationService.CreateNPC(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::BoatParts:
        return creationService.CreateBoatParts(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::Boat:
        return creationService.CreateBoat(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::BoatArrivalPoint:
        return creationService.CreateBoatArrivalPoint(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::FallRespawnPoint:
        return creationService.CreateFallRespawnPoint(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::Key:
        return creationService.CreateKey(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::Star:
        return creationService.CreateStar(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::StageObject:
        return creationService.CreateStageObject(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::TutorialTrigger:
        return creationService.CreateTutorialTrigger(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::JewelItem:
        return creationService.CreateJewelItem(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::HazardActor:
        return creationService.CreateHazardActor(
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
