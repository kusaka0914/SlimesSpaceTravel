#pragma once

#include "actor/Actor.h"

class Game;

class BoatArrivalPoint : public Actor {
public:
    BoatArrivalPoint(Game* game);

    void UpdateActor(float deltaTime) override {}
    void ApplyConfig();
};