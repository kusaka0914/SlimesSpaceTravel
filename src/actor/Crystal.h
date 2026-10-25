#pragma once

#include "actor/Actor.h"

#include <string>
#include <yaml-cpp/yaml.h>

class Game;
class DestructibleComponent;

// 破壊可能な配置オブジェクト。耐久値と攻撃判定はDestructibleComponentが担当する。
class Crystal : public Actor {
public:
    Crystal(Game* game);
    void UpdateActor(float deltaTime) override;

    // 設定ルートのcrystals配列から、commonのモデルとtype別の耐久値・大きさを適用する。
    void ApplyConfig(const YAML::Node& configRoot, const std::string& type);

    DestructibleComponent* GetDestructibleComponent() const { return mDestructibleComponent; }

private:
    void AddDestructibleComponent();
    void OnDestroyed();

private:
    // Actorが所有するComponentへの非所有参照。
    DestructibleComponent* mDestructibleComponent;
};
