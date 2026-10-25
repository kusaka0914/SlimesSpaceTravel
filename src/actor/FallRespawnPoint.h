#pragma once

#include "actor/Actor.h"

#include <glm/glm.hpp>
#include <yaml-cpp/yaml.h>

class Game;

// 落下時のダメージとリスポーン判定に使う配置オブジェクト。自分では毎フレーム移動しない。
class FallRespawnPoint : public Actor {
public:
    FallRespawnPoint(Game* game);

    // 設定ルートのfallRespawnPoints配列を順に適用する。モデルとダメージ量は後の項目を優先する。
    void ApplyConfig(const YAML::Node& configRoot);
    void UpdateActor(float deltaTime) override {}

    void SetDamage(float damage) { mDamage = damage; }

    float GetDamage() const { return mDamage; }

private:
    float mDamage;
};
