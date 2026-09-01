#include "system/camera/CameraCollisionResolver.h"

#include "Game.h"
#include "actor/Actor.h"
#include "system/PhysicsSystem.h"

#include <btBulletDynamicsCommon.h>
#include <algorithm>
#include <glm/glm.hpp>

namespace {
bool ShouldIgnoreActor(
    const Actor* actor,
    const Actor* ignoredActor,
    const std::vector<Actor*>* ignoredActors)
{
    if (ignoredActor && actor == ignoredActor) {
        return true;
    }
    return ignoredActors &&
        std::find(ignoredActors->begin(), ignoredActors->end(), actor) !=
            ignoredActors->end();
}

class CameraRayResultCallback : public btCollisionWorld::ClosestRayResultCallback {
public:
    CameraRayResultCallback(
        const btVector3& rayFromWorld,
        const btVector3& rayToWorld,
        const Actor* ignoredActor,
        const std::vector<Actor*>* ignoredActors)
        : btCollisionWorld::ClosestRayResultCallback(rayFromWorld, rayToWorld),
          mIgnoredActor(ignoredActor),
          mIgnoredActors(ignoredActors)
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
        if (ShouldIgnoreActor(actor, mIgnoredActor, mIgnoredActors)) {
            return false;
        }

        return !actor || actor->GetIsActive();
    }

private:
    const Actor* mIgnoredActor;
    const std::vector<Actor*>* mIgnoredActors;
};

class CameraClearanceResultCallback
    : public btCollisionWorld::ContactResultCallback {
public:
    explicit CameraClearanceResultCallback(
        const std::vector<Actor*>& ignoredActors)
        : mIgnoredActors(ignoredActors)
    {
        m_collisionFilterGroup =
            static_cast<short>(btBroadphaseProxy::DefaultFilter);
        m_collisionFilterMask =
            static_cast<short>(btBroadphaseProxy::DefaultFilter);
    }

    bool needsCollision(btBroadphaseProxy* proxy) const override
    {
        if (!btCollisionWorld::ContactResultCallback::needsCollision(proxy)) {
            return false;
        }

        const auto* collisionObject =
            static_cast<const btCollisionObject*>(proxy->m_clientObject);
        if (!collisionObject) {
            return false;
        }

        const Actor* actor = static_cast<const Actor*>(
            collisionObject->getUserPointer());
        if (ShouldIgnoreActor(actor, nullptr, &mIgnoredActors)) {
            return false;
        }

        return !actor || actor->GetIsActive();
    }

    btScalar addSingleResult(
        btManifoldPoint& contactPoint,
        const btCollisionObjectWrapper* object0Wrapper,
        int partId0,
        int index0,
        const btCollisionObjectWrapper* object1Wrapper,
        int partId1,
        int index1) override
    {
        (void)contactPoint;
        (void)object0Wrapper;
        (void)partId0;
        (void)index0;
        (void)object1Wrapper;
        (void)partId1;
        (void)index1;
        mHasContact = true;
        return 0.0f;
    }

    bool HasContact() const { return mHasContact; }

private:
    const std::vector<Actor*>& mIgnoredActors;
    bool mHasContact = false;
};
}

CameraCollisionResolver::CameraCollisionResolver(Game* game)
    : mGame(game)
{
}

glm::vec3 CameraCollisionResolver::Resolve(
    const glm::vec3& targetPos,
    const glm::vec3& desiredCameraPos,
    const Actor* ignoredActor) const
{
    return ResolveInternal(
        targetPos,
        desiredCameraPos,
        ignoredActor,
        nullptr);
}

glm::vec3 CameraCollisionResolver::Resolve(
    const glm::vec3& targetPos,
    const glm::vec3& desiredCameraPos,
    const std::vector<Actor*>& ignoredActors) const
{
    return ResolveInternal(
        targetPos,
        desiredCameraPos,
        nullptr,
        &ignoredActors);
}

bool CameraCollisionResolver::HasClearLineOfSight(
    const glm::vec3& cameraPos,
    const glm::vec3& targetPos,
    const std::vector<Actor*>& ignoredActors) const
{
    if (!mGame || !mGame->GetPhysicsSystem()) {
        return true;
    }

    btDiscreteDynamicsWorld* bulletWorld =
        mGame->GetPhysicsSystem()->GetBulletWorld();
    if (!bulletWorld) {
        return true;
    }

    CameraRayResultCallback callback(
        btVector3(cameraPos.x, cameraPos.y, cameraPos.z),
        btVector3(targetPos.x, targetPos.y, targetPos.z),
        nullptr,
        &ignoredActors);
    bulletWorld->rayTest(
        callback.m_rayFromWorld,
        callback.m_rayToWorld,
        callback);
    return !callback.hasHit();
}

bool CameraCollisionResolver::HasCameraClearance(
    const glm::vec3& cameraPos,
    float clearanceRadius,
    const std::vector<Actor*>& ignoredActors) const
{
    if (!mGame || !mGame->GetPhysicsSystem()) {
        return true;
    }

    PhysicsSystem* physicsSystem = mGame->GetPhysicsSystem();
    btDiscreteDynamicsWorld* bulletWorld = physicsSystem->GetBulletWorld();
    if (!bulletWorld) {
        return true;
    }

    physicsSystem->SyncKinematicBodies();

    btSphereShape cameraClearanceShape(
        std::max(0.05f, clearanceRadius));
    btCollisionObject cameraCollisionObject;
    cameraCollisionObject.setCollisionShape(&cameraClearanceShape);
    btTransform cameraTransform;
    cameraTransform.setIdentity();
    cameraTransform.setOrigin(btVector3(
        cameraPos.x,
        cameraPos.y,
        cameraPos.z));
    cameraCollisionObject.setWorldTransform(cameraTransform);

    CameraClearanceResultCallback callback(ignoredActors);
    bulletWorld->contactTest(&cameraCollisionObject, callback);
    return !callback.HasContact();
}

glm::vec3 CameraCollisionResolver::ResolveInternal(
    const glm::vec3& targetPos,
    const glm::vec3& desiredCameraPos,
    const Actor* ignoredActor,
    const std::vector<Actor*>* ignoredActors) const
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

    CameraRayResultCallback cb(
        btVector3(from.x, from.y, from.z),
        btVector3(to.x, to.y, to.z),
        ignoredActor,
        ignoredActors);

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
