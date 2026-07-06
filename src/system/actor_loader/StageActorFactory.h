#pragma once

#include "Game.h"
#include "Stage.h"
#include "actor/Planet.h"
#include "system/MeshLoadSystem.h"
#include "system/actor_loader/ActorPlacementLoader.h"

#include <cstddef>
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <yaml-cpp/yaml.h>

class Actor;

class StageActorFactory {
public:
    StageActorFactory(Game* game, const ActorPlacementLoader& placementLoader)
        : mGame(game),
          mPlacementLoader(placementLoader)
    {
    }

    template <class TActor>
    TActor*
    CreatePlacedActorFromStageNode(const YAML::Node& node, int stageYamlIndex, float defaultHeight,
                                   const glm::vec3& defaultScale, const std::string& defaultModelPath,
                                   const std::function<void(Planet*, TActor*)>& registerToPlanet,
                                   const std::function<void(TActor*, const YAML::Node&)>& applyConfig = {},
                                   const std::function<void(TActor*, const YAML::Node&)>& applyAfterStageOverride = {});

    template <class TActor>
    void LoadActorSequence(const char* path, const std::string& sequenceName,
                           const std::function<void(Planet*)>& clearFromPlanet,
                           const std::function<TActor*(const YAML::Node&, int)>& createActor);

private:
    Game* mGame = nullptr;
    const ActorPlacementLoader& mPlacementLoader;
};

template <class TActor>
TActor* StageActorFactory::CreatePlacedActorFromStageNode(
    const YAML::Node& node, int stageYamlIndex, float defaultHeight, const glm::vec3& defaultScale,
    const std::string& defaultModelPath, const std::function<void(Planet*, TActor*)>& registerToPlanet,
    const std::function<void(TActor*, const YAML::Node&)>& applyConfig,
    const std::function<void(TActor*, const YAML::Node&)>& applyAfterStageOverride)
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return nullptr;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    const int currentPlanetNum = node["currentPlanetNum"] ? node["currentPlanetNum"].as<int>() : 0;

    if (currentPlanetNum < 0 || currentPlanetNum >= static_cast<int>(planets.size())) {
        return nullptr;
    }

    Planet* currentPlanet = planets[currentPlanetNum];

    if (!currentPlanet) {
        return nullptr;
    }

    std::unique_ptr<TActor> actor = std::make_unique<TActor>(mGame);

    actor->SetCurrentPlanet(currentPlanet);

    mPlacementLoader.ApplyPlacementFromStageNode(actor.get(), node, currentPlanet, stageYamlIndex, defaultHeight);
    mPlacementLoader.ApplyRotationFromStageNode(actor.get(), node);

    actor->SetScale(defaultScale);
    actor->SetModelPath(defaultModelPath);

    if (applyConfig) {
        applyConfig(actor.get(), node);
    }

    if (node["modelPath"]) {
        actor->SetModelPath(node["modelPath"].as<std::string>());
    }

    mPlacementLoader.ApplyScaleFromStageNode(actor.get(), node);

    if (applyAfterStageOverride) {
        applyAfterStageOverride(actor.get(), node);
    }

    actor->Initialize();

    TActor* actorPtr = actor.get();

    mGame->GetMeshLoadSystem()->SetActorMesh(actorPtr);
    mGame->AddActor(std::move(actor));

    if (registerToPlanet) {
        registerToPlanet(currentPlanet, actorPtr);
    }

    return actorPtr;
}

template <class TActor>
void StageActorFactory::LoadActorSequence(const char* path, const std::string& sequenceName,
                                          const std::function<void(Planet*)>& clearFromPlanet,
                                          const std::function<TActor*(const YAML::Node&, int)>& createActor)
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return;
    }

    YAML::Node root = YAML::LoadFile(path);

    if (!root[sequenceName] || !root[sequenceName].IsSequence()) {
        return;
    }

    for (Planet* planet : mGame->GetCurrentStage()->GetPlanets()) {
        if (planet && clearFromPlanet) {
            clearFromPlanet(planet);
        }
    }

    const YAML::Node sequence = root[sequenceName];

    for (std::size_t i = 0; i < sequence.size(); ++i) {
        createActor(sequence[i], static_cast<int>(i));
    }
}
