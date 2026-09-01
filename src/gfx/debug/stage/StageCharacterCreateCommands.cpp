#include "gfx/debug/stage/StageActorCreateService.h"

#include "gfx/debug/stage/StageYamlRepository.h"

bool StageActorCreateService::AddEnemy(const std::string& type, int currentPlanetNum,
                                       const StageActorPlacement* placement)
{
    if (!CanCreateActor()) {
        return false;
    }

    if (!IsValidPlanetIndex(currentPlanetNum, "enemy")) {
        return false;
    }

    YAML::Node config;

    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    EnsureSequence(config, "enemies");

    const int index = static_cast<int>(config["enemies"].size());
    YAML::Node enemyNode = mNodeFactory.CreateEnemy(type, currentPlanetNum);
    ApplyPlacementToNode(enemyNode, currentPlanetNum, placement);

    config["enemies"].push_back(enemyNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mRuntimeCreationService.CreateActor(
        StageActorType::Enemy, enemyNode, index);
    mRuntimeCreationService.RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::AddNPC(
    const std::string& modelPath,
    int currentPlanetNum,
    const std::string& name,
    const std::vector<std::string>& talkTexts,
    float radius,
    float scale,
    const StageActorPlacement* placement)
{
    if (!CanCreateActor()) {
        return false;
    }

    if (!IsValidPlanetIndex(currentPlanetNum, "NPC")) {
        return false;
    }

    YAML::Node config;

    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    EnsureSequence(config, "NPCs");

    const int index = static_cast<int>(config["NPCs"].size());
    YAML::Node npcNode =
        mNodeFactory.CreateNPC(
            modelPath,
            currentPlanetNum,
            name,
            talkTexts,
            radius,
            scale);
    ApplyPlacementToNode(npcNode, currentPlanetNum, placement);

    config["NPCs"].push_back(npcNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mRuntimeCreationService.CreateActor(
        StageActorType::NPC, npcNode, index);
    mRuntimeCreationService.RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::AddTutorialTrigger(
    int currentPlanetNum,
    const std::string& modelPath,
    const std::vector<std::string>& talkTexts,
    const glm::vec3& scale,
    const StageActorPlacement* placement)
{
    if (!CanCreateActor() || modelPath.empty() ||
        !IsValidPlanetIndex(
            currentPlanetNum,
            "tutorial trigger")) {
        return false;
    }

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(
            mContext,
            config)) {
        return false;
    }

    EnsureSequence(config, "tutorialTriggers");
    const int index =
        static_cast<int>(
            config["tutorialTriggers"].size());
    YAML::Node triggerNode =
        mNodeFactory.CreateTutorialTrigger(
            currentPlanetNum,
            modelPath,
            talkTexts,
            scale);
    ApplyPlacementToNode(triggerNode, currentPlanetNum, placement);
    config["tutorialTriggers"].push_back(triggerNode);

    if (!StageYamlRepository::SaveCurrentStage(
            mContext,
            config)) {
        return false;
    }

    mRuntimeCreationService.CreateActor(
        StageActorType::TutorialTrigger, triggerNode, index);
    mRuntimeCreationService.RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::AddHazardActor(
    int currentPlanetNum,
    const std::string& modelPath,
    const std::string& texturePath,
    const glm::vec3& scale,
    float triggerRadius,
    float damage,
    float damageIntervalSeconds,
    const StageActorPlacement* placement)
{
    if (!CanCreateActor() || modelPath.empty() ||
        !IsValidPlanetIndex(currentPlanetNum, "hazard actor")) {
        return false;
    }

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    EnsureSequence(config, "hazardActors");
    const int index =
        static_cast<int>(config["hazardActors"].size());
    YAML::Node hazardActorNode = mNodeFactory.CreateHazardActor(
        currentPlanetNum,
        modelPath,
        texturePath,
        scale,
        triggerRadius,
        damage,
        damageIntervalSeconds);
    ApplyPlacementToNode(
        hazardActorNode,
        currentPlanetNum,
        placement);
    config["hazardActors"].push_back(hazardActorNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mRuntimeCreationService.CreateActor(
        StageActorType::HazardActor, hazardActorNode, index);
    mRuntimeCreationService.RefreshPhysicsWorld();
    return true;
}

