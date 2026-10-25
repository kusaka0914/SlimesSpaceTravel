#pragma once

#include <yaml-cpp/yaml.h>

class Game;
class Planet;

// ステージ定義からPlanetを生成し、所有権をGameへ渡してStageへ登録する。
class StagePlanetCreator {
public:
    explicit StagePlanetCreator(Game* game);

    // 生成結果への借用ポインターを返す。現在のStageがなければnullptr。YAML例外は伝播する。
    Planet* CreateFromStageNode(const YAML::Node& node);

private:
    Game* mGame = nullptr;
};
