#pragma once

#include "actor/Actor.h"

class Game;

class StageObject : public Actor {
public:
    explicit StageObject(Game* game);

    void SetCollisionEnabled(bool enabled) { mCollisionEnabled = enabled; }
    bool GetCollisionEnabled() const { return mCollisionEnabled; }

protected:
    bool ShouldUpdateUpVecEveryFrame() const override { return false; }

private:
    bool mCollisionEnabled = true;
};
