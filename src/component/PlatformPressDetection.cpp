#include "component/PlatformPressDetection.h"

#include "Game.h"
#include "actor/Actor.h"
#include "actor/Platform.h"
#include "actor/Player.h"
#include "system/PhysicsSystem.h"

#include <algorithm>
#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <iterator>

namespace {

bool DoesSupportRayHitPlatformTop(const Player& player, const Platform& platform, const glm::vec3& rayOffset)
{
    Game* game = platform.GetGame();
    PhysicsSystem* physicsSystem = game ? game->GetPhysicsSystem() : nullptr;
    btDiscreteDynamicsWorld* bulletWorld = physicsSystem ? physicsSystem->GetBulletWorld() : nullptr;
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
    const glm::vec3 rayFrom = player.GetPos() + rayOffset + normalizedUp * rayStartOffset;
    const glm::vec3 rayTo = rayFrom - normalizedUp * rayLength;
    const btVector3 bulletRayFrom(rayFrom.x, rayFrom.y, rayFrom.z);
    const btVector3 bulletRayTo(rayTo.x, rayTo.y, rayTo.z);
    btCollisionWorld::AllHitsRayResultCallback callback(bulletRayFrom, bulletRayTo);
    callback.m_collisionFilterGroup = static_cast<short>(btBroadphaseProxy::DefaultFilter);
    callback.m_collisionFilterMask = static_cast<short>(btBroadphaseProxy::DefaultFilter);
    bulletWorld->rayTest(bulletRayFrom, bulletRayTo, callback);

    for (int hitIndex = 0; hitIndex < callback.m_collisionObjects.size(); ++hitIndex) {
        const btCollisionObject* collisionObject = callback.m_collisionObjects[hitIndex];
        const Actor* hitActor =
            collisionObject ? static_cast<const Actor*>(collisionObject->getUserPointer()) : nullptr;
        if (hitActor != &platform) {
            continue;
        }

        const btVector3& bulletHitNormal = callback.m_hitNormalWorld[hitIndex];
        const glm::vec3 hitNormal(bulletHitNormal.x(), bulletHitNormal.y(), bulletHitNormal.z());
        if (glm::dot(hitNormal, normalizedUp) >= walkableSurfaceMinimumUpDot) {
            return true;
        }
    }

    return false;
}

bool IsPlayerSupportedByPlatform(const Player& player, const Platform& platform)
{
    if (player.GetGroundActor() == &platform) {
        return true;
    }

    PhysicsSystem* physicsSystem = platform.GetGame() ? platform.GetGame()->GetPhysicsSystem() : nullptr;
    if (!physicsSystem) {
        return false;
    }

    // 移動済みの床をレイで調べるため、物理形状の位置を先に同期する。
    physicsSystem->SyncKinematicBodies();

    const float collisionScaleMultiplier = player.GetCollisionScaleMultiplier();
    constexpr float footprintExtentRatio = 0.45f;
    const float forwardRayOffset =
        physicsSystem->GetPlayerCollisionDepth() * collisionScaleMultiplier * footprintExtentRatio;
    const float leftRayOffset =
        physicsSystem->GetPlayerCollisionWidth() * collisionScaleMultiplier * footprintExtentRatio;

    glm::vec3 forwardDirection = player.GetFacingForwardVec();
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

    const glm::vec3 forwardOffset = forwardDirection * forwardRayOffset;
    const glm::vec3 leftOffset = leftDirection * leftRayOffset;

    // 足の中心だけでは床の縁で接触が途切れるため、足元の周囲8点も調べる。
    const glm::vec3 rayOffsets[] = {glm::vec3(0.0f),
                                    forwardOffset,
                                    -forwardOffset,
                                    leftOffset,
                                    -leftOffset,
                                    forwardOffset + leftOffset,
                                    forwardOffset - leftOffset,
                                    -forwardOffset + leftOffset,
                                    -forwardOffset - leftOffset};

    return std::any_of(std::begin(rayOffsets), std::end(rayOffsets), [&player, &platform](const glm::vec3& rayOffset) {
        return DoesSupportRayHitPlatformTop(player, platform, rayOffset);
    });
}

} // namespace

namespace PlatformPressDetection {

bool IsPlayerPressingPlatform(const Player& player, const Platform& platform)
{
    if (!player.GetIsActive() || !IsPlayerSupportedByPlatform(player, platform)) {
        return false;
    }

    if (player.GetOnGround()) {
        return true;
    }

    const glm::vec3 playerUp = player.GetUpVec();
    if (glm::length(playerUp) <= 0.000001f) {
        return true;
    }

    // 接地フラグが途切れても下降中は押下とし、ジャンプ上昇中は解除する。
    const float upwardSpeed = glm::dot(player.GetVelocity(), glm::normalize(playerUp));
    constexpr float maximumPressingUpwardSpeed = 0.05f;
    return upwardSpeed <= maximumPressingUpwardSpeed;
}

Player* FindPlayerPressingPlatform(const Platform* platform)
{
    if (!platform || !platform->GetGame()) {
        return nullptr;
    }

    for (Player* player : platform->GetGame()->GetPlayers()) {
        if (player && IsPlayerPressingPlatform(*player, *platform)) {
            return player;
        }
    }
    return nullptr;
}

} // namespace PlatformPressDetection
