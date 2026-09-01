#pragma once

#include "actor/Actor.h"
#include <yaml-cpp/yaml.h>

#include <string>

class Game;
class CollectableComponent;

class BoatParts : public Actor {
public:
    BoatParts(Game* game);
    void UpdateActor(float deltaTime) override;

    void ApplyConfig(const YAML::Node& configRoot, const std::string& type);

private:
    void AddCollectableComponent();
    void OnObtained();

private:
    CollectableComponent* mCollectableComponent;
};
