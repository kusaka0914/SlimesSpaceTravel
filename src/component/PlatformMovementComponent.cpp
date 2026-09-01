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

    if (UpdateEditorPreview(deltaTime)) {
        return;
    }

    if (mMoveOnPlayer) {
        UpdatePlayerActivatedMovement(deltaTime);
    } else {
        UpdateAutomaticMovement(deltaTime);
    }
}

bool PlatformMovementComponent::UpdateEditorPreview(float deltaTime)
{
    Game* game = mPlatform ? mPlatform->GetGame() : nullptr;
    if (!game) {
        return false;
    }

    const bool isFreeCameraEditorActive =
        game->GetIsDebugEditorShowing() &&
        game->GetIsFreeCameraMode();
    if (isFreeCameraEditorActive) {
        mWasEditorPreviewing = true;
        if (mIsEditorMovementPreviewPlaying) {
            AdvanceEditorMovementPreview(deltaTime);
            return true;
        }

        const glm::vec3 previewLocalPos =
            mEditorPreviewPoint == 1
                ? GetDestinationLocalPos()
                : mBaseLocalPos;
        SetLocalPosition(previewLocalPos, false);
        return true;
    }

    if (!mWasEditorPreviewing) {
        return false;
    }

    ResetRuntimeMovementAfterEditorPreview();
    return true;
}

void PlatformMovementComponent::ResetRuntimeMovementAfterEditorPreview()
{
    mWasEditorPreviewing = false;
    mMoveTimer = 0.0f;
    mHasBeenActivated = false;
    mReturnDelayTimer = 0.0f;
    mEndpointWaitElapsedSeconds = 0.0f;
    mStartEndpointWaitRemainingSeconds = 0.0f;
    mTravelProgress = 0.0f;
    SetLocalPosition(mBaseLocalPos, false);
}

void PlatformMovementComponent::SetMoveDuration(float moveDuration)
{
    mMoveDuration = std::max(0.1f, moveDuration);
}

void PlatformMovementComponent::SetReturnDelay(float returnDelay)
{
    mReturnDelay = std::max(0.0f, returnDelay);
}

void PlatformMovementComponent::SetEndpointWaitDurationSeconds(
    float endpointWaitDurationSeconds)
{
    mEndpointWaitDurationSeconds =
        std::max(0.0f, endpointWaitDurationSeconds);
}

void PlatformMovementComponent::SetEditorPreviewPoint(int point)
{
    StopEditorMovementPreview();
    mEditorPreviewPoint = point == 1 ? 1 : 0;

    const glm::vec3 previewLocalPos =
        mEditorPreviewPoint == 1
            ? GetDestinationLocalPos()
            : mBaseLocalPos;
    SetLocalPosition(previewLocalPos, false);
}

void PlatformMovementComponent::StartEditorMovementPreview()
{
    mEditorPreviewPoint = 0;
    mEditorMovementPreviewElapsedSeconds = 0.0f;
    mIsEditorMovementPreviewPlaying = true;
    SetLocalPosition(mBaseLocalPos, false);
}

void PlatformMovementComponent::StopEditorMovementPreview()
{
    mIsEditorMovementPreviewPlaying = false;
    mEditorMovementPreviewElapsedSeconds = 0.0f;
}

void PlatformMovementComponent::UpdateEditorMovementPreview(
    float deltaTime)
{
    Game* game = mPlatform ? mPlatform->GetGame() : nullptr;
    if (!game || !game->GetIsDebugEditorShowing() ||
        !game->GetIsFreeCameraMode() ||
        !mIsEditorMovementPreviewPlaying) {
        return;
    }

    mWasEditorPreviewing = true;
    AdvanceEditorMovementPreview(deltaTime);
}

