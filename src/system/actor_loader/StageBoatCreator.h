#pragma once

#include <yaml-cpp/yaml.h>

class ActorPlacementLoader;
class Boat;
class Game;

// 出発・到着Planetと到着点を解決し、Boatを生成してGameとPlanetへ登録する。
class StageBoatCreator {
public:
    StageBoatCreator(Game* game, const ActorPlacementLoader& placementLoader);

    // 生成結果への借用ポインターを返す。Stageや必要なPlanetを解決できなければnullptr。YAML例外は伝播する。
    Boat* CreateFromStageNode(const YAML::Node& node, int stageYamlIndex);

private:
    Game* mGame = nullptr;
    const ActorPlacementLoader& mPlacementLoader;
};
