#include "actor/MovingPlatform.h"

#include "Game.h"
#include "actor/Planet.h"
#include "actor/Player.h"

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

    if (mMoveOnPlayer) {
        UpdatePlayerActivatedMovement(deltaTime);
        return;
    }

    UpdateAutomaticMovement(deltaTime);
}

void MovingPlatform::SetReturnDelay(float returnDelay)
{
    mReturnDelay = std::max(0.0f, returnDelay);
}

void MovingPlatform::SetEditorPreviewLocalPos(const glm::vec3& localPos)
{
    if (mEditorPreviewPoint == 1) {
        SetDestinationLocalPos(localPos);
    } else {
        const glm::vec3 destination = GetDestinationLocalPos();
        mBaseLocalPos = localPos;
        SetDestinationLocalPos(destination);
    }
}

void MovingPlatform::UpdateAutomaticMovement(float deltaTime)
{
    if (!GetCurrentPlanet()) {
        mFrameDelta = glm::vec3(0.0f);
        return;
    }

    const float duration = std::max(mMoveDuration, 0.01f);

    mMoveTimer += deltaTime;

    const float phase = std::fmod(mMoveTimer, duration) / duration;
    const float t = phase < 0.5f ? phase * 2.0f : 2.0f - phase * 2.0f;

    const glm::vec3 localPos = mBaseLocalPos + mMoveOffset * t;
    SetLocalPosition(localPos);
}

void MovingPlatform::UpdatePlayerActivatedMovement(float deltaTime)
{
    Planet* planet = GetCurrentPlanet();
    if (!planet) {
        mFrameDelta = glm::vec3(0.0f);
        return;
    }

    if (GetGame() && GetGame()->GetIsDebugEditorShowing()) {
        mWasEditorPreviewing = true;
        const glm::vec3 previewLocalPos =
            mEditorPreviewPoint == 1
                ? GetDestinationLocalPos()
                : mBaseLocalPos;
        SetLocalPosition(previewLocalPos, false);
        return;
    }

    if (mWasEditorPreviewing) {
        mWasEditorPreviewing = false;
        mHasBeenActivated = false;
        mReturnDelayTimer = 0.0f;
        mTravelProgress = 0.0f;
        SetLocalPosition(mBaseLocalPos, false);
        return;
    }

    const bool hasPlayer = HasPlayerOnPlatform();
    const float progressStep =
        std::max(0.0f, deltaTime) / std::max(mMoveDuration, 0.01f);

    if (hasPlayer) {
        mHasBeenActivated = true;
        mReturnDelayTimer = 0.0f;
        mTravelProgress = std::min(1.0f, mTravelProgress + progressStep);
    } else if (mHasBeenActivated) {
        mReturnDelayTimer += std::max(0.0f, deltaTime);
        if (mReturnDelayTimer >= mReturnDelay) {
            mTravelProgress = std::max(0.0f, mTravelProgress - progressStep);
            if (mTravelProgress <= 0.0f) {
                mHasBeenActivated = false;
                mReturnDelayTimer = 0.0f;
            }
        }
    }

    const float easedProgress =
        glm::smoothstep(0.0f, 1.0f, mTravelProgress);
    SetLocalPosition(mBaseLocalPos + mMoveOffset * easedProgress);
}

bool MovingPlatform::HasPlayerOnPlatform() const
{
    if (!GetGame()) {
        return false;
    }

    for (Player* player : GetGame()->GetPlayers()) {
        if (!player || !player->GetIsActive() || !player->GetOnGround()) {
            continue;
        }
        if (player->GetGroundActor() == this) {
            return true;
        }
    }

    return false;
}

void MovingPlatform::SetLocalPosition(
    const glm::vec3& localPos,
    bool calculateFrameDelta)
{
    Planet* planet = GetCurrentPlanet();
    if (!planet) {
        mFrameDelta = glm::vec3(0.0f);
        return;
    }

    const glm::vec3 newPos = planet->GetPos() + localPos;
    mFrameDelta =
        calculateFrameDelta ? newPos - GetPos() : glm::vec3(0.0f);
    SetPos(newPos);
}
