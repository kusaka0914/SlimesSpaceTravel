#pragma once

#include "actor/Actor.h"

class CollectableComponent;
class Game;

// 回収したプレイヤーへジュエルを付与し、回収音を鳴らして非アクティブ化するアイテム。
class JewelItem final : public Actor {
public:
    explicit JewelItem(Game* game);

    void UpdateActor(float deltaTime) override;

private:
    // Actorが所有するComponentへの非所有参照。
    CollectableComponent* mCollectableComponent = nullptr;
};
