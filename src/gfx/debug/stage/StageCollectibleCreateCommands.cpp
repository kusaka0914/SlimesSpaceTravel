#include "gfx/debug/stage/StageActorCreateService.h"

#include "Game.h"
#include "gfx/debug/stage/StageYamlRepository.h"

bool StageActorCreateService::AddCrystal(const std::string& type, int currentPlanetNum,
                                         const StageActorPlacement* placement)
{
    if (!CanCreateActor()) {
        return false;
    }

    if (!IsValidPlanetIndex(currentPlanetNum, "crystal")) {
        return false;
    }

    YAML::Node config;

    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    EnsureSequence(config, "crystals");

    const int index = static_cast<int>(config["crystals"].size());
    YAML::Node crystalNode = mNodeFactory.CreateCrystal(type, currentPlanetNum);
    ApplyPlacementToNode(crystalNode, currentPlanetNum, placement);

    config["crystals"].push_back(crystalNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mRuntimeCreationService.CreateActor(
        StageActorType::Crystal, crystalNode, index);
    mRuntimeCreationService.RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::AddBoatParts(const std::string& type, int currentPlanetNum,
                                           const StageActorPlacement* placement)
{
    if (!CanCreateActor()) {
        return false;
    }

    if (!IsValidPlanetIndex(currentPlanetNum, "boat parts")) {
        return false;
    }

    YAML::Node config;

    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    EnsureSequence(config, "boatParts");

    const int index = static_cast<int>(config["boatParts"].size());
    YAML::Node partNode = mNodeFactory.CreateBoatParts(type, currentPlanetNum);
    ApplyPlacementToNode(partNode, currentPlanetNum, placement);

    config["boatParts"].push_back(partNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mRuntimeCreationService.CreateActor(
        StageActorType::BoatParts, partNode, index);
    mRuntimeCreationService.RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::AddStar(int currentPlanetNum, const StageActorPlacement* placement)
{
    if (!CanCreateActor()) {
        return false;
    }

    if (!IsValidPlanetIndex(currentPlanetNum, "star")) {
        return false;
    }

    YAML::Node config;

    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    EnsureSequence(config, "star");

    const int index = static_cast<int>(config["star"].size());
    YAML::Node starNode = mNodeFactory.CreateStar(currentPlanetNum);
    ApplyPlacementToNode(starNode, currentPlanetNum, placement);
    if (mContext.game->GetIsUGCMode()) {
        starNode["isActive"] = true;
    }

    config["star"].push_back(starNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mRuntimeCreationService.CreateActor(
        StageActorType::Star, starNode, index);
    mRuntimeCreationService.RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::AddJewelItem(
    int currentPlanetNum,
    const std::string& modelPath,
    const std::string& texturePath,
    const glm::vec3& scale,
    const StageActorPlacement* placement)
{
    if (!CanCreateActor() || modelPath.empty() ||
        !IsValidPlanetIndex(currentPlanetNum, "jewel item")) {
        return false;
    }

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    EnsureSequence(config, "jewelItems");
    const int index = static_cast<int>(config["jewelItems"].size());
    YAML::Node jewelItemNode = mNodeFactory.CreateJewelItem(
        currentPlanetNum,
        modelPath,
        texturePath,
        scale);
    ApplyPlacementToNode(jewelItemNode, currentPlanetNum, placement);
    config["jewelItems"].push_back(jewelItemNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mRuntimeCreationService.CreateActor(
        StageActorType::JewelItem, jewelItemNode, index);
    mRuntimeCreationService.RefreshPhysicsWorld();
    return true;
}

