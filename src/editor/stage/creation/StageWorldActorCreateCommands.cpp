#include "editor/stage/creation/StageActorCreateService.h"

#include "editor/stage/persistence/StageYamlRepository.h"

bool StageActorCreateService::AddPlanet(const std::string& modelPath)
{
    if (!CanCreateActor()) {
        return false;
    }

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    EnsureSequence(config, "planets");
    const int planetIndex = static_cast<int>(config["planets"].size());
    YAML::Node planetNode = mNodeFactory.CreatePlanet(planetIndex, modelPath);
    config["planets"].push_back(planetNode);

    return CommitCreation(
        config, [&]() { return mRuntimeCreationService.CreateActor(StageActorType::Planet, planetNode, planetIndex); });
}

bool StageActorCreateService::AddEllipsePlanet(const std::string& modelPath)
{
    if (!CanCreateActor()) {
        return false;
    }

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    EnsureSequence(config, "planets");
    const int planetIndex = static_cast<int>(config["planets"].size());
    const glm::vec3 defaultWorldPosition(static_cast<float>(planetIndex) * 32.0f, 0.0f, 0.0f);
    return AddEllipsePlanetAtPosition(modelPath, defaultWorldPosition);
}

bool StageActorCreateService::AddEllipsePlanetAtPosition(const std::string& modelPath, const glm::vec3& worldPosition)
{
    if (!CanCreateActor()) {
        return false;
    }

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    EnsureSequence(config, "planets");
    const int planetIndex = static_cast<int>(config["planets"].size());
    YAML::Node planetNode = mNodeFactory.CreatePlanet(planetIndex, modelPath);
    planetNode["center"][0] = worldPosition.x;
    planetNode["center"][1] = worldPosition.y;
    planetNode["center"][2] = worldPosition.z;
    planetNode["scale"][0] = 4.0f;
    planetNode["scale"][1] = 1.0f;
    planetNode["scale"][2] = 4.0f;
    planetNode["shape"] = "Ellipse";
    planetNode["canAttractNearbyPlayer"] = false;

    config["planets"].push_back(planetNode);
    return CommitCreation(
        config, [&]() { return mRuntimeCreationService.CreateActor(StageActorType::Planet, planetNode, planetIndex); });
}
