#pragma once

#include "component/Component.h"

#include <glm/glm.hpp>

class Platform;

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
