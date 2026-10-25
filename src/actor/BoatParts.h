#pragma once

#include "actor/Actor.h"

#include <string>
#include <yaml-cpp/yaml.h>

class Game;
class CollectableComponent;

// 回収可能な船の部品。回収判定はComponentに任せ、回収成立時にゲーム進行へ通知する。
class BoatParts : public Actor {
public:
    BoatParts(Game* game);
    void UpdateActor(float deltaTime) override;

    // 設定ルートのboatParts配列を読み、commonのスケールとtype別モデルを適用する。
    void ApplyConfig(const YAML::Node& configRoot, const std::string& type);

private:
    void AddCollectableComponent();
    void OnObtained();

private:
    // Actorが所有するComponentへの非所有参照。
    CollectableComponent* mCollectableComponent;
};
