#include "system/physics/FallRespawnTriggerSystem.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/FallRespawnPoint.h"
#include "actor/Planet.h"
#include "system/physics/StageCollisionBuilder.h"

#include <btBulletDynamicsCommon.h>
#include <memory>

namespace {
btTransform CreatePlayerSweepTransform(
    const Actor& actor,
    const glm::vec3& position)
{
    const glm::quat& actorOrientation = actor.GetOrientation();

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(position.x, position.y, position.z));
    transform.setRotation(
        btQuaternion(
            actorOrientation.x,
            actorOrientation.y,
            actorOrientation.z,
            actorOrientation.w));
    return transform;
}
}

FallRespawnTriggerSystem::FallRespawnTriggerSystem(Game* game)
    : mGame(game)
{
}

void FallRespawnTriggerSystem::CreateTriggerBodies(
    btDiscreteDynamicsWorld* world, std::vector<std::unique_ptr<btCollisionObject>>& triggerObjects,
    std::vector<std::unique_ptr<btCollisionShape>>& triggerShapes) const
{
    if (!mGame || !mGame->GetCurrentStage() || !world) {
        return;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    for (Planet* planet : planets) {
        if (!planet) {
            continue;
        }

        for (FallRespawnPoint* point : planet->GetFallRespawnPoints()) {
            CreateTriggerBody(world, point, triggerObjects, triggerShapes);
        }
    }
}

void FallRespawnTriggerSystem::CreateTriggerBody(
    btDiscreteDynamicsWorld* world, FallRespawnPoint* point,
    std::vector<std::unique_ptr<btCollisionObject>>& triggerObjects,
    std::vector<std::unique_ptr<btCollisionShape>>& triggerShapes) const
{
    if (!point || !point->GetIsActive() || !world) {
        return;
    }

    const glm::vec3 scale = point->GetScale();

    auto shape = std::make_unique<btBoxShape>(btVector3(scale.x, scale.y, scale.z));
    auto object = std::make_unique<btCollisionObject>();

    const btTransform transform = StageCollisionBuilder::CreateActorTransform(mGame, point);
    object->setWorldTransform(transform);

    object->setCollisionShape(shape.get());
    object->setUserPointer(point);

    object->setCollisionFlags(object->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);

    world->addCollisionObject(object.get(), static_cast<short>(btBroadphaseProxy::SensorTrigger),
                              static_cast<short>(btBroadphaseProxy::DefaultFilter));

    triggerShapes.emplace_back(std::move(shape));
    triggerObjects.emplace_back(std::move(object));
}

void FallRespawnTriggerSystem::SyncTriggerBodies(
    btDiscreteDynamicsWorld* world, const std::vector<std::unique_ptr<btCollisionObject>>& triggerObjects) const
{
    if (!world) {
        return;
    }

    for (const auto& object : triggerObjects) {
        if (!object) {
            continue;
        }

        Actor* actor = static_cast<Actor*>(object->getUserPointer());

        if (!actor) {
            continue;
        }

        const btTransform transform = StageCollisionBuilder::CreateActorTransform(mGame, actor);
        object->setWorldTransform(transform);
        world->updateSingleAabb(object.get());
    }
}

std::optional<PhysicsSystem::RayHitActor> FallRespawnTriggerSystem::CheckFallRespawnBySweep(
    btDiscreteDynamicsWorld* world,
    btConvexShape* playerShape,
    const Actor* actor,
    const glm::vec3& from,
    const glm::vec3& to,
    const std::vector<std::unique_ptr<btCollisionObject>>& triggerObjects) const
{
    if (!world || !playerShape || !actor) {
        return std::nullopt;
    }

    if (glm::length(to - from) < 1e-5f) {
        return std::nullopt;
    }

    SyncTriggerBodies(world, triggerObjects);

    const btTransform fromTransform =
        CreatePlayerSweepTransform(*actor, from);
    const btTransform toTransform =
        CreatePlayerSweepTransform(*actor, to);

    class FallRespawnSweepCallback : public btCollisionWorld::ClosestConvexResultCallback {
    public:
        FallRespawnSweepCallback(const btVector3& from, const btVector3& to)
            : btCollisionWorld::ClosestConvexResultCallback(from, to)
        {
        }

        btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult, bool normalInWorldSpace) override
        {
            Actor* actor = static_cast<Actor*>(convexResult.m_hitCollisionObject->getUserPointer());

            if (!dynamic_cast<FallRespawnPoint*>(actor)) {
                return 1.0f;
            }

            return btCollisionWorld::ClosestConvexResultCallback::addSingleResult(convexResult, normalInWorldSpace);
        }
    };

    FallRespawnSweepCallback cb(fromTransform.getOrigin(), toTransform.getOrigin());

    cb.m_collisionFilterGroup = static_cast<short>(btBroadphaseProxy::DefaultFilter);
    cb.m_collisionFilterMask = static_cast<short>(btBroadphaseProxy::SensorTrigger);

    world->convexSweepTest(playerShape, fromTransform, toTransform, cb);

    if (!cb.hasHit()) {
        return std::nullopt;
    }

    Actor* hitActor = static_cast<Actor*>(cb.m_hitCollisionObject->getUserPointer());

    if (!hitActor) {
        return std::nullopt;
    }

    PhysicsSystem::RayHitActor hit;
    hit.actor = hitActor;

    const btVector3 hitPoint = cb.m_hitPointWorld;
    const btVector3 hitNormal = cb.m_hitNormalWorld;

    hit.hitPos = glm::vec3(hitPoint.x(), hitPoint.y(), hitPoint.z());
    hit.hitNormal = glm::vec3(hitNormal.x(), hitNormal.y(), hitNormal.z());
    hit.distance = glm::length(hit.hitPos - from);

    return hit;
}
