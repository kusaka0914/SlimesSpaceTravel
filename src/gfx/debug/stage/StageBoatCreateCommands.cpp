#include "gfx/debug/stage/StageActorCreateService.h"

#include "gfx/debug/stage/StageYamlRepository.h"

bool StageActorCreateService::AddBoat(int startPlanetNum, int destPlanetNum, int destStage,
                                      const StageActorPlacement* placement)
{
    if (!CanCreateActor()) {
        return false;
    }

    if (!IsValidPlanetIndex(startPlanetNum, "boat start")) {
        return false;
    }

    if (!IsValidPlanetIndex(destPlanetNum, "boat destination")) {
        return false;
    }

    YAML::Node config;

    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    EnsureSequence(config, "boats");

    const int index = static_cast<int>(config["boats"].size());
    YAML::Node boatNode = mNodeFactory.CreateBoat(
        startPlanetNum,
        destPlanetNum,
        destStage);
    ApplyPlacementToNode(boatNode, startPlanetNum, placement);

    config["boats"].push_back(boatNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mRuntimeCreationService.CreateActor(
        StageActorType::Boat, boatNode, index);
    mRuntimeCreationService.RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::AddBoatArrivalPoint(
    int currentPlanetNum,
    const std::string& modelPath,
    const glm::vec3& scale,
    const StageActorPlacement* placement)
{
    if (!CanCreateActor() || modelPath.empty() ||
        !IsValidPlanetIndex(currentPlanetNum, "boat arrival point")) {
        return false;
    }

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    EnsureSequence(config, "boatArrivalPoints");

    const int index =
        static_cast<int>(config["boatArrivalPoints"].size());
    YAML::Node arrivalPointNode =
        mNodeFactory.CreateBoatArrivalPoint(
            currentPlanetNum,
            modelPath,
            scale);
    ApplyPlacementToNode(
        arrivalPointNode,
        currentPlanetNum,
        placement);
    config["boatArrivalPoints"].push_back(arrivalPointNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mRuntimeCreationService.CreateActor(
        StageActorType::BoatArrivalPoint,
        arrivalPointNode,
        index);
    mRuntimeCreationService.RefreshPhysicsWorld();
    return true;
}

