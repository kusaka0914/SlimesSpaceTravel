#pragma once

#include "actor/Actor.h"

#include <unordered_map>

class Game;
class Component;
class PlatformConveyorComponent;
class PlatformDirectionalMovementComponent;
class PlatformFadeOnStandComponent;
class PlatformIntervalToggleComponent;
class PlatformJumpToggleComponent;
class PlatformMovementComponent;
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

    void SetComponentOpacity(const Component* component, float opacity);
    void SetComponentCollisionEnabled(const Component* component, bool enabled);
    void ClearComponentRuntimeState(const Component* component);
    void SetConveyorFrameDelta(const glm::vec3& delta) { mConveyorFrameDelta = delta; }

    float GetRenderOpacity() const override;
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

    std::unordered_map<const Component*, float> mComponentOpacities;
    std::unordered_map<const Component*, bool> mComponentCollisionStates;

    glm::vec3 mFrameStartPos{0.0f};
    glm::quat mFrameStartOrientation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 mConveyorFrameDelta{0.0f};
};
