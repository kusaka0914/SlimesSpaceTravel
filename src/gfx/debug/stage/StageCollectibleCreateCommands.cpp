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

    return CommitCreation(
        config, [&]() { return mRuntimeCreationService.CreateActor(StageActorType::Crystal, crystalNode, index); });
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

    return CommitCreation(
        config, [&]() { return mRuntimeCreationService.CreateActor(StageActorType::BoatParts, partNode, index); });
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

    return CommitCreation(config,
                          [&]() { return mRuntimeCreationService.CreateActor(StageActorType::Star, starNode, index); });
}

bool StageActorCreateService::AddJewelItem(int currentPlanetNum, const std::string& modelPath,
                                           const std::string& texturePath, const glm::vec3& scale,
                                           const StageActorPlacement* placement)
{
    if (!CanCreateActor() || modelPath.empty() || !IsValidPlanetIndex(currentPlanetNum, "jewel item")) {
        return false;
    }

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    EnsureSequence(config, "jewelItems");
    const int index = static_cast<int>(config["jewelItems"].size());
    YAML::Node jewelItemNode = mNodeFactory.CreateJewelItem(currentPlanetNum, modelPath, texturePath, scale);
    ApplyPlacementToNode(jewelItemNode, currentPlanetNum, placement);
    config["jewelItems"].push_back(jewelItemNode);

    return CommitCreation(
        config, [&]() { return mRuntimeCreationService.CreateActor(StageActorType::JewelItem, jewelItemNode, index); });
}
