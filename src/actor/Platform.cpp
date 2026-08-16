#include "Platform.h"
#include "component/PlatformBehaviorComponents.h"
#include "component/PlatformMovementComponent.h"

#include <algorithm>
#include <iostream>
#include <memory>

namespace {

const std::string holdSwitchOffTexturePath =
    "textures/platform_switch_hold_off_red_platform_uv.png";
const std::string holdSwitchOnTexturePath =
    "textures/platform_switch_hold_on_blue_platform_uv.png";
const std::string latchedSwitchOffTexturePath =
    "textures/platform_switch_latched_off_red_platform_uv.png";
const std::string latchedSwitchOnTexturePath =
    "textures/platform_switch_latched_on_blue_platform_uv.png";
const std::string twoPlayerSwitchOffTexturePath =
    "textures/platform_switch_two_player_off_red_platform_uv.png";
const std::string twoPlayerSwitchOnTexturePath =
    "textures/platform_switch_two_player_on_blue_platform_uv.png";

}

Platform::Platform(Game* game)
    : Actor(game)
{
    mIsUpVecInitialized = true;
}

void Platform::UpdateActor(float deltaTime)
{
    (void)deltaTime;
    mFrameStartPos = GetPos();
    mFrameStartOrientation = GetOrientation();
    mConveyorFrameDelta = glm::vec3(0.0f);
}

PlatformMovementComponent* Platform::AddMovementComponent()
{
    if (mMovementComponent) {
        return mMovementComponent;
    }

    auto movementComponent =
        std::make_unique<PlatformMovementComponent>(this);
    mMovementComponent = movementComponent.get();
    AddComponent(std::move(movementComponent));
    return mMovementComponent;
}

void Platform::RemoveMovementComponent()
{
    if (!mMovementComponent) {
        return;
    }

    RemoveComponent(mMovementComponent);
    mMovementComponent = nullptr;
}

glm::vec3 Platform::GetFrameDelta() const
{
    return GetPos() - mFrameStartPos;
}

PlatformFadeOnStandComponent* Platform::AddFadeOnStandComponent()
{
    if (mFadeOnStandComponent) return mFadeOnStandComponent;
    auto component = std::make_unique<PlatformFadeOnStandComponent>(this);
    mFadeOnStandComponent = component.get();
    AddComponent(std::move(component));
    return mFadeOnStandComponent;
}

void Platform::RemoveFadeOnStandComponent()
{
    if (!mFadeOnStandComponent) return;
    ClearComponentRuntimeState(mFadeOnStandComponent);
    RemoveComponent(mFadeOnStandComponent);
    mFadeOnStandComponent = nullptr;
}

PlatformJumpToggleComponent* Platform::AddJumpToggleComponent()
{
    if (mJumpToggleComponent) return mJumpToggleComponent;
    auto component = std::make_unique<PlatformJumpToggleComponent>(this);
    mJumpToggleComponent = component.get();
    AddComponent(std::move(component));
    return mJumpToggleComponent;
}

void Platform::RemoveJumpToggleComponent()
{
    if (!mJumpToggleComponent) return;
    ClearComponentRuntimeState(mJumpToggleComponent);
    RemoveComponent(mJumpToggleComponent);
    mJumpToggleComponent = nullptr;
}

PlatformIntervalToggleComponent* Platform::AddIntervalToggleComponent()
{
    if (mIntervalToggleComponent) return mIntervalToggleComponent;
    auto component = std::make_unique<PlatformIntervalToggleComponent>(this);
    mIntervalToggleComponent = component.get();
    AddComponent(std::move(component));
    return mIntervalToggleComponent;
}

void Platform::RemoveIntervalToggleComponent()
{
    if (!mIntervalToggleComponent) return;
    ClearComponentRuntimeState(mIntervalToggleComponent);
    RemoveComponent(mIntervalToggleComponent);
    mIntervalToggleComponent = nullptr;
}

PlatformDirectionalMovementComponent* Platform::AddDirectionalMovementComponent()
{
    if (mDirectionalMovementComponent) return mDirectionalMovementComponent;
    auto component = std::make_unique<PlatformDirectionalMovementComponent>(this);
    mDirectionalMovementComponent = component.get();
    AddComponent(std::move(component));
    return mDirectionalMovementComponent;
}

void Platform::RemoveDirectionalMovementComponent()
{
    if (!mDirectionalMovementComponent) return;
    ClearComponentRuntimeState(mDirectionalMovementComponent);
    RemoveComponent(mDirectionalMovementComponent);
    mDirectionalMovementComponent = nullptr;
}

PlatformRotationComponent* Platform::AddRotationComponent()
{
    if (mRotationComponent) return mRotationComponent;
    auto component = std::make_unique<PlatformRotationComponent>(this);
    mRotationComponent = component.get();
    AddComponent(std::move(component));
    return mRotationComponent;
}

void Platform::RemoveRotationComponent()
{
    if (!mRotationComponent) return;
    ClearComponentRuntimeState(mRotationComponent);
    RemoveComponent(mRotationComponent);
    mRotationComponent = nullptr;
}

PlatformConveyorComponent* Platform::AddConveyorComponent()
{
    if (mConveyorComponent) return mConveyorComponent;
    auto component = std::make_unique<PlatformConveyorComponent>(this);
    mConveyorComponent = component.get();
    AddComponent(std::move(component));
    return mConveyorComponent;
}

