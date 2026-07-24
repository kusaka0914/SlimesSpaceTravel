#pragma once

#include "actor/Actor.h"
#include <glm/glm.hpp>

class Game;
class CollectableComponent;

class Star : public Actor {
public:
    Star(Game* game);
    void UpdateActor(float deltaTime) override;

    void ApplyConfig();

    CollectableComponent* GetCollectableComponent() const { return mCollectableComponent; }

private:
    void AddCollectableComponent();
    void OnObtained();

private:
    CollectableComponent* mCollectableComponent;
    float mGlowEmitTimer = 0.0f;
    float mSparkleEmitTimer = 0.0f;
    float mSparklePhase = 0.0f;
};
