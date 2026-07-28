#include "system/physics/EditorPickSystem.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Boat.h"
#include "actor/BoatArrivalPoint.h"
#include "actor/BoatParts.h"
#include "actor/Crystal.h"
#include "actor/Enemy.h"
#include "actor/FallRespawnPoint.h"
#include "actor/Key.h"
#include "actor/MovingPlatform.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Star.h"
#include "actor/StageObject.h"

#include <algorithm>
#include <btBulletDynamicsCommon.h>
#include <limits>
#include <memory>

EditorPickSystem::EditorPickSystem(Game* game)
    : mGame(game)
{
}

void EditorPickSystem::CreatePickBodies(btDiscreteDynamicsWorld* world,
                                        std::vector<std::unique_ptr<btCollisionObject>>& pickObjects,
                                        std::vector<std::unique_ptr<btCollisionShape>>& pickShapes) const
{
    if (!mGame || !mGame->GetCurrentStage() || !world) {
        return;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    for (Planet* planet : planets) {
        if (!planet) {
            continue;
        }

        for (Enemy* enemy : planet->GetEnemies()) {
            CreatePickBody(world, enemy, pickObjects, pickShapes);
        }

        for (Platform* platform : planet->GetPlatforms()) {
            CreatePickBody(world, platform, pickObjects, pickShapes);
        }

        for (MovingPlatform* movingPlatform : planet->GetMovingPlatforms()) {
            CreatePickBody(world, movingPlatform, pickObjects, pickShapes);
        }

        if (Key* key = planet->GetKey()) {
            CreatePickBody(world, key, pickObjects, pickShapes);
        }

        for (Boat* boat : planet->GetBoats()) {
            CreatePickBody(world, boat, pickObjects, pickShapes);
        }

        for (BoatParts* boatParts : planet->GetBoatParts()) {
            CreatePickBody(world, boatParts, pickObjects, pickShapes);
        }

        for (Crystal* crystal : planet->GetCrystals()) {
            CreatePickBody(world, crystal, pickObjects, pickShapes);
        }

        for (NPC* npc : planet->GetNPCs()) {
            CreatePickBody(world, npc, pickObjects, pickShapes);
        }

        if (Star* star = planet->GetStar()) {
            CreatePickBody(world, star, pickObjects, pickShapes);
        }

        for (BoatArrivalPoint* arrivalPoint : planet->GetBoatArrivalPoints()) {
            CreatePickBody(world, arrivalPoint, pickObjects, pickShapes);
        }

        for (FallRespawnPoint* fallRespawnPoint : planet->GetFallRespawnPoints()) {
            CreatePickBody(world, fallRespawnPoint, pickObjects, pickShapes);
        }

        for (StageObject* stageObject : planet->GetStageObjects()) {
            CreatePickBody(world, stageObject, pickObjects, pickShapes);
        }
    }
}

void EditorPickSystem::CreatePickBody(btDiscreteDynamicsWorld* world, Actor* actor,
                                      std::vector<std::unique_ptr<btCollisionObject>>& pickObjects,
                                      std::vector<std::unique_ptr<btCollisionShape>>& pickShapes) const
{
    if (!actor || !world) {
        return;
    }

    const glm::vec3 scale = actor->GetScale();
    const float maxScale = std::max(scale.x, std::max(scale.y, scale.z));
    const float radius = std::max({actor->GetRadius(), maxScale * 0.5f, 0.5f});

    auto shape = std::make_unique<btSphereShape>(radius);
    auto object = std::make_unique<btCollisionObject>();

    btTransform transform;
    transform.setIdentity();

    const glm::vec3 pos = actor->GetPos();
    transform.setOrigin(btVector3(pos.x, pos.y, pos.z));

    object->setWorldTransform(transform);
    object->setCollisionShape(shape.get());
    object->setUserPointer(actor);

    object->setCollisionFlags(object->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);

    world->addCollisionObject(object.get(), static_cast<short>(btBroadphaseProxy::SensorTrigger),
                              static_cast<short>(btBroadphaseProxy::DefaultFilter));

    pickShapes.emplace_back(std::move(shape));
    pickObjects.emplace_back(std::move(object));
}

void EditorPickSystem::SyncPickBodies(btDiscreteDynamicsWorld* world,
                                      const std::vector<std::unique_ptr<btCollisionObject>>& pickObjects) const
{
    if (!world) {
        return;
    }

    for (const auto& object : pickObjects) {
        if (!object) {
            continue;
        }

        Actor* actor = static_cast<Actor*>(object->getUserPointer());
        if (!actor) {
            continue;
        }

        btTransform transform = object->getWorldTransform();

        const glm::vec3 pos = actor->GetPos();
        transform.setOrigin(btVector3(pos.x, pos.y, pos.z));

        object->setWorldTransform(transform);
        world->updateSingleAabb(object.get());
    }
}

std::optional<PhysicsSystem::RayHitActor>
EditorPickSystem::PickActorByRay(btDiscreteDynamicsWorld* world, const glm::vec3& rayFrom, const glm::vec3& rayTo,
                                 const std::vector<std::unique_ptr<btCollisionObject>>& pickObjects) const
{
    if (!world) {
        return std::nullopt;
    }

    SyncPickBodies(world, pickObjects);

    const btVector3 btFrom(rayFrom.x, rayFrom.y, rayFrom.z);
    const btVector3 btTo(rayTo.x, rayTo.y, rayTo.z);

    btCollisionWorld::AllHitsRayResultCallback cb(btFrom, btTo);

    cb.m_collisionFilterGroup = static_cast<short>(btBroadphaseProxy::DefaultFilter);
    cb.m_collisionFilterMask = static_cast<short>(btBroadphaseProxy::DefaultFilter | btBroadphaseProxy::SensorTrigger);

    world->rayTest(btFrom, btTo, cb);

    if (!cb.hasHit()) {
        return std::nullopt;
    }

    int bestIndex = -1;
    float bestFraction = std::numeric_limits<float>::max();

    for (int i = 0; i < cb.m_collisionObjects.size(); ++i) {
        const btCollisionObject* obj = cb.m_collisionObjects[i];

        if (!obj) {
            continue;
        }

        Actor* actor = static_cast<Actor*>(obj->getUserPointer());

        if (!actor) {
            continue;
        }

        // 惑星はマウス選択対象から外す
        if (dynamic_cast<Planet*>(actor)) {
            continue;
        }

        const float fraction = cb.m_hitFractions[i];

        if (fraction < bestFraction) {
            bestFraction = fraction;
            bestIndex = i;
        }
    }

    if (bestIndex < 0) {
        return std::nullopt;
    }

    Actor* hitActor = static_cast<Actor*>(cb.m_collisionObjects[bestIndex]->getUserPointer());

    if (!hitActor) {
        return std::nullopt;
    }

    PhysicsSystem::RayHitActor hit;
    hit.actor = hitActor;

    const btVector3 hitPoint = cb.m_hitPointWorld[bestIndex];
    const btVector3 hitNormal = cb.m_hitNormalWorld[bestIndex];

    hit.hitPos = glm::vec3(hitPoint.x(), hitPoint.y(), hitPoint.z());
    hit.hitNormal = glm::vec3(hitNormal.x(), hitNormal.y(), hitNormal.z());
    hit.distance = glm::length(hit.hitPos - rayFrom);

    return hit;
}
