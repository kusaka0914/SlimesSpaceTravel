#include "component/PlatformBehaviorComponents.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Boat.h"
#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Player.h"
#include "component/PlatformMovementComponent.h"
#include "system/ActorLoadSystem.h"
#include "system/PhysicsSystem.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iterator>

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

}

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

    if (mFadePhase == FadePhase::WaitingToReappear) {
        mHiddenTimer += safeDeltaTime;
        mOpacity = 0.0f;
        mCollisionEnabled = false;
        mPlatform->SetComponentOpacity(this, mOpacity);
        mPlatform->SetComponentCollisionEnabled(this, false);

        if (mHiddenTimer < mReappearDelay) {
            return;
        }

        mFadePhase = FadePhase::Visible;
        mHiddenTimer = 0.0f;
        mOpacity = 1.0f;
        mCollisionEnabled = true;
        mPlatform->SetComponentOpacity(this, mOpacity);
        mPlatform->SetComponentCollisionEnabled(this, true);
        return;
    }

    if (mFadePhase == FadePhase::Visible &&
        FindPlayerOnPlatform(mPlatform)) {
        mFadePhase = FadePhase::FadingOut;
    }

    if (mFadePhase == FadePhase::FadingOut) {
        const float step =
            safeDeltaTime / std::max(0.05f, mFadeOutDuration);
        mOpacity = glm::clamp(mOpacity - step, 0.0f, 1.0f);
    } else {
        mOpacity = 1.0f;
        mCollisionEnabled = true;
    }

    if (mFadePhase == FadePhase::FadingOut &&
        mOpacity <= 0.001f) {
        mOpacity = 0.0f;
        mCollisionEnabled = false;
        mFadePhase = FadePhase::WaitingToReappear;
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


