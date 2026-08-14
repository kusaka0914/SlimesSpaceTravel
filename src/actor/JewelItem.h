#pragma once

#include "actor/Actor.h"

class CollectableComponent;
class Game;

class JewelItem final : public Actor {
public:
    explicit JewelItem(Game* game);

    void UpdateActor(float deltaTime) override;

private:
    CollectableComponent* mCollectableComponent = nullptr;
};
