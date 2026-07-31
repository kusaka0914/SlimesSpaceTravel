#pragma once

#include "system/actor_loader/ActorPlacementLoader.h"
#include "system/actor_loader/StageActorFactory.h"

#include <glm/glm.hpp>
#include <string>
#include <yaml-cpp/yaml.h>

class Game;
class Player;
class Enemy;
class Platform;
class NPC;
class Crystal;
class BoatParts;
class Boat;
class Star;
class Actor;
class Key;
class BoatArrivalPoint;
class FallRespawnPoint;
class Planet;
class StageObject;
class TutorialTrigger;

class ActorLoadSystem {
public:
    explicit ActorLoadSystem(Game* game);

    void LoadData(bool isLoadPlayer);

    Planet* CreatePlanetFromStageNode(const YAML::Node& node);
    Player* CreatePlayerFromStageNode(const YAML::Node& node, int playerNum);
    Enemy* CreateEnemyFromStageNode(const YAML::Node& node, int stageYamlIndex);
    Platform* CreatePlatformFromStageNode(const YAML::Node& node, int stageYamlIndex);
    Platform* CreateLegacyMovingPlatformFromStageNode(const YAML::Node& node, int stageYamlIndex);
    NPC* CreateNPCFromStageNode(const YAML::Node& node, int stageYamlIndex);
    Crystal* CreateCrystalFromStageNode(const YAML::Node& node, int stageYamlIndex);
    BoatParts* CreateBoatPartsFromStageNode(const YAML::Node& node, int stageYamlIndex);
    Boat* CreateBoatFromStageNode(const YAML::Node& node, int stageYamlIndex);
    Star* CreateStarFromStageNode(const YAML::Node& node, int stageYamlIndex);
    Key* CreateKeyFromStageNode(const YAML::Node& node, int stageYamlIndex);
    bool CreatePlayerFromCurrentStage(int playerNum);
    BoatArrivalPoint* CreateBoatArrivalPointFromStageNode(const YAML::Node& node, int stageYamlIndex);
    FallRespawnPoint* CreateFallRespawnPointFromStageNode(const YAML::Node& node, int stageYamlIndex);
    StageObject* CreateStageObjectFromStageNode(const YAML::Node& node, int stageYamlIndex);
    TutorialTrigger* CreateTutorialTriggerFromStageNode(
        const YAML::Node& node,
        int stageYamlIndex);
    Actor* FindPlacedActor(const std::string& sequenceName, int stageYamlIndex) const;

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
    void LoadLegacyMovingPlatforms(const char* path);
    void LoadBoatArrivalPoints(const char* path);
    void LoadFallRespawnPoints(const char* path);
    void LoadStageObjects(const char* path);
    void LoadTutorialTriggers(const char* path);

private:
    Game* mGame = nullptr;
    ActorPlacementLoader mPlacementLoader;
    StageActorFactory mActorFactory;
};
