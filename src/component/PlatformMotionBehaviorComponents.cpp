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


