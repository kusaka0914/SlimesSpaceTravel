#pragma once

#include <glm/glm.hpp>
#include <yaml-cpp/yaml.h>

class Game;
class Planet;
class Player;
class Enemy;
class Platform;
class NPC;
class Crystal;
class BoatParts;
class Boat;
class Star;
class Actor;

class ActorLoadSystem {
public:
    ActorLoadSystem(Game* game);

    void LoadData(bool isLoadPlayer);

    Planet* CreatePlanetFromStageNode(const YAML::Node& node);
    Player* CreatePlayerFromStageNode(const YAML::Node& node, int playerNum);
    Enemy* CreateEnemyFromStageNode(const YAML::Node& node, int stageYamlIndex);
    Platform* CreatePlatformFromStageNode(const YAML::Node& node, int stageYamlIndex);
    NPC* CreateNPCFromStageNode(const YAML::Node& node, int stageYamlIndex);
    Crystal* CreateCrystalFromStageNode(const YAML::Node& node, int stageYamlIndex);
    BoatParts* CreateBoatPartsFromStageNode(const YAML::Node& node, int stageYamlIndex);
    Boat* CreateBoatFromStageNode(const YAML::Node& node, int stageYamlIndex);
    Star* CreateStarFromStageNode(const YAML::Node& node, int stageYamlIndex);
    bool CreatePlayerFromCurrentStage(int playerNum);
    void ApplyPlacementFromStageNode(Actor* actor, const YAML::Node& node, Planet* currentPlanet, int stageYamlIndex,
                                     float defaultHeight = 0.0f);
    void ApplyRotationFromStageNode(Actor* actor, const YAML::Node& node);

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

    void ApplyEnemyConfig(Enemy* enemy, const std::string& type);

    glm::vec3 CalculatePos(YAML::Node node, Planet* currentPlanet);

private:
    Game* mGame;
};