#include "system/camera/CameraCollisionResolver.h"

#include "Game.h"
#include "actor/Actor.h"
#include "system/PhysicsSystem.h"

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

namespace {
class CameraRayResultCallback : public btCollisionWorld::ClosestRayResultCallback {
public:
    CameraRayResultCallback(const btVector3& rayFromWorld, const btVector3& rayToWorld)
        : btCollisionWorld::ClosestRayResultCallback(rayFromWorld, rayToWorld)
    {
        m_collisionFilterGroup = static_cast<short>(btBroadphaseProxy::DefaultFilter);
        m_collisionFilterMask = static_cast<short>(btBroadphaseProxy::DefaultFilter);
    }

    bool needsCollision(btBroadphaseProxy* proxy) const override
    {
        if (!btCollisionWorld::ClosestRayResultCallback::needsCollision(proxy)) {
            return false;
        }

        const auto* collisionObject = static_cast<const btCollisionObject*>(proxy->m_clientObject);
        if (!collisionObject) {
            return false;
        }

        const Actor* actor = static_cast<const Actor*>(collisionObject->getUserPointer());
        return !actor || actor->GetIsActive();
    }
};
}

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

    CameraRayResultCallback cb(btVector3(from.x, from.y, from.z), btVector3(to.x, to.y, to.z));

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