void Platform::RemoveConveyorComponent()
{
    if (!mConveyorComponent) return;
    ClearComponentRuntimeState(mConveyorComponent);
    RemoveComponent(mConveyorComponent);
    mConveyorComponent = nullptr;
}

PlatformPressureSwitchComponent* Platform::AddPressureSwitchComponent()
{
    if (mPressureSwitchComponent) return mPressureSwitchComponent;
    auto component =
        std::make_unique<PlatformPressureSwitchComponent>(this);
    mPressureSwitchComponent = component.get();
    AddComponent(std::move(component));
    return mPressureSwitchComponent;
}

void Platform::RemovePressureSwitchComponent()
{
    if (!mPressureSwitchComponent) return;
    mPressureSwitchComponent->ClearTargetRuntimeStates();
    RemoveComponent(mPressureSwitchComponent);
    mPressureSwitchComponent = nullptr;
}

PlatformEnemyClearUnlockComponent*
Platform::AddEnemyClearUnlockComponent()
{
    if (mEnemyClearUnlockComponent) {
        return mEnemyClearUnlockComponent;
    }

    auto component =
        std::make_unique<PlatformEnemyClearUnlockComponent>(this);
    mEnemyClearUnlockComponent = component.get();
    AddComponent(std::move(component));
    return mEnemyClearUnlockComponent;
}

void Platform::RemoveEnemyClearUnlockComponent()
{
    if (!mEnemyClearUnlockComponent) {
        return;
    }

    ClearComponentRuntimeState(mEnemyClearUnlockComponent);
    RemoveComponent(mEnemyClearUnlockComponent);
    mEnemyClearUnlockComponent = nullptr;
}

PlatformLatchedGroupSwitchComponent*
Platform::AddLatchedGroupSwitchComponent()
{
    if (mLatchedGroupSwitchComponent) {
        return mLatchedGroupSwitchComponent;
    }

    auto component =
        std::make_unique<PlatformLatchedGroupSwitchComponent>(this);
    mLatchedGroupSwitchComponent = component.get();
    AddComponent(std::move(component));
    return mLatchedGroupSwitchComponent;
}

void Platform::RemoveLatchedGroupSwitchComponent()
{
    if (!mLatchedGroupSwitchComponent) {
        return;
    }

    mLatchedGroupSwitchComponent->ClearTargetRuntimeStates();
    RemoveComponent(mLatchedGroupSwitchComponent);
    mLatchedGroupSwitchComponent = nullptr;
}

void Platform::SetComponentOpacity(const Component* component, float opacity)
{
    if (!component) return;
    mComponentOpacities[component] = glm::clamp(opacity, 0.0f, 1.0f);
}

void Platform::SetComponentCollisionEnabled(const Component* component, bool enabled)
{
    if (!component) return;
    mComponentCollisionStates[component] = enabled;
}

void Platform::ClearComponentRuntimeState(const Component* component)
{
    if (!component) return;
    mComponentOpacities.erase(component);
    mComponentCollisionStates.erase(component);
}

float Platform::GetRenderOpacity() const
{
    if (GetGame() && GetGame()->GetIsDebugEditorShowing()) {
        return 1.0f;
    }

    float opacity = 1.0f;
    for (const auto& [component, componentOpacity] : mComponentOpacities) {
        (void)component;
        opacity = std::min(opacity, componentOpacity);
    }
    return opacity;
}

glm::vec2 Platform::GetRenderTextureTiling() const
{
    if (mPressureSwitchComponent || mLatchedGroupSwitchComponent) {
        return glm::vec2(1.0f);
    }
    return Actor::GetRenderTextureTiling();
}

const std::string& Platform::GetRenderTextureOverridePath() const
{
    if (mLatchedGroupSwitchComponent) {
        return mLatchedGroupSwitchComponent->GetIsOn()
            ? twoPlayerSwitchOnTexturePath
            : twoPlayerSwitchOffTexturePath;
    }

    if (!mPressureSwitchComponent) {
        return Actor::GetRenderTextureOverridePath();
    }

    const bool shouldRemainOnAfterPressed =
        mPressureSwitchComponent->ShouldRemainOnAfterPressed();
    if (shouldRemainOnAfterPressed) {
        return mPressureSwitchComponent->GetIsPressed()
            ? latchedSwitchOnTexturePath
            : latchedSwitchOffTexturePath;
    }

    return mPressureSwitchComponent->GetIsPressed()
        ? holdSwitchOnTexturePath
        : holdSwitchOffTexturePath;
}

bool Platform::GetCollisionEnabled() const
{
    if (GetGame() && GetGame()->GetIsDebugEditorShowing()) {
        return true;
    }

    for (const auto& [component, enabled] : mComponentCollisionStates) {
        (void)component;
        if (!enabled) return false;
    }
    return true;
}

bool Platform::UsesKinematicPhysics() const
{
    return mMovementComponent ||
           mDirectionalMovementComponent ||
           mRotationComponent;
}

glm::vec3 Platform::GetTransformFrameDelta(const glm::vec3& worldPoint) const
{
    glm::vec3 localPoint = worldPoint - mFrameStartPos;
    if (glm::length(mFrameStartOrientation) > 1e-6f) {
        localPoint = glm::inverse(mFrameStartOrientation) * localPoint;
    }

    const glm::vec3 transformedPoint =
        GetPos() + GetOrientation() * localPoint;
    return transformedPoint - worldPoint;
}
