#pragma once

#include "actor/Actor.h"

#include <string>

class Game;
class DestructibleComponent;

class Crystal : public Actor {
public:
    Crystal(Game* game);
    void UpdateActor(float deltaTime) override;

    void ApplyConfig(const std::string& type);

    DestructibleComponent* GetDestructibleComponent() const { return mDestructibleComponent; }

private:
    void AddDestructibleComponent();
    void OnDestroyed();

private:
    DestructibleComponent* mDestructibleComponent;
};