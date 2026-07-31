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
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Star.h"
#include "actor/StageObject.h"
#include "actor/TutorialTrigger.h"
#include "system/MeshLoadSystem.h"
#include "system/physics/StageCollisionBuilder.h"

#include <algorithm>
#include <btBulletDynamicsCommon.h>
#include <memory>
#include <unordered_map>

EditorPickSystem::EditorPickSystem(Game* game)
    : mGame(game)
{
}

void EditorPickSystem::CreatePickBodies(btDiscreteDynamicsWorld* world,
                                        std::vector<std::unique_ptr<btCollisionObject>>& pickObjects,
                                        std::vector<std::unique_ptr<btCollisionShape>>& pickShapes,
                                        std::vector<std::unique_ptr<btTriangleMesh>>& pickTriangleMeshes) const
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
            CreatePickBody(world, enemy, pickObjects, pickShapes, pickTriangleMeshes);
        }

        for (Platform* platform : planet->GetPlatforms()) {
            CreatePickBody(world, platform, pickObjects, pickShapes, pickTriangleMeshes);
        }

        if (Key* key = planet->GetKey()) {
            CreatePickBody(world, key, pickObjects, pickShapes, pickTriangleMeshes);
        }

        for (Boat* boat : planet->GetBoats()) {
            CreatePickBody(world, boat, pickObjects, pickShapes, pickTriangleMeshes);
        }

        for (BoatParts* boatParts : planet->GetBoatParts()) {
            CreatePickBody(world, boatParts, pickObjects, pickShapes, pickTriangleMeshes);
        }

        for (Crystal* crystal : planet->GetCrystals()) {
            CreatePickBody(world, crystal, pickObjects, pickShapes, pickTriangleMeshes);
        }

        for (NPC* npc : planet->GetNPCs()) {
            CreatePickBody(world, npc, pickObjects, pickShapes, pickTriangleMeshes);
        }

        for (TutorialTrigger* trigger :
             planet->GetTutorialTriggers()) {
            CreatePickBody(
                world,
                trigger,
                pickObjects,
                pickShapes,
                pickTriangleMeshes);
        }

        if (Star* star = planet->GetStar()) {
            CreatePickBody(world, star, pickObjects, pickShapes, pickTriangleMeshes);
        }

        for (BoatArrivalPoint* arrivalPoint : planet->GetBoatArrivalPoints()) {
            CreatePickBody(world, arrivalPoint, pickObjects, pickShapes, pickTriangleMeshes);
        }

        for (FallRespawnPoint* fallRespawnPoint : planet->GetFallRespawnPoints()) {
            CreatePickBody(world, fallRespawnPoint, pickObjects, pickShapes, pickTriangleMeshes);
        }

        for (StageObject* stageObject : planet->GetStageObjects()) {
            CreatePickBody(world, stageObject, pickObjects, pickShapes, pickTriangleMeshes);
        }
    }
}

void EditorPickSystem::CreatePickBody(btDiscreteDynamicsWorld* world, Actor* actor,
                                      std::vector<std::unique_ptr<btCollisionObject>>& pickObjects,
                                      std::vector<std::unique_ptr<btCollisionShape>>& pickShapes,
                                      std::vector<std::unique_ptr<btTriangleMesh>>& pickTriangleMeshes) const
{
    if (!actor || actor->IsDebugDisabled() || !world) {
        return;
    }

    const bool useMesh =
        dynamic_cast<Platform*>(actor) ||
        dynamic_cast<StageObject*>(actor) ||
        dynamic_cast<TutorialTrigger*>(actor);

    if (useMesh && CreateMeshPickBody(world, actor, pickObjects, pickShapes, pickTriangleMeshes)) {
        return;
    }

    CreateSpherePickBody(world, actor, pickObjects, pickShapes);
}

void EditorPickSystem::CreateSpherePickBody(btDiscreteDynamicsWorld* world, Actor* actor,
                                            std::vector<std::unique_ptr<btCollisionObject>>& pickObjects,
                                            std::vector<std::unique_ptr<btCollisionShape>>& pickShapes) const
{
    const glm::vec3 scale = actor->GetScale();
    const float maxScale = std::max(scale.x, std::max(scale.y, scale.z));
    const float radius = std::max({actor->GetRadius(), maxScale * 0.5f, 0.5f});

    auto shape = std::make_unique<btSphereShape>(radius);
    auto object = std::make_unique<btCollisionObject>();

    object->setWorldTransform(StageCollisionBuilder::CreateActorTransform(mGame, actor));
    object->setCollisionShape(shape.get());
    object->setUserPointer(actor);
    object->setCollisionFlags(object->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);

    world->addCollisionObject(object.get(), static_cast<short>(btBroadphaseProxy::SensorTrigger),
                              static_cast<short>(btBroadphaseProxy::DefaultFilter));

    pickShapes.emplace_back(std::move(shape));
    pickObjects.emplace_back(std::move(object));
}

