#include "gfx/debug/stage/StageActorCreateService.h"

#include "gfx/debug/stage/StageYamlRepository.h"

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

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mRuntimeCreationService.CreateActor(
        StageActorType::Planet, planetNode, planetIndex);
    mRuntimeCreationService.RefreshPhysicsWorld();
    return true;
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
    YAML::Node planetNode = mNodeFactory.CreatePlanet(planetIndex, modelPath);
    planetNode["scale"][0] = 4.0f;
    planetNode["scale"][1] = 1.0f;
    planetNode["scale"][2] = 4.0f;
    planetNode["shape"] = "Ellipse";

    config["planets"].push_back(planetNode);
    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mRuntimeCreationService.CreateActor(
        StageActorType::Planet, planetNode, planetIndex);
    mRuntimeCreationService.RefreshPhysicsWorld();
    return true;
}
