#pragma once

#include "Game.h"
#include "Stage.h"
#include "actor/Planet.h"
#include "system/MeshLoadSystem.h"

#include <cstddef>
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <yaml-cpp/yaml.h>

class Player;
class Enemy;
class Platform;
class MovingPlatform;
class NPC;
class Crystal;
class BoatParts;
class Boat;
class Star;
class Actor;
class Key;
class BoatArrivalPoint;
class FallRespawnPoint;

class ActorLoadSystem {
public:
    ActorLoadSystem(Game* game);

    void LoadData(bool isLoadPlayer);

    Planet* CreatePlanetFromStageNode(const YAML::Node& node);
    Player* CreatePlayerFromStageNode(const YAML::Node& node, int playerNum);
    Enemy* CreateEnemyFromStageNode(const YAML::Node& node, int stageYamlIndex);
    Platform* CreatePlatformFromStageNode(const YAML::Node& node, int stageYamlIndex);
    MovingPlatform* CreateMovingPlatformFromStageNode(const YAML::Node& node, int stageYamlIndex);
    NPC* CreateNPCFromStageNode(const YAML::Node& node, int stageYamlIndex);
    Crystal* CreateCrystalFromStageNode(const YAML::Node& node, int stageYamlIndex);
    BoatParts* CreateBoatPartsFromStageNode(const YAML::Node& node, int stageYamlIndex);
    Boat* CreateBoatFromStageNode(const YAML::Node& node, int stageYamlIndex);
    Star* CreateStarFromStageNode(const YAML::Node& node, int stageYamlIndex);
    Key* CreateKeyFromStageNode(const YAML::Node& node, int stageYamlIndex);
    bool CreatePlayerFromCurrentStage(int playerNum);
    BoatArrivalPoint* CreateBoatArrivalPointFromStageNode(const YAML::Node& node, int stageYamlIndex);
    FallRespawnPoint* CreateFallRespawnPointFromStageNode(const YAML::Node& node, int stageYamlIndex);

    void ApplyPlacementFromStageNode(Actor* actor, const YAML::Node& node, Planet* currentPlanet, int stageYamlIndex,
                                     float defaultHeight = 0.0f);
    void ApplyRotationFromStageNode(Actor* actor, const YAML::Node& node);
    void ApplyScaleFromStageNode(Actor* actor, const YAML::Node& node);

private:
    void LoadPlayers(const char* path);
    void LoadNPCs(const char* path);
    void LoadEnemies(const char* path);
    void LoadPlanets(const char* path);
    void LoadBoats(const char* path);
    void LoadBoatParts(const char* path);
    void LoadKeys(const char* path);
    void LoadCrystals(const char* path);
    void LoadStar(const char* path);
    void LoadPlatforms(const char* path);
    void LoadMovingPlatforms(const char* path);
    void LoadBoatArrivalPoints(const char* path);
    void LoadFallRespawnPoints(const char* path);

    glm::vec3 CalculatePos(YAML::Node node, Planet* currentPlanet);

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
    Game* mGame;
};

template <class TActor>
TActor* ActorLoadSystem::CreatePlacedActorFromStageNode(
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

    ApplyPlacementFromStageNode(actor.get(), node, currentPlanet, stageYamlIndex, defaultHeight);
    ApplyRotationFromStageNode(actor.get(), node);

    actor->SetScale(defaultScale);
    actor->SetModelPath(defaultModelPath);

    if (applyConfig) {
        applyConfig(actor.get(), node);
    }

    if (node["modelPath"]) {
        actor->SetModelPath(node["modelPath"].as<std::string>());
    }

    ApplyScaleFromStageNode(actor.get(), node);

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
void ActorLoadSystem::LoadActorSequence(const char* path, const std::string& sequenceName,
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

    YAML::Node sequence = root[sequenceName];

    for (std::size_t i = 0; i < sequence.size(); ++i) {
        createActor(sequence[i], static_cast<int>(i));
    }
}