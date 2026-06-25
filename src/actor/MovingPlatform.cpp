#include "actor/MovingPlatform.h"

#include "Game.h"
#include "actor/Planet.h"

#include <algorithm>
#include <cmath>

MovingPlatform::MovingPlatform(Game* game)
    : Platform(game)
{
}

void MovingPlatform::UpdateActor(float deltaTime)
{
    Planet* planet = GetCurrentPlanet();

    if (!planet) {
        mFrameDelta = glm::vec3(0.0f);
        return;
    }

    const glm::vec3 oldPos = GetPos();

    const float duration = std::max(mMoveDuration, 0.01f);

    mMoveTimer += deltaTime;

    const float phase = std::fmod(mMoveTimer, duration) / duration;
    const float t = phase < 0.5f ? phase * 2.0f : 2.0f - phase * 2.0f;

    const glm::vec3 localPos = mBaseLocalPos + mMoveOffset * t;
    const glm::vec3 newPos = planet->GetPos() + localPos;

    mFrameDelta = newPos - oldPos;

    SetPos(newPos);
}