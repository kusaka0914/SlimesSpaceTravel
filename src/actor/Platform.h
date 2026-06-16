#pragma once

#include "actor/Actor.h"

class Game;

class Platform : public Actor {
public:
    Platform(Game* game);
    void UpdateActor(float deltaTime) override;
    // bool ShouldUpdateUpVecEveryFrame() const override { return true; }

private:
};