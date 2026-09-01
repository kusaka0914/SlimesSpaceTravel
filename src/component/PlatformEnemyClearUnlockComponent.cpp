#include "component/PlatformEnemyClearUnlockComponent.h"

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

constexpr float pressureSwitchContactReleaseGraceSeconds = 0.15f;

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

bool DoesSupportRayHitPlatformTop(
    const Player& player,
    const Platform& platform,
    const glm::vec3& rayOffset)
{
    Game* game = platform.GetGame();
    PhysicsSystem* physicsSystem = game ? game->GetPhysicsSystem() : nullptr;
    btDiscreteDynamicsWorld* bulletWorld =
        physicsSystem ? physicsSystem->GetBulletWorld() : nullptr;
    if (!physicsSystem || !bulletWorld) {
        return false;
    }

    const glm::vec3 playerUp = player.GetUpVec();
    if (glm::length(playerUp) <= 0.000001f) {
        return false;
    }

    constexpr float rayStartOffset = 0.15f;
    constexpr float rayLength = 0.45f;
    constexpr float walkableSurfaceMinimumUpDot = 0.65f;

    const glm::vec3 normalizedUp = glm::normalize(playerUp);
    const glm::vec3 rayFrom =
        player.GetPos() + rayOffset + normalizedUp * rayStartOffset;
    const glm::vec3 rayTo = rayFrom - normalizedUp * rayLength;
    const btVector3 bulletRayFrom(rayFrom.x, rayFrom.y, rayFrom.z);
    const btVector3 bulletRayTo(rayTo.x, rayTo.y, rayTo.z);
    btCollisionWorld::AllHitsRayResultCallback callback(
        bulletRayFrom,
        bulletRayTo);
    callback.m_collisionFilterGroup =
        static_cast<short>(btBroadphaseProxy::DefaultFilter);
    callback.m_collisionFilterMask =
        static_cast<short>(btBroadphaseProxy::DefaultFilter);
    bulletWorld->rayTest(bulletRayFrom, bulletRayTo, callback);

    for (int hitIndex = 0;
         hitIndex < callback.m_collisionObjects.size();
         ++hitIndex) {
        const btCollisionObject* collisionObject =
            callback.m_collisionObjects[hitIndex];
        const Actor* hitActor =
            collisionObject
            ? static_cast<const Actor*>(collisionObject->getUserPointer())
            : nullptr;
        if (hitActor != &platform) {
            continue;
        }

        const btVector3& bulletHitNormal =
            callback.m_hitNormalWorld[hitIndex];
        const glm::vec3 hitNormal(
            bulletHitNormal.x(),
            bulletHitNormal.y(),
            bulletHitNormal.z());
        if (glm::dot(hitNormal, normalizedUp) >=
            walkableSurfaceMinimumUpDot) {
            return true;
        }
    }

    return false;
}

bool IsPlayerSupportedByPlatform(
    const Player& player,
    const Platform& platform)
{
    if (player.GetGroundActor() == &platform) {
        return true;
    }

    PhysicsSystem* physicsSystem =
        platform.GetGame()
        ? platform.GetGame()->GetPhysicsSystem()
        : nullptr;
    if (!physicsSystem) {
        return false;
    }
    physicsSystem->SyncKinematicBodies();

    const float collisionScaleMultiplier =
        player.GetCollisionScaleMultiplier();
    constexpr float footprintExtentRatio = 0.45f;
    const float forwardRayOffset =
        physicsSystem->GetPlayerCollisionDepth() *
        collisionScaleMultiplier *
        footprintExtentRatio;
    const float leftRayOffset =
        physicsSystem->GetPlayerCollisionWidth() *
        collisionScaleMultiplier *
        footprintExtentRatio;

    glm::vec3 forwardDirection =
        player.GetFacingForwardVec();
    if (glm::length(forwardDirection) > 0.000001f) {
        forwardDirection = glm::normalize(forwardDirection);
    } else {
        forwardDirection = glm::vec3(0.0f);
    }

    glm::vec3 leftDirection = player.GetLeftVec();
    if (glm::length(leftDirection) > 0.000001f) {
        leftDirection = glm::normalize(leftDirection);
    } else {
        leftDirection = glm::vec3(0.0f);
    }

    const glm::vec3 forwardOffset =
        forwardDirection * forwardRayOffset;
    const glm::vec3 leftOffset =
        leftDirection * leftRayOffset;
    const glm::vec3 rayOffsets[] = {
        glm::vec3(0.0f),
        forwardOffset,
        -forwardOffset,
        leftOffset,
        -leftOffset,
        forwardOffset + leftOffset,
        forwardOffset - leftOffset,
        -forwardOffset + leftOffset,
        -forwardOffset - leftOffset};

    return std::any_of(
        std::begin(rayOffsets),
        std::end(rayOffsets),
        [&player, &platform](const glm::vec3& rayOffset) {
            return DoesSupportRayHitPlatformTop(
                player,
                platform,
                rayOffset);
        });
}

bool IsPlayerPressingPlatform(
    const Player& player,
    const Platform& platform)
{
    if (!player.GetIsActive() ||
        !IsPlayerSupportedByPlatform(player, platform)) {
        return false;
    }

    if (player.GetOnGround()) {
        return true;
    }

    const glm::vec3 playerUp = player.GetUpVec();
    if (glm::length(playerUp) <= 0.000001f) {
        return true;
    }

    const float upwardSpeed =
        glm::dot(
            player.GetVelocity(),
            glm::normalize(playerUp));
    constexpr float maximumPressingUpwardSpeed = 0.05f;
    return upwardSpeed <= maximumPressingUpwardSpeed;
}

Player* FindPlayerPressingPlatform(const Platform* platform)
{
    if (!platform || !platform->GetGame()) {
        return nullptr;
    }

    for (Player* player : platform->GetGame()->GetPlayers()) {
        if (player &&
            IsPlayerPressingPlatform(*player, *platform)) {
            return player;
        }
    }
    return nullptr;
}

}

void PlatformEnemyClearUnlockComponent::Update(float deltaTime)
{
    (void)deltaTime;
    if (!mPlatform) {
        return;
    }

    if (IsEditorPreview(mPlatform)) {
        ClearLockedState();
        return;
    }

    if (mIsUnlocked) {
        return;
    }

    if (HasLivingEnemyOnCurrentPlanet()) {
        ApplyLockedState();
        return;
    }

    mIsUnlocked = true;
    ClearLockedState();
}

bool PlatformEnemyClearUnlockComponent::
HasLivingEnemyOnCurrentPlanet() const
{
    const Planet* currentPlanet =
        mPlatform ? mPlatform->GetCurrentPlanet() : nullptr;
    if (!currentPlanet) {
        return false;
    }

    for (const Enemy* enemy : currentPlanet->GetEnemies()) {
        if (enemy && enemy->GetIsActive() &&
            !enemy->GetIsDead()) {
            return true;
        }
    }
    return false;
}

void PlatformEnemyClearUnlockComponent::ApplyLockedState()
{
    if (!mPlatform) {
        return;
    }
    constexpr float lockedSwitchOpacity = 0.2f;
    mPlatform->SetComponentOpacity(this, lockedSwitchOpacity);
    mPlatform->SetComponentCollisionEnabled(this, false);
}

void PlatformEnemyClearUnlockComponent::ClearLockedState()
{
    if (mPlatform) {
        mPlatform->ClearComponentRuntimeState(this);
    }
}

