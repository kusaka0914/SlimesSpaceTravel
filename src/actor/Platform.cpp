#include "Platform.h"
#include <iostream>

Platform::Platform(Game* game)
    : Actor(game)
{
    mIsUpVecInitialized = true;
}

void Platform::UpdateActor(float deltaTime) {}