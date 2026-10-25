#pragma once

#include "actor/Actor.h"

#include <yaml-cpp/yaml.h>

class Game;

// 船の到着先として配置するマーカー。移動や到着処理はBoatが担当する。
class BoatArrivalPoint : public Actor {
public:
    BoatArrivalPoint(Game* game);

    void UpdateActor(float deltaTime) override {}

    // 設定ルートのboatArrivalPoints配列を順に適用する。モデルとスケールは後の項目を優先する。
    void ApplyConfig(const YAML::Node& configRoot);
};
