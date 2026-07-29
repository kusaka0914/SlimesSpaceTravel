#include "component/PlatformMovementComponent.h"

#include "Game.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Player.h"

#include <algorithm>
#include <cmath>

PlatformMovementComponent::PlatformMovementComponent(
    Platform* owner,
    int updateOrder)
    : Component(owner, updateOrder),
      mPlatform(owner)
{
}

void PlatformMovementComponent::Update(float deltaTime)
{
    if (!mPlatform || !mPlatform->GetCurrentPlanet()) {
        mFrameDelta = glm::vec3(0.0f);
        return;
    }

    if (mMoveOnPlayer) {
        UpdatePlayerActivatedMovement(deltaTime);
    } else {
        UpdateAutomaticMovement(deltaTime);
    }
}

void PlatformMovementComponent::SetMoveDuration(float moveDuration)
{
    mMoveDuration = std::max(0.1f, moveDuration);
}

void PlatformMovementComponent::SetReturnDelay(float returnDelay)
{
    mReturnDelay = std::max(0.0f, returnDelay);
}

void PlatformMovementComponent::SetEditorPreviewLocalPos(const glm::vec3& localPos)
{
    if (mEditorPreviewPoint == 1) {
        SetDestinationLocalPos(localPos);
    } else {
        const glm::vec3 destination = GetDestinationLocalPos();
        mBaseLocalPos = localPos;
        SetDestinationLocalPos(destination);
    }
}

void PlatformMovementComponent::UpdateAutomaticMovement(float deltaTime)
{
    const float duration = std::max(mMoveDuration, 0.01f);
    mMoveTimer += deltaTime;

    const float phase = std::fmod(mMoveTimer, duration) / duration;
    const float interpolation =
        phase < 0.5f ? phase * 2.0f : 2.0f - phase * 2.0f;
    SetLocalPosition(mBaseLocalPos + mMoveOffset * interpolation);
}

void PlatformMovementComponent::UpdatePlayerActivatedMovement(float deltaTime)
{
    Game* game = mPlatform ? mPlatform->GetGame() : nullptr;
    if (!game) {
        mFrameDelta = glm::vec3(0.0f);
        return;
    }

    if (game->GetIsDebugEditorShowing()) {
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

bool PlatformMovementComponent::HasPlayerOnPlatform() const
{
    Game* game = mPlatform ? mPlatform->GetGame() : nullptr;
    if (!game) {
        return false;
    }

    for (Player* player : game->GetPlayers()) {
        if (!player || !player->GetIsActive() || !player->GetOnGround()) {
            continue;
        }
        if (player->GetGroundActor() == mPlatform) {
            return true;
        }
    }

    return false;
}

void PlatformMovementComponent::SetLocalPosition(
    const glm::vec3& localPos,
    bool calculateFrameDelta)
{
    Planet* planet = mPlatform ? mPlatform->GetCurrentPlanet() : nullptr;
    if (!planet) {
        mFrameDelta = glm::vec3(0.0f);
        return;
    }

    const glm::vec3 newPos = planet->GetPos() + localPos;
    mFrameDelta =
        calculateFrameDelta
            ? newPos - mPlatform->GetPos()
            : glm::vec3(0.0f);
    mPlatform->SetPos(newPos);
}
