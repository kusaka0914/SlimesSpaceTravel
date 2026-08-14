#pragma once

#include "actor/Actor.h"

#include <string>
#include <unordered_map>

class Game;
class Component;
class PlatformConveyorComponent;
class PlatformDirectionalMovementComponent;
class PlatformEnemyClearUnlockComponent;
class PlatformFadeOnStandComponent;
class PlatformIntervalToggleComponent;
class PlatformJumpToggleComponent;
class PlatformLatchedGroupSwitchComponent;
class PlatformMovementComponent;
class PlatformPressureSwitchComponent;
class PlatformRotationComponent;

class Platform : public Actor {
public:
    Platform(Game* game);
    void UpdateActor(float deltaTime) override;
    bool ShouldUpdateUpVecEveryFrame() const override { return false; }
    bool ShouldRebuildDirectionVectorsEveryFrame() const override { return false; }

    PlatformMovementComponent* AddMovementComponent();
    void RemoveMovementComponent();
    PlatformMovementComponent* GetMovementComponent() const { return mMovementComponent; }

    PlatformFadeOnStandComponent* AddFadeOnStandComponent();
    void RemoveFadeOnStandComponent();
    PlatformFadeOnStandComponent* GetFadeOnStandComponent() const { return mFadeOnStandComponent; }

    PlatformJumpToggleComponent* AddJumpToggleComponent();
    void RemoveJumpToggleComponent();
    PlatformJumpToggleComponent* GetJumpToggleComponent() const { return mJumpToggleComponent; }

    PlatformIntervalToggleComponent* AddIntervalToggleComponent();
    void RemoveIntervalToggleComponent();
    PlatformIntervalToggleComponent* GetIntervalToggleComponent() const { return mIntervalToggleComponent; }

    PlatformDirectionalMovementComponent* AddDirectionalMovementComponent();
    void RemoveDirectionalMovementComponent();
    PlatformDirectionalMovementComponent* GetDirectionalMovementComponent() const
    {
        return mDirectionalMovementComponent;
    }

    PlatformRotationComponent* AddRotationComponent();
    void RemoveRotationComponent();
    PlatformRotationComponent* GetRotationComponent() const { return mRotationComponent; }

    PlatformConveyorComponent* AddConveyorComponent();
    void RemoveConveyorComponent();
    PlatformConveyorComponent* GetConveyorComponent() const { return mConveyorComponent; }

    PlatformPressureSwitchComponent* AddPressureSwitchComponent();
    void RemovePressureSwitchComponent();
    PlatformPressureSwitchComponent* GetPressureSwitchComponent() const
    {
        return mPressureSwitchComponent;
    }

    PlatformEnemyClearUnlockComponent*
    AddEnemyClearUnlockComponent();
    void RemoveEnemyClearUnlockComponent();
    PlatformEnemyClearUnlockComponent*
    GetEnemyClearUnlockComponent() const
    {
        return mEnemyClearUnlockComponent;
    }

    PlatformLatchedGroupSwitchComponent*
    AddLatchedGroupSwitchComponent();
    void RemoveLatchedGroupSwitchComponent();
    PlatformLatchedGroupSwitchComponent*
    GetLatchedGroupSwitchComponent() const
    {
        return mLatchedGroupSwitchComponent;
    }

    void SetPlatformId(const std::string& platformId) { mPlatformId = platformId; }
    const std::string& GetPlatformId() const { return mPlatformId; }

    void SetComponentOpacity(const Component* component, float opacity);
    void SetComponentCollisionEnabled(const Component* component, bool enabled);
    void ClearComponentRuntimeState(const Component* component);
    void SetConveyorFrameDelta(const glm::vec3& delta) { mConveyorFrameDelta = delta; }

    float GetRenderOpacity() const override;
    glm::vec2 GetRenderTextureTiling() const override;
    const std::string& GetRenderTextureOverridePath() const override;
    bool GetCollisionEnabled() const;
    bool UsesKinematicPhysics() const;
    glm::vec3 GetFrameDelta() const;
    glm::vec3 GetTransformFrameDelta(const glm::vec3& worldPoint) const;
    const glm::vec3& GetConveyorFrameDelta() const { return mConveyorFrameDelta; }

private:
    PlatformMovementComponent* mMovementComponent = nullptr;
    PlatformFadeOnStandComponent* mFadeOnStandComponent = nullptr;
    PlatformJumpToggleComponent* mJumpToggleComponent = nullptr;
    PlatformIntervalToggleComponent* mIntervalToggleComponent = nullptr;
    PlatformDirectionalMovementComponent* mDirectionalMovementComponent = nullptr;
    PlatformRotationComponent* mRotationComponent = nullptr;
    PlatformConveyorComponent* mConveyorComponent = nullptr;
    PlatformPressureSwitchComponent* mPressureSwitchComponent = nullptr;
    PlatformEnemyClearUnlockComponent*
        mEnemyClearUnlockComponent = nullptr;
    PlatformLatchedGroupSwitchComponent*
        mLatchedGroupSwitchComponent = nullptr;

    std::string mPlatformId;

    std::unordered_map<const Component*, float> mComponentOpacities;
    std::unordered_map<const Component*, bool> mComponentCollisionStates;

    glm::vec3 mFrameStartPos{0.0f};
    glm::quat mFrameStartOrientation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 mConveyorFrameDelta{0.0f};
};
