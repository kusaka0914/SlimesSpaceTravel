#include "actor/enemy/EnemyGrounding.h"

#include "Game.h"
#include "actor/Enemy.h"
#include "system/PhysicsSystem.h"

#include <algorithm>
#include <btBulletDynamicsCommon.h>
#include <cmath>
#include <glm/glm.hpp>

#ifdef max
#undef max
#endif

// Keeps enemies from walking past valid ground on curved stages or small platforms.
glm::vec3 EnemyGrounding::ClampMoveToGround(const Enemy& enemy, const glm::vec3& desiredPos) const
{
    const glm::vec3 move = desiredPos - enemy.GetPos();
    const float moveLength = glm::length(move);

    if (moveLength < 1e-6f) {
        return desiredPos;
    }

    constexpr float checkStep = 0.25f;
    const int checkCount = std::max(1, static_cast<int>(std::ceil(moveLength / checkStep)));
    glm::vec3 lastSafePos = enemy.GetPos();

    for (int i = 1; i <= checkCount; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(checkCount);
        const glm::vec3 checkPos = glm::mix(enemy.GetPos(), desiredPos, t);

        if (!HasGroundBelow(enemy, checkPos)) {
            return lastSafePos;
        }

        lastSafePos = checkPos;
    }

    return desiredPos;
}

bool EnemyGrounding::HasGroundBelow(const Enemy& enemy, const glm::vec3& checkPos) const
{
    if (!enemy.GetGame() || !enemy.GetGame()->GetPhysicsSystem()) {
        return true;
    }

    btDiscreteDynamicsWorld* bulletWorld = enemy.GetGame()->GetPhysicsSystem()->GetBulletWorld();
    if (!bulletWorld) {
        return true;
    }

    if (glm::length(enemy.GetUpVec()) < 1e-6f) {
        return true;
    }

    const glm::vec3 up = glm::normalize(enemy.GetUpVec());

    constexpr float rayStartOffset = 0.3f;
    constexpr float rayLength = 1.2f;

    const glm::vec3 rayFromPos = checkPos + up * rayStartOffset;
    const glm::vec3 rayToPos = checkPos - up * rayLength;

    const btVector3 rayFrom(rayFromPos.x, rayFromPos.y, rayFromPos.z);
    const btVector3 rayTo(rayToPos.x, rayToPos.y, rayToPos.z);

    btCollisionWorld::ClosestRayResultCallback rayCallback(rayFrom, rayTo);
    rayCallback.m_collisionFilterGroup = static_cast<short>(btBroadphaseProxy::DefaultFilter);
    rayCallback.m_collisionFilterMask = static_cast<short>(btBroadphaseProxy::DefaultFilter);

    bulletWorld->rayTest(rayFrom, rayTo, rayCallback);

    if (!rayCallback.hasHit()) {
        return false;
    }

    const btVector3 hitNormalBt = rayCallback.m_hitNormalWorld;
    glm::vec3 hitNormal(hitNormalBt.x(), hitNormalBt.y(), hitNormalBt.z());

    if (glm::length(hitNormal) < 1e-6f) {
        return false;
    }

    hitNormal = glm::normalize(hitNormal);

    if (enemy.IsSteepGroundForEnemy(hitNormal, up)) {
        return false;
    }

    return true;
}
