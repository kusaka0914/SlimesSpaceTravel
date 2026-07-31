#include "component/PlatformBehaviorComponents.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Player.h"
#include "component/PlatformMovementComponent.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>

namespace {

bool IsEditorPreview(const Platform* platform)
{
    return platform && platform->GetGame() &&
           platform->GetGame()->GetIsDebugEditorShowing();
}

Player* FindPlayerOnPlatform(const Platform* platform)
{
    if (!platform || !platform->GetGame()) return nullptr;

    for (Player* player : platform->GetGame()->GetPlayers()) {
        if (player && player->GetIsActive() && player->GetOnGround() &&
            player->GetGroundActor() == platform) {
            return player;
        }
    }
    return nullptr;
}

std::uint64_t GetTotalPlayerJumpCount(const Platform* platform)
{
    if (!platform || !platform->GetGame()) return 0;

    std::uint64_t total = 0;
    for (Player* player : platform->GetGame()->GetPlayers()) {
        if (player) {
            total += player->GetJumpSequence();
        }
    }
    return total;
}

} // namespace

PlatformFadeOnStandComponent::PlatformFadeOnStandComponent(
    Platform* owner,
    int updateOrder)
    : Component(owner, updateOrder),
      mPlatform(owner)
{
    if (mPlatform) {
        mPlatform->SetComponentOpacity(this, mOpacity);
        mPlatform->SetComponentCollisionEnabled(this, true);
    }
}

void PlatformFadeOnStandComponent::Update(float deltaTime)
{
    if (!mPlatform || IsEditorPreview(mPlatform)) return;

    const float safeDeltaTime = std::max(0.0f, deltaTime);

    if (mWaitingToReappear) {
        mHiddenTimer += safeDeltaTime;
        mOpacity = 0.0f;
        mCollisionEnabled = false;
        mPlatform->SetComponentOpacity(this, mOpacity);
        mPlatform->SetComponentCollisionEnabled(this, false);

        if (mHiddenTimer < mReappearDelay) {
            return;
        }

        mWaitingToReappear = false;
        mHiddenTimer = 0.0f;
        mOpacity = 1.0f;
        mCollisionEnabled = true;
        mPlatform->SetComponentOpacity(this, mOpacity);
        mPlatform->SetComponentCollisionEnabled(this, true);
        return;
    }

    const bool occupied = FindPlayerOnPlatform(mPlatform) != nullptr;
    if (occupied) {
        const float step =
            safeDeltaTime / std::max(0.05f, mFadeOutDuration);
        mOpacity = glm::clamp(mOpacity - step, 0.0f, 1.0f);
    } else {
        mOpacity = 1.0f;
        mCollisionEnabled = true;
    }

    if (occupied && mOpacity <= 0.001f) {
        mOpacity = 0.0f;
        mCollisionEnabled = false;
        mWaitingToReappear = true;
        mHiddenTimer = 0.0f;
    }

    mPlatform->SetComponentOpacity(this, mOpacity);
    mPlatform->SetComponentCollisionEnabled(
        this,
        mCollisionEnabled);
}

void PlatformFadeOnStandComponent::SetFadeOutDuration(float duration)
{
    mFadeOutDuration = std::max(0.05f, duration);
}

void PlatformFadeOnStandComponent::SetReappearDelay(float delay)
{
    mReappearDelay = std::max(0.0f, delay);
}

PlatformJumpToggleComponent::PlatformJumpToggleComponent(
    Platform* owner,
    int updateOrder)
    : Component(owner, updateOrder),
      mPlatform(owner)
{
    mLastObservedJumpCount = GetTotalPlayerJumpCount(mPlatform);
    ApplyState();
}

void PlatformJumpToggleComponent::Update(float deltaTime)
{
    (void)deltaTime;
    if (!mPlatform || IsEditorPreview(mPlatform)) return;

    const std::uint64_t jumpCount =
        GetTotalPlayerJumpCount(mPlatform);
    if (jumpCount != mLastObservedJumpCount) {
        mVisible = !mVisible;
        ApplyState();
    }
    mLastObservedJumpCount = jumpCount;
}

void PlatformJumpToggleComponent::SetInitiallyVisible(bool visible)
{
    mInitiallyVisible = visible;
    mVisible = visible;
    ApplyState();
}

void PlatformJumpToggleComponent::ApplyState()
{
    if (!mPlatform) return;
    mPlatform->SetComponentOpacity(this, mVisible ? 1.0f : 0.0f);
    mPlatform->SetComponentCollisionEnabled(this, mVisible);
}

