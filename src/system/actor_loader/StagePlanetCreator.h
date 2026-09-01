#pragma once

#include <yaml-cpp/yaml.h>

class Game;
class Planet;

class StagePlanetCreator {
public:
    explicit StagePlanetCreator(Game* game);

    Planet* CreateFromStageNode(const YAML::Node& node);

private:
    Game* mGame = nullptr;
};