bool EditorPickSystem::CreateMeshPickBody(btDiscreteDynamicsWorld* world, Actor* actor,
                                          std::vector<std::unique_ptr<btCollisionObject>>& pickObjects,
                                          std::vector<std::unique_ptr<btCollisionShape>>& pickShapes,
                                          std::vector<std::unique_ptr<btTriangleMesh>>& pickTriangleMeshes) const
{
    if (!mGame || !mGame->GetMeshLoadSystem()) {
        return false;
    }

    const std::string modelPath = "../assets/models/" + actor->GetModelPath();
    std::vector<float> positions;
    std::vector<unsigned int> indices;

    if (!mGame->GetMeshLoadSystem()->LoadMeshPositionsAndIndices(modelPath.c_str(), positions, indices) ||
        positions.size() < 9 || indices.size() < 3) {
        return false;
    }

    auto triangleMesh = CreateTriangleMesh(actor->GetScale(), positions, indices);
    if (!triangleMesh) {
        return false;
    }

    auto shape = std::make_unique<btBvhTriangleMeshShape>(triangleMesh.get(), true);
    auto object = std::make_unique<btCollisionObject>();

    object->setWorldTransform(StageCollisionBuilder::CreateActorTransform(mGame, actor));
    object->setCollisionShape(shape.get());
    object->setUserPointer(actor);
    object->setCollisionFlags(object->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);

    world->addCollisionObject(object.get(), static_cast<short>(btBroadphaseProxy::SensorTrigger),
                              static_cast<short>(btBroadphaseProxy::DefaultFilter));

    pickTriangleMeshes.emplace_back(std::move(triangleMesh));
    pickShapes.emplace_back(std::move(shape));
    pickObjects.emplace_back(std::move(object));
    return true;
}

std::unique_ptr<btTriangleMesh>
EditorPickSystem::CreateTriangleMesh(const glm::vec3& actorScale, const std::vector<float>& positions,
                                     const std::vector<unsigned int>& indices) const
{
    auto triangleMesh = std::make_unique<btTriangleMesh>();

    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        const unsigned int index0 = indices[i];
        const unsigned int index1 = indices[i + 1];
        const unsigned int index2 = indices[i + 2];

        if (index0 * 3 + 2 >= positions.size() || index1 * 3 + 2 >= positions.size() ||
            index2 * 3 + 2 >= positions.size()) {
            return nullptr;
        }

        const btVector3 vertex0(actorScale.x * positions[index0 * 3], actorScale.y * positions[index0 * 3 + 1],
                                actorScale.z * positions[index0 * 3 + 2]);
        const btVector3 vertex1(actorScale.x * positions[index1 * 3], actorScale.y * positions[index1 * 3 + 1],
                                actorScale.z * positions[index1 * 3 + 2]);
        const btVector3 vertex2(actorScale.x * positions[index2 * 3], actorScale.y * positions[index2 * 3 + 1],
                                actorScale.z * positions[index2 * 3 + 2]);

        triangleMesh->addTriangle(vertex0, vertex1, vertex2);
    }

    return triangleMesh;
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

        object->setWorldTransform(StageCollisionBuilder::CreateActorTransform(mGame, actor));
        world->updateSingleAabb(object.get());
    }
}

std::optional<PhysicsSystem::RayHitActor>
EditorPickSystem::PickActorByRay(btDiscreteDynamicsWorld* world, const glm::vec3& rayFrom, const glm::vec3& rayTo,
                                 const std::vector<std::unique_ptr<btCollisionObject>>& pickObjects) const
{
    const std::vector<PhysicsSystem::RayHitActor> hits = PickActorsByRay(world, rayFrom, rayTo, pickObjects);
    if (hits.empty()) {
        return std::nullopt;
    }

    return hits.front();
}

std::vector<PhysicsSystem::RayHitActor>
EditorPickSystem::PickActorsByRay(btDiscreteDynamicsWorld* world, const glm::vec3& rayFrom,
                                  const glm::vec3& rayTo,
                                  const std::vector<std::unique_ptr<btCollisionObject>>& pickObjects) const
{
    std::vector<PhysicsSystem::RayHitActor> hits;
    if (!world) {
        return hits;
    }

    SyncPickBodies(world, pickObjects);

    const btVector3 btFrom(rayFrom.x, rayFrom.y, rayFrom.z);
    const btVector3 btTo(rayTo.x, rayTo.y, rayTo.z);
    btCollisionWorld::AllHitsRayResultCallback callback(btFrom, btTo);

    callback.m_collisionFilterGroup = static_cast<short>(btBroadphaseProxy::DefaultFilter);
    callback.m_collisionFilterMask = static_cast<short>(btBroadphaseProxy::SensorTrigger);
    world->rayTest(btFrom, btTo, callback);

    if (!callback.hasHit()) {
        return hits;
    }

    std::unordered_map<Actor*, PhysicsSystem::RayHitActor> nearestHits;

    for (int i = 0; i < callback.m_collisionObjects.size(); ++i) {
        const btCollisionObject* object = callback.m_collisionObjects[i];
        if (!object) {
            continue;
        }

        Actor* actor = static_cast<Actor*>(object->getUserPointer());
        if (!actor || dynamic_cast<Planet*>(actor)) {
            continue;
        }

        const btVector3 hitPoint = callback.m_hitPointWorld[i];
        const btVector3 hitNormal = callback.m_hitNormalWorld[i];

        PhysicsSystem::RayHitActor hit;
        hit.actor = actor;
        hit.hitPos = glm::vec3(hitPoint.x(), hitPoint.y(), hitPoint.z());
        hit.hitNormal = glm::vec3(hitNormal.x(), hitNormal.y(), hitNormal.z());
        hit.distance = glm::length(hit.hitPos - rayFrom);

        auto existing = nearestHits.find(actor);
        if (existing == nearestHits.end() || hit.distance < existing->second.distance) {
            nearestHits[actor] = hit;
        }
    }

    hits.reserve(nearestHits.size());
    for (const auto& [actor, hit] : nearestHits) {
        hits.emplace_back(hit);
    }

    std::sort(hits.begin(), hits.end(),
              [](const PhysicsSystem::RayHitActor& lhs, const PhysicsSystem::RayHitActor& rhs) {
                  return lhs.distance < rhs.distance;
              });
    return hits;
}