PlatformIntervalToggleComponent::PlatformIntervalToggleComponent(
    Platform* owner,
    int updateOrder)
    : Component(owner, updateOrder),
      mPlatform(owner)
{
    ApplyState(mVisible ? 1.0f : 0.0f);
}

void PlatformIntervalToggleComponent::Update(float deltaTime)
{
    if (!mPlatform || IsEditorPreview(mPlatform)) return;

    mTimer += std::max(0.0f, deltaTime);
    if (mTimer >= mInterval) {
        mTimer = std::fmod(mTimer, mInterval);
        mVisible = !mVisible;
    }

    float opacity = mVisible ? 1.0f : 0.0f;
    const float warningStart =
        std::max(0.0f, mInterval - mWarningDuration);
    if (mVisible &&
        mTimer >= warningStart &&
        mWarningDuration > 0.0f) {
        const int blinkPhase = static_cast<int>(
            (mTimer - warningStart) / std::max(0.03f, mBlinkInterval));
        const bool showCurrentState = (blinkPhase % 2) == 0;
        opacity = showCurrentState ? 1.0f : 0.15f;
    }

    ApplyState(opacity);
}

void PlatformIntervalToggleComponent::SetInitiallyVisible(bool visible)
{
    mInitiallyVisible = visible;
    mVisible = visible;
    mTimer = 0.0f;
    ApplyState(mVisible ? 1.0f : 0.0f);
}

void PlatformIntervalToggleComponent::SetInterval(float interval)
{
    mInterval = std::max(0.1f, interval);
    mTimer = std::fmod(mTimer, mInterval);
}

void PlatformIntervalToggleComponent::SetWarningDuration(float duration)
{
    mWarningDuration = std::max(0.0f, duration);
}

void PlatformIntervalToggleComponent::SetBlinkInterval(float interval)
{
    mBlinkInterval = std::max(0.03f, interval);
}

void PlatformIntervalToggleComponent::ApplyState(float opacity)
{
    if (!mPlatform) return;
    mPlatform->SetComponentOpacity(this, opacity);
    mPlatform->SetComponentCollisionEnabled(this, mVisible);
}

PlatformDirectionalMovementComponent::PlatformDirectionalMovementComponent(
    Platform* owner,
    int updateOrder)
    : Component(owner, updateOrder),
      mPlatform(owner)
{
}

void PlatformDirectionalMovementComponent::Update(float deltaTime)
{
    if (!mPlatform || IsEditorPreview(mPlatform)) return;

    Player* player = FindPlayerOnPlatform(mPlatform);
    const bool occupied = player != nullptr;

    if (occupied && !mWasOccupied) {
        const glm::vec3 localOffset =
            glm::inverse(mPlatform->GetOrientation()) *
            (player->GetPos() - mPlatform->GetPos());

        glm::vec3 localDirection(0.0f);
        if (std::abs(localOffset.z) >= std::abs(localOffset.x)) {
            localDirection.z = localOffset.z >= 0.0f ? 1.0f : -1.0f;
        } else {
            localDirection.x = localOffset.x >= 0.0f ? 1.0f : -1.0f;
        }

        glm::vec3 worldDirection =
            mPlatform->GetOrientation() * localDirection;
        worldDirection.y = 0.0f;

        if (glm::length(worldDirection) > 1e-6f) {
            mTravelDirection = glm::normalize(worldDirection);
        } else {
            mTravelDirection = glm::vec3(0.0f);
        }
    }

    if (!occupied) {
        mTravelDirection = glm::vec3(0.0f);
    }

    mWasOccupied = occupied;
    if (occupied && glm::length(mTravelDirection) > 1e-6f) {
        const glm::vec3 movementDelta =
            mTravelDirection * mSpeed * std::max(0.0f, deltaTime);
        if (PlatformMovementComponent* movement =
                mPlatform->GetMovementComponent()) {
            movement->TranslatePath(movementDelta);
        }
        mPlatform->SetPos(
            mPlatform->GetPos() + movementDelta);
    }
}

void PlatformDirectionalMovementComponent::SetSpeed(float speed)
{
    mSpeed = std::max(0.0f, speed);
}

PlatformRotationComponent::PlatformRotationComponent(
    Platform* owner,
    int updateOrder)
    : Component(owner, updateOrder),
      mPlatform(owner)
{
}

void PlatformRotationComponent::Update(float deltaTime)
{
    if (!mPlatform || IsEditorPreview(mPlatform) ||
        glm::length(mLocalAxis) < 1e-6f) {
        return;
    }

    const float angle =
        glm::radians(mDegreesPerSecond) * std::max(0.0f, deltaTime);
    const glm::quat deltaRotation =
        glm::angleAxis(angle, glm::normalize(mLocalAxis));
    mPlatform->SetOrientation(
        glm::normalize(mPlatform->GetOrientation() * deltaRotation));
}

