#pragma once

#include "actor/player/PlayerConfig.h"

#include <yaml-cpp/yaml.h>

class ActorPlacementLoader;
class Game;
class Player;

class StagePlayerLoader {
public:
    StagePlayerLoader(
        Game* game,
        const ActorPlacementLoader& placementLoader);

    void SetPlayerConfig(PlayerConfig config);
    void LoadPlayers(const YAML::Node& stageRoot);
    Player* CreatePlayerFromStageNode(
        const YAML::Node& node,
        int playerNum);
    bool CreatePlayerFromCurrentStage(int playerNum);

private:
    Game* mGame = nullptr;
    const ActorPlacementLoader& mPlacementLoader;
    PlayerConfig mPlayerConfig;
};
