#include "system/camera/CameraCollisionResolver.h"

#include "Game.h"
#include "system/PhysicsSystem.h"

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

CameraCollisionResolver::CameraCollisionResolver(Game* game)
    : mGame(game)
{
}

glm::vec3 CameraCollisionResolver::Resolve(const glm::vec3& targetPos, const glm::vec3& desiredCameraPos) const
{
    if (!mGame || !mGame->GetPhysicsSystem()) {
        return desiredCameraPos;
    }

    btDiscreteDynamicsWorld* bulletWorld = mGame->GetPhysicsSystem()->GetBulletWorld();
    if (!bulletWorld) {
        return desiredCameraPos;
    }

    const glm::vec3 from = targetPos;
    const glm::vec3 to = desiredCameraPos;

    btCollisionWorld::ClosestRayResultCallback cb(btVector3(from.x, from.y, from.z), btVector3(to.x, to.y, to.z));

    bulletWorld->rayTest(cb.m_rayFromWorld, cb.m_rayToWorld, cb);

    if (!cb.hasHit()) {
        return desiredCameraPos;
    }

    const glm::vec3 hitPos(cb.m_hitPointWorld.x(), cb.m_hitPointWorld.y(), cb.m_hitPointWorld.z());

    glm::vec3 dir = desiredCameraPos - targetPos;
    if (glm::length(dir) < 1e-5f) {
        return desiredCameraPos;
    }

    dir = glm::normalize(dir);

    constexpr float cameraCollisionMargin = 0.3f;
    return hitPos - dir * cameraCollisionMargin;
}