void PlatformRotationComponent::SetLocalAxis(const glm::vec3& axis)
{
    mLocalAxis =
        glm::length(axis) > 1e-6f
            ? glm::normalize(axis)
            : glm::vec3(0.0f, 1.0f, 0.0f);
}

PlatformConveyorComponent::PlatformConveyorComponent(
    Platform* owner,
    int updateOrder)
    : Component(owner, updateOrder),
      mPlatform(owner)
{
}

void PlatformConveyorComponent::Update(float deltaTime)
{
    if (!mPlatform || IsEditorPreview(mPlatform)) return;

    glm::vec3 worldDirection =
        mPlatform->GetOrientation() * mLocalDirection;
    if (glm::length(worldDirection) < 1e-6f) {
        mPlatform->SetConveyorFrameDelta(glm::vec3(0.0f));
        return;
    }

    mPlatform->SetConveyorFrameDelta(
        glm::normalize(worldDirection) *
        mSpeed * std::max(0.0f, deltaTime));
}

void PlatformConveyorComponent::SetLocalDirection(const glm::vec3& direction)
{
    mLocalDirection =
        glm::length(direction) > 1e-6f
            ? glm::normalize(direction)
            : glm::vec3(0.0f, 0.0f, 1.0f);
}

void PlatformConveyorComponent::SetSpeed(float speed)
{
    mSpeed = std::max(0.0f, speed);
}

PlatformPressureSwitchComponent::PlatformPressureSwitchComponent(
    Platform* owner,
    int updateOrder)
    : Component(owner, updateOrder),
      mPlatform(owner)
{
}

PlatformPressureSwitchComponent::~PlatformPressureSwitchComponent()
{
}

void PlatformPressureSwitchComponent::Update(float deltaTime)
{
    (void)deltaTime;
    if (!mPlatform || IsEditorPreview(mPlatform)) return;

    const bool isPressed =
        FindPlayerOnPlatform(mPlatform) != nullptr;
    if (isPressed != mIsPressed) {
        mIsPressed = isPressed;
    }
    ApplyTargetState();
}

void PlatformPressureSwitchComponent::SetTargetPlatformIds(
    const std::vector<std::string>& targetPlatformIds)
{
    if (!mPlatform) {
        mTargetPlatformIds.clear();
        return;
    }

    ClearTargetRuntimeStates();

    mTargetPlatformIds.clear();
    for (const std::string& platformId : targetPlatformIds) {
        if (platformId.empty() ||
            platformId == mPlatform->GetPlatformId() ||
            std::find(
                mTargetPlatformIds.begin(),
                mTargetPlatformIds.end(),
                platformId) != mTargetPlatformIds.end()) {
            continue;
        }
        mTargetPlatformIds.emplace_back(platformId);
    }

    mIsPressed =
        !IsEditorPreview(mPlatform) &&
        FindPlayerOnPlatform(mPlatform) != nullptr;
    ApplyTargetState();
}

void PlatformPressureSwitchComponent::SetInactiveOpacity(float opacity)
{
    mInactiveOpacity = glm::clamp(opacity, 0.0f, 1.0f);
    ApplyTargetState();
}

Platform* PlatformPressureSwitchComponent::FindTargetPlatform(
    const std::string& platformId) const
{
    if (!mPlatform || platformId.empty() ||
        !mPlatform->GetGame() ||
        !mPlatform->GetGame()->GetCurrentStage()) {
        return nullptr;
    }

    for (Planet* planet :
         mPlatform->GetGame()->GetCurrentStage()->GetPlanets()) {
        if (!planet) continue;
        for (Platform* platform : planet->GetPlatforms()) {
            if (platform && platform->GetIsActive() &&
                platform != mPlatform &&
                platform->GetPlatformId() == platformId) {
                return platform;
            }
        }
    }
    return nullptr;
}

void PlatformPressureSwitchComponent::ApplyTargetState()
{
    for (const std::string& platformId : mTargetPlatformIds) {
        Platform* target = FindTargetPlatform(platformId);
        if (!target) continue;
        target->SetComponentOpacity(
            this,
            mIsPressed ? 1.0f : mInactiveOpacity);
        target->SetComponentCollisionEnabled(this, mIsPressed);
    }
}

void PlatformPressureSwitchComponent::ClearTargetRuntimeStates()
{
    for (const std::string& platformId : mTargetPlatformIds) {
        Platform* target = FindTargetPlatform(platformId);
        if (target) {
            target->ClearComponentRuntimeState(this);
        }
    }
}