void PlatformMovementComponent::AdvanceEditorMovementPreview(
    float deltaTime)
{
    const float previewDurationSeconds =
        mMoveOnPlayer
            ? std::max(mMoveDuration, 0.01f)
            : std::max(mMoveDuration * 0.5f, 0.01f);
    mEditorMovementPreviewElapsedSeconds +=
        std::max(0.0f, deltaTime);
    const float progress = glm::clamp(
        mEditorMovementPreviewElapsedSeconds /
            previewDurationSeconds,
        0.0f,
        1.0f);
    const float interpolation =
        mMoveOnPlayer
            ? glm::smoothstep(0.0f, 1.0f, progress)
            : progress;
    SetLocalPosition(
        mBaseLocalPos + mMoveOffset * interpolation,
        false);

    if (progress >= 1.0f) {
        mIsEditorMovementPreviewPlaying = false;
        mEditorPreviewPoint = 1;
    }
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
    const float movementDuration = std::max(mMoveDuration, 0.01f);
    const float oneWayMovementDuration = movementDuration * 0.5f;
    const float cycleDuration =
        movementDuration + mEndpointWaitDurationSeconds * 2.0f;
    mMoveTimer += deltaTime;

    const float cycleTime =
        std::fmod(mMoveTimer, std::max(cycleDuration, 0.01f));
    const float destinationWaitEnd =
        oneWayMovementDuration + mEndpointWaitDurationSeconds;
    const float returnMovementEnd =
        destinationWaitEnd + oneWayMovementDuration;
    float interpolation = 0.0f;
    if (cycleTime < oneWayMovementDuration) {
        interpolation = cycleTime / oneWayMovementDuration;
    } else if (cycleTime < destinationWaitEnd) {
        interpolation = 1.0f;
    } else if (cycleTime < returnMovementEnd) {
        const float returnElapsedSeconds =
            cycleTime - destinationWaitEnd;
        interpolation =
            1.0f -
            returnElapsedSeconds / oneWayMovementDuration;
    } else {
        interpolation = 0.0f;
    }
    interpolation = glm::clamp(interpolation, 0.0f, 1.0f);
    SetLocalPosition(mBaseLocalPos + mMoveOffset * interpolation);
}

void PlatformMovementComponent::UpdatePlayerActivatedMovement(float deltaTime)
{
    Game* game = mPlatform ? mPlatform->GetGame() : nullptr;
    if (!game) {
        mFrameDelta = glm::vec3(0.0f);
        return;
    }

    const float elapsedSeconds = std::max(0.0f, deltaTime);
    if (mStartEndpointWaitRemainingSeconds > 0.0f) {
        mStartEndpointWaitRemainingSeconds =
            std::max(
                0.0f,
                mStartEndpointWaitRemainingSeconds -
                    elapsedSeconds);
        mTravelProgress = 0.0f;
        SetLocalPosition(mBaseLocalPos);
        return;
    }

    const bool hasPlayer = HasPlayerOnPlatform();
    const float progressStep =
        elapsedSeconds / std::max(mMoveDuration, 0.01f);

    if (hasPlayer) {
        mHasBeenActivated = true;
        mReturnDelayTimer = 0.0f;
        mTravelProgress = std::min(1.0f, mTravelProgress + progressStep);
    } else if (mHasBeenActivated) {
        const bool isWaitingAtDestination =
            mTravelProgress >= 1.0f &&
            mEndpointWaitElapsedSeconds <
                mEndpointWaitDurationSeconds;
        if (isWaitingAtDestination) {
            mEndpointWaitElapsedSeconds =
                std::min(
                    mEndpointWaitDurationSeconds,
                    mEndpointWaitElapsedSeconds +
                        elapsedSeconds);
        } else {
            mReturnDelayTimer += std::max(0.0f, deltaTime);
        }
        if (!isWaitingAtDestination &&
            mReturnDelayTimer >= mReturnDelay) {
            mTravelProgress = std::max(0.0f, mTravelProgress - progressStep);
            if (mTravelProgress <= 0.0f) {
                mHasBeenActivated = false;
                mReturnDelayTimer = 0.0f;
                mEndpointWaitElapsedSeconds = 0.0f;
                mStartEndpointWaitRemainingSeconds =
                    mEndpointWaitDurationSeconds;
            }
        }
    }

    if (mTravelProgress < 1.0f) {
        mEndpointWaitElapsedSeconds = 0.0f;
    } else if (hasPlayer) {
        mEndpointWaitElapsedSeconds =
            std::min(
                mEndpointWaitDurationSeconds,
                mEndpointWaitElapsedSeconds +
                    elapsedSeconds);
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
