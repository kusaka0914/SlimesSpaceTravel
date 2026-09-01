#pragma once

#include "actor/Actor.h"
#include <yaml-cpp/yaml.h>

class Game;
class CollectableComponent;
class FocusComponent;

class Key : public Actor {
public:
    Key(Game* game);
    void UpdateActor(float deltaTime) override;

    void ApplyConfig(const YAML::Node& configRoot);

    CollectableComponent* GetCollectableComponent() const { return mCollectableComponent; }
    FocusComponent* GetFocusComponent() const { return mFocusComponent; }

private:
    void AddCollectableComponent();
    void AddFocusComponent();
    void OnShown() const;
    void OnObtained();

private:
    bool mIsActivePrev;

    FocusComponent* mFocusComponent;
    CollectableComponent* mCollectableComponent;
};
