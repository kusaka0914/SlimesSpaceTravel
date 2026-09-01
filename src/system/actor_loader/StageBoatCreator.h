#pragma once

#include <yaml-cpp/yaml.h>

class ActorPlacementLoader;
class Boat;
class Game;

class StageBoatCreator {
public:
    StageBoatCreator(
        Game* game,
        const ActorPlacementLoader& placementLoader);

    Boat* CreateFromStageNode(
        const YAML::Node& node,
        int stageYamlIndex);

private:
    Game* mGame = nullptr;
    const ActorPlacementLoader& mPlacementLoader;
};
