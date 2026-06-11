#pragma once

#include <glm/glm.hpp>
#include <yaml-cpp/yaml.h>

class Game;
class Planet;
class Enemy;
class Platform;

class ActorLoadSystem {
public:
    ActorLoadSystem(Game* game);

    void LoadData(bool isLoadPlayer);

    Planet* CreatePlanetFromStageNode(const YAML::Node& node);
    Enemy* CreateEnemyFromStageNode(const YAML::Node& node, int stageYamlIndex);
    Platform* CreatePlatformFromStageNode(const YAML::Node& node, int stageYamlIndex);

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