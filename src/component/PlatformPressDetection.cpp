#include "component/PlatformPressDetection.h"

#include "actor/Actor.h"
#include "actor/Platform.h"
#include "actor/Player.h"
#include "app/Game.h"
#include "system/PhysicsSystem.h"

#include <algorithm>
#include <btBulletDynamicsCommon.h>
#include <cmath>
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

    // 着地探索用の長いレイではなく、足元の接触誤差だけを許容する。
    const float collisionScale = player.GetCollisionScaleMultiplier();
    const float rayStartOffset = 0.05f * collisionScale;
    const float maximumFootGap = 0.04f * collisionScale;
    constexpr float walkableSurfaceMinimumUpDot = 0.65f;

    const glm::vec3 normalizedUp = glm::normalize(playerUp);
    const glm::vec3 footPosition = player.GetPos() + rayOffset;
    const glm::vec3 rayFrom = footPosition + normalizedUp * rayStartOffset;
    const glm::vec3 rayTo = footPosition - normalizedUp * maximumFootGap;
    const btVector3 bulletRayFrom(rayFrom.x, rayFrom.y, rayFrom.z);
    const btVector3 bulletRayTo(rayTo.x, rayTo.y, rayTo.z);
    btCollisionWorld::ClosestRayResultCallback callback(bulletRayFrom, bulletRayTo);
    callback.m_collisionFilterGroup = static_cast<short>(btBroadphaseProxy::DefaultFilter);
    callback.m_collisionFilterMask = static_cast<short>(btBroadphaseProxy::DefaultFilter);
    bulletWorld->rayTest(bulletRayFrom, bulletRayTo, callback);
    if (!callback.hasHit()) {
        return false;
    }

    const btCollisionObject* collisionObject = callback.m_collisionObject;
    const Actor* hitActor = collisionObject ? static_cast<const Actor*>(collisionObject->getUserPointer()) : nullptr;
    // 別の床を挟んで、その下のスイッチまで押したことにしない。
    if (hitActor != &platform) {
        return false;
    }
    const btVector3& bulletHitNormal = callback.m_hitNormalWorld;
    const glm::vec3 hitNormal(bulletHitNormal.x(), bulletHitNormal.y(), bulletHitNormal.z());
    return glm::dot(hitNormal, normalizedUp) >= walkableSurfaceMinimumUpDot;
}

bool IsPlayerSupportedByPlatform(const Player& player, const Platform& platform)
{
    // 隣の床に接地したまま、足元の周辺レイだけがスイッチへ届いても押下にはしない。
    if (player.GetGroundActor() && player.GetGroundActor() != &platform) {
        return false;
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
    if (!player.GetIsActive() || !player.IsAlive() || player.IsControlLocked() || !player.GetOnGround() ||
        !platform.GetIsActive() || !platform.GetCollisionEnabled()) {
        return false;
    }
    const float upLength = glm::length(player.GetUpVec());
    if (!std::isfinite(upLength) || upLength <= 0.000001f) {
        return false;
    }
    return IsPlayerSupportedByPlatform(player, platform);
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
