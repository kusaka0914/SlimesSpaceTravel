#pragma once

#include "component/Component.h"

#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <vector>

class Platform;

class PlatformFadeOnStandComponent : public Component {
public:
    explicit PlatformFadeOnStandComponent(Platform* owner, int updateOrder = 70);
    void Update(float deltaTime) override;

    void SetFadeOutDuration(float duration);
    void SetReappearDelay(float delay);
    float GetFadeOutDuration() const { return mFadeOutDuration; }
    float GetReappearDelay() const { return mReappearDelay; }

private:
    Platform* mPlatform = nullptr;
    float mOpacity = 1.0f;
    float mFadeOutDuration = 1.0f;
    float mReappearDelay = 2.0f;
    float mHiddenTimer = 0.0f;
    bool mCollisionEnabled = true;
    bool mWaitingToReappear = false;
};

class PlatformJumpToggleComponent : public Component {
public:
    explicit PlatformJumpToggleComponent(Platform* owner, int updateOrder = 71);
    void Update(float deltaTime) override;

    void SetInitiallyVisible(bool visible);
    bool GetInitiallyVisible() const { return mInitiallyVisible; }
    bool GetVisible() const { return mVisible; }

private:
    void ApplyState();

    Platform* mPlatform = nullptr;
    bool mInitiallyVisible = true;
    bool mVisible = true;
    std::uint64_t mLastObservedJumpCount = 0;
};

class PlatformIntervalToggleComponent : public Component {
public:
    explicit PlatformIntervalToggleComponent(Platform* owner, int updateOrder = 72);
    void Update(float deltaTime) override;

    void SetInitiallyVisible(bool visible);
    void SetInterval(float interval);
    void SetWarningDuration(float duration);
    void SetBlinkInterval(float interval);

    bool GetInitiallyVisible() const { return mInitiallyVisible; }
    float GetInterval() const { return mInterval; }
    float GetWarningDuration() const { return mWarningDuration; }
    float GetBlinkInterval() const { return mBlinkInterval; }

private:
    void ApplyState(float opacity);

    Platform* mPlatform = nullptr;
    bool mInitiallyVisible = true;
    bool mVisible = true;
    float mTimer = 0.0f;
    float mInterval = 3.0f;
    float mWarningDuration = 1.0f;
    float mBlinkInterval = 0.15f;
};

class PlatformDirectionalMovementComponent : public Component {
public:
    explicit PlatformDirectionalMovementComponent(Platform* owner, int updateOrder = 55);
    void Update(float deltaTime) override;

    void SetSpeed(float speed);
    float GetSpeed() const { return mSpeed; }

private:
    Platform* mPlatform = nullptr;
    glm::vec3 mTravelDirection{0.0f};
    float mSpeed = 2.0f;
    bool mWasOccupied = false;
};

class PlatformRotationComponent : public Component {
public:
    explicit PlatformRotationComponent(Platform* owner, int updateOrder = 60);
    void Update(float deltaTime) override;

    void SetLocalAxis(const glm::vec3& axis);
    void SetDegreesPerSecond(float speed) { mDegreesPerSecond = speed; }
    const glm::vec3& GetLocalAxis() const { return mLocalAxis; }
    float GetDegreesPerSecond() const { return mDegreesPerSecond; }

private:
    Platform* mPlatform = nullptr;
    glm::vec3 mLocalAxis{0.0f, 1.0f, 0.0f};
    float mDegreesPerSecond = 45.0f;
};

class PlatformConveyorComponent : public Component {
public:
    explicit PlatformConveyorComponent(Platform* owner, int updateOrder = 80);
    void Update(float deltaTime) override;

    void SetLocalDirection(const glm::vec3& direction);
    void SetSpeed(float speed);
    const glm::vec3& GetLocalDirection() const { return mLocalDirection; }
    float GetSpeed() const { return mSpeed; }

private:
    Platform* mPlatform = nullptr;
    glm::vec3 mLocalDirection{0.0f, 0.0f, 1.0f};
    float mSpeed = 2.0f;
};

class PlatformPressureSwitchComponent : public Component {
public:
    explicit PlatformPressureSwitchComponent(
        Platform* owner,
        int updateOrder = 85);
    ~PlatformPressureSwitchComponent() override;

    void Update(float deltaTime) override;

    void SetTargetPlatformIds(
        const std::vector<std::string>& targetPlatformIds);
    const std::vector<std::string>& GetTargetPlatformIds() const
    {
        return mTargetPlatformIds;
    }
    void SetInactiveOpacity(float opacity);
    float GetInactiveOpacity() const { return mInactiveOpacity; }
    bool GetIsPressed() const { return mIsPressed; }

    void ClearTargetRuntimeStates();

private:
    Platform* FindTargetPlatform(const std::string& platformId) const;
    void ApplyTargetState();

    Platform* mPlatform = nullptr;
    std::vector<std::string> mTargetPlatformIds;
    float mInactiveOpacity = 0.2f;
    bool mIsPressed = false;
};
