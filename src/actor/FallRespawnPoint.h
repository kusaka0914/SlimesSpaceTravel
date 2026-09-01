#pragma once

#include "actor/Actor.h"
#include <yaml-cpp/yaml.h>

#include <glm/glm.hpp>

class Game;

class FallRespawnPoint : public Actor {
public:
    FallRespawnPoint(Game* game);

    void ApplyConfig(const YAML::Node& configRoot);
    void UpdateActor(float deltaTime) override {}

    void SetDamage(float damage) { mDamage = damage; }

    float GetDamage() const { return mDamage; }

private:
    float mDamage;
};
