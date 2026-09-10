#include "actor/enemy/EnemyGrounding.h"

#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "system/PhysicsSystem.h"

#include <algorithm>
#include <btBulletDynamicsCommon.h>
#include <cmath>
#include <glm/glm.hpp>

#ifdef max
#undef max
#endif

EnemyGrounding::EnemyGrounding(PhysicsSystem& physicsSystem)
    : mPhysicsSystem(physicsSystem)
{
}

glm::vec3 EnemyGrounding::ClampMoveToGround(const Enemy& enemy, const glm::vec3& desiredPos) const
{
    const glm::vec3 currentPos = enemy.GetPos();
    const glm::vec3 move = desiredPos - currentPos;
    const float moveLength = glm::length(move);

    if (moveLength < 1e-6f) {
        return desiredPos;
    }

    constexpr float checkStep = 0.25f;
    const int checkCount = std::max(1, static_cast<int>(std::ceil(moveLength / checkStep)));
    float lastSafeRatio = 0.0f;

    for (int i = 1; i <= checkCount; ++i) {
        const float checkRatio =
            static_cast<float>(i) /
            static_cast<float>(checkCount);
        const glm::vec3 checkPos =
            glm::mix(currentPos, desiredPos, checkRatio);

        if (!HasGroundBelow(enemy, checkPos)) {




            float safeRatio = lastSafeRatio;
            float unsupportedRatio = checkRatio;
            constexpr int edgeSearchIterations = 10;
            for (int iteration = 0;
                 iteration < edgeSearchIterations;
                 ++iteration) {
                const float middleRatio =
                    (safeRatio + unsupportedRatio) * 0.5f;
                const glm::vec3 middlePos =
                    glm::mix(currentPos, desiredPos, middleRatio);

                if (HasGroundBelow(enemy, middlePos)) {
                    safeRatio = middleRatio;
                } else {
                    unsupportedRatio = middleRatio;
                }
            }

            return glm::mix(currentPos, desiredPos, safeRatio);
        }

        lastSafeRatio = checkRatio;
    }

    return desiredPos;
}

bool EnemyGrounding::HasGroundBelow(const Enemy& enemy, const glm::vec3& checkPos) const
{
    btDiscreteDynamicsWorld* bulletWorld = mPhysicsSystem.GetBulletWorld();
    if (!bulletWorld) {
        return true;
    }

    glm::vec3 groundSearchUp = enemy.GetUpVec();
    const Planet* currentPlanet = enemy.GetCurrentPlanet();
    if (currentPlanet &&
        currentPlanet->GetPlanetShape() ==
            Planet::PlanetShape::Sphere) {
        groundSearchUp = checkPos - currentPlanet->GetPos();
    }

    if (glm::length(groundSearchUp) < 1e-6f) {
        return true;
    }

    const glm::vec3 up = glm::normalize(groundSearchUp);

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
