#include "PhysicsSystem.h"
#include "Game.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Crystal.h"
#include "actor/Enemy.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Player.h"
#include "system/MeshLoadSystem.h"
#include "utils/MathUtils.h"
#include <btBulletDynamicsCommon.h>

PhysicsSystem::PhysicsSystem(Game* game)
    : mGame(game)
{
}

PhysicsSystem::~PhysicsSystem()
{
    ClearBulletWorld();
}

void PhysicsSystem::Initialize()
{
    ClearBulletWorld();

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();
    if (planets.empty())
        return;

    mBulletCollisionConfig = std::make_unique<btDefaultCollisionConfiguration>();
    mBulletDispatcher = std::make_unique<btCollisionDispatcher>(mBulletCollisionConfig.get());
    mBulletBroadphase = std::make_unique<btDbvtBroadphase>();
    mBulletSolver = std::make_unique<btSequentialImpulseConstraintSolver>();
    mBulletWorld = std::make_unique<btDiscreteDynamicsWorld>(mBulletDispatcher.get(), mBulletBroadphase.get(),
                                                             mBulletSolver.get(), mBulletCollisionConfig.get());
    mBulletWorld->setGravity(btVector3(0, -9.8f, 0));

    CreateWorld();
}

void PhysicsSystem::ClearBulletWorld()
{
    if (mBulletWorld) {
        for (const auto& pickObject : mEditorPickObjects) {
            if (pickObject) {
                mBulletWorld->removeCollisionObject(pickObject.get());
            }
        }

        for (const auto& rigidBody : mBulletRigidBodies) {
            if (rigidBody) {
                mBulletWorld->removeRigidBody(rigidBody.get());
            }
        }
    }

    mEditorPickObjects.clear();
    mEditorPickShapes.clear();

    // world から外した後に所有物を破棄
    mPlayerShape.reset();
    mBulletRigidBodies.clear();
    mBulletTriangleMeshShapes.clear();
    mBulletTriangleMeshes.clear();

    // world は依存先より先に破棄する
    mBulletWorld.reset();

    mBulletSolver.reset();
    mBulletBroadphase.reset();
    mBulletDispatcher.reset();
    mBulletCollisionConfig.reset();
}

void PhysicsSystem::CreateWorld()
{
    CreateStageCollisionBodies();
    CreateEditorPickBodies();
    CreatePlayerShape();
}

void PhysicsSystem::CreateStageCollisionBodies()
{
    const std::vector<Planet*> planets = mGame->GetCurrentStage()->GetPlanets();
    for (auto planet : planets) {
        CreateStaticMeshBody(planet);

        std::vector<Platform*> platforms = planet->GetPlatforms();
        for (auto platform : platforms)
            CreateStaticMeshBody(platform);
    }
}

void PhysicsSystem::CreateEditorPickBodies()
{
    if (!mGame || !mGame->GetCurrentStage() || !mBulletWorld) {
        return;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    for (Planet* planet : planets) {
        if (!planet) {
            continue;
        }

        for (Enemy* enemy : planet->GetEnemies()) {
            CreateEditorPickBody(enemy);
        }

        for (Crystal* crystal : planet->GetCrystals()) {
            CreateEditorPickBody(crystal);
        }
    }
}

void PhysicsSystem::CreateEditorPickBody(Actor* actor)
{
    if (!actor || !mBulletWorld) {
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

    mBulletWorld->addCollisionObject(object.get(), static_cast<short>(btBroadphaseProxy::SensorTrigger),
                                     static_cast<short>(btBroadphaseProxy::DefaultFilter));

    mEditorPickShapes.emplace_back(std::move(shape));
    mEditorPickObjects.emplace_back(std::move(object));
}

void PhysicsSystem::SyncEditorPickBodies() const
{
    for (const auto& object : mEditorPickObjects) {
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
    }
}

void PhysicsSystem::CreatePlayerShape()
{
    constexpr float playerRadius = 0.6f;
    mPlayerShape = std::make_unique<btSphereShape>(playerRadius);
}

void PhysicsSystem::CreateStaticMeshBody(Actor* actor)
{
    const std::string& actorModelPath = "../assets/models/" + actor->GetModelPath();

    std::vector<float> pos;
    std::vector<unsigned int> idx;

    if (!mGame->GetMeshLoadSystem()->LoadMeshPositionsAndIndices(actorModelPath.c_str(), pos, idx) || pos.size() < 9 ||
        idx.size() < 3)
        return;

    const glm::vec3& actorScale = actor->GetScale();
    auto triangleMesh = CreateTriangleMesh(actorScale, pos, idx);
    if (!triangleMesh)
        return;

    auto triangleMeshShape = std::make_unique<btBvhTriangleMeshShape>(triangleMesh.get(), true);

    btRigidBody::btRigidBodyConstructionInfo rigidBodyConstructionInfo(0, nullptr, triangleMeshShape.get());

    auto rigidBody = std::make_unique<btRigidBody>(rigidBodyConstructionInfo);
    rigidBody->setUserPointer(actor);

    btTransform actorTransform;
    actorTransform.setIdentity();
    const glm::vec3& actorPos = actor->GetPos();
    actorTransform.setOrigin(btVector3(actorPos.x, actorPos.y, actorPos.z));

    if (dynamic_cast<Platform*>(actor)) {
        glm::mat4 orient = mGame->GetMathUtils()->CreateOrient(actor);

        glm::vec3 axisX = glm::normalize(glm::vec3(orient[0]));
        glm::vec3 axisY = glm::normalize(glm::vec3(orient[1]));
        glm::vec3 axisZ = glm::normalize(glm::vec3(orient[2]));

        btMatrix3x3 basis(axisX.x, axisY.x, axisZ.x, axisX.y, axisY.y, axisZ.y, axisX.z, axisY.z, axisZ.z);

        actorTransform.setBasis(basis);
    }

    rigidBody->setWorldTransform(actorTransform);
    rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);

    mBulletTriangleMeshes.emplace_back(std::move(triangleMesh));
    mBulletTriangleMeshShapes.emplace_back(std::move(triangleMeshShape));
    mBulletRigidBodies.emplace_back(std::move(rigidBody));

    mBulletWorld->addRigidBody(mBulletRigidBodies.back().get(), static_cast<short>(btBroadphaseProxy::DefaultFilter),
                               static_cast<short>(-1));
}

std::optional<PhysicsSystem::RayHitActor> PhysicsSystem::PickActorByRay(const glm::vec3& rayFrom,
                                                                        const glm::vec3& rayTo) const
{
    if (!mBulletWorld) {
        return std::nullopt;
    }

    SyncEditorPickBodies();

    const btVector3 btFrom(rayFrom.x, rayFrom.y, rayFrom.z);
    const btVector3 btTo(rayTo.x, rayTo.y, rayTo.z);

    btCollisionWorld::AllHitsRayResultCallback cb(btFrom, btTo);

    cb.m_collisionFilterGroup = static_cast<short>(btBroadphaseProxy::DefaultFilter);
    cb.m_collisionFilterMask = static_cast<short>(btBroadphaseProxy::DefaultFilter | btBroadphaseProxy::SensorTrigger);

    mBulletWorld->rayTest(btFrom, btTo, cb);

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

    RayHitActor hit;
    hit.actor = hitActor;

    const btVector3 hitPoint = cb.m_hitPointWorld[bestIndex];
    const btVector3 hitNormal = cb.m_hitNormalWorld[bestIndex];

    hit.hitPos = glm::vec3(hitPoint.x(), hitPoint.y(), hitPoint.z());
    hit.hitNormal = glm::vec3(hitNormal.x(), hitNormal.y(), hitNormal.z());
    hit.distance = glm::length(hit.hitPos - rayFrom);

    return hit;
}

std::unique_ptr<btTriangleMesh> PhysicsSystem::CreateTriangleMesh(const glm::vec3& actorScale,
                                                                  const std::vector<float>& pos,
                                                                  const std::vector<unsigned int>& idx)
{
    auto triangleMesh = std::make_unique<btTriangleMesh>();
    for (size_t i = 0; i + 2 < idx.size(); i += 3) {
        const unsigned int idx0 = idx[i];
        const unsigned int idx1 = idx[i + 1];
        const unsigned int idx2 = idx[i + 2];

        if (idx0 * 3 + 2 >= pos.size() || idx1 * 3 + 2 >= pos.size() || idx2 * 3 + 2 >= pos.size())
            return nullptr;

        const btVector3 v0(actorScale.x * pos[idx0 * 3], actorScale.y * pos[idx0 * 3 + 1],
                           actorScale.z * pos[idx0 * 3 + 2]);
        const btVector3 v1(actorScale.x * pos[idx1 * 3], actorScale.y * pos[idx1 * 3 + 1],
                           actorScale.z * pos[idx1 * 3 + 2]);
        const btVector3 v2(actorScale.x * pos[idx2 * 3], actorScale.y * pos[idx2 * 3 + 1],
                           actorScale.z * pos[idx2 * 3 + 2]);

        triangleMesh->addTriangle(v0, v1, v2);
    }
    return triangleMesh;
}

glm::vec3 PhysicsSystem::CheckCollision(Actor* actor, const glm::vec3& moveDelta, const glm::vec3& desiredPos)
{
    if (auto conflictPos = CheckConflictActors(actor, desiredPos))
        return *conflictPos;

    if (!mBulletWorld || !mPlayerShape)
        return desiredPos;

    if (auto conflictPos = CheckConflictWall(actor, moveDelta, desiredPos))
        return *conflictPos;

    return desiredPos;
}

std::optional<glm::vec3> PhysicsSystem::CheckConflictActors(Actor* actor, const glm::vec3& desiredPos)
{
    std::vector<Enemy*> enemies = actor->GetCurrentPlanet()->GetEnemies();

    for (Enemy* enemy : enemies) {
        if (enemy == actor) {
            continue;
        }

        if (auto conflictPos = CheckConflictActor(enemy, desiredPos)) {
            return *conflictPos;
        }
    }

    std::vector<Crystal*> crystals = actor->GetCurrentPlanet()->GetCrystals();
    for (Crystal* crystal : crystals) {
        if (crystal == actor) {
            continue;
        }

        if (auto conflictPos = CheckConflictActor(crystal, desiredPos)) {
            return *conflictPos;
        }
    }

    std::vector<NPC*> npcs = actor->GetCurrentPlanet()->GetNPCs();
    for (NPC* npc : npcs) {
        if (npc == actor) {
            continue;
        }

        if (auto conflictPos = CheckConflictActor(npc, desiredPos)) {
            return *conflictPos;
        }
    }

    return std::nullopt;
}

std::optional<glm::vec3> PhysicsSystem::CheckConflictActor(Actor* actor, const glm::vec3& desiredPos)
{
    if (!actor->GetIsActive()) {
        return std::nullopt;
    }

    const glm::vec3 actorPos = actor->GetPos();
    const glm::vec3 toDesired = desiredPos - actorPos;

    const float dist = glm::length(toDesired);
    const float radius = actor->GetRadius();

    if (dist < radius && dist > 1e-5f)
        return actorPos + glm::normalize(toDesired) * radius;

    return std::nullopt;
}

std::optional<glm::vec3> PhysicsSystem::CheckConflictWall(Actor* actor, const glm::vec3& moveDelta,
                                                          const glm::vec3& desiredPos)
{
    glm::vec3 currentPos = actor->GetPos();
    glm::vec3 actorUpVec = actor->GetUpVec();
    constexpr float actorUpMargin = 0.7f;
    glm::vec3 sweepFrom = currentPos + actorUpVec * actorUpMargin;
    glm::vec3 sweepTo = desiredPos + actorUpVec * actorUpMargin;

    btTransform fromTransform, toTransform;
    fromTransform.setIdentity();
    fromTransform.setOrigin(btVector3(sweepFrom.x, sweepFrom.y, sweepFrom.z));
    toTransform.setIdentity();
    toTransform.setOrigin(btVector3(sweepTo.x, sweepTo.y, sweepTo.z));

    btCollisionWorld::ClosestConvexResultCallback sweepCallback(fromTransform.getOrigin(), toTransform.getOrigin());

    sweepCallback.m_collisionFilterGroup = static_cast<short>(btBroadphaseProxy::DefaultFilter);
    sweepCallback.m_collisionFilterMask = static_cast<short>(btBroadphaseProxy::DefaultFilter);
    mBulletWorld->convexSweepTest(mPlayerShape.get(), fromTransform, toTransform, sweepCallback);

    if (!sweepCallback.hasHit())
        return std::nullopt;

    const float allowFrac = std::max(0.0f, sweepCallback.m_closestHitFraction - 0.01f);
    const glm::vec3 posAfterHit = currentPos + moveDelta * allowFrac;
    const glm::vec3 hitNormGlm(sweepCallback.m_hitNormalWorld.x(), sweepCallback.m_hitNormalWorld.y(),
                               sweepCallback.m_hitNormalWorld.z());

    const glm::vec3 blocked = moveDelta * (1.0f - allowFrac);
    const glm::vec3 slideVec = blocked - hitNormGlm * glm::dot(blocked, hitNormGlm);
    const float slideEps = 1e-4f;
    if (glm::length(slideVec) > slideEps) {
        glm::vec3 slideFrom = posAfterHit + actorUpVec * actorUpMargin;
        glm::vec3 slideTo = slideFrom + slideVec;

        btTransform slideFromTransition, slideToTransition;
        slideFromTransition.setIdentity();
        slideFromTransition.setOrigin(btVector3(slideFrom.x, slideFrom.y, slideFrom.z));
        slideToTransition.setIdentity();
        slideToTransition.setOrigin(btVector3(slideTo.x, slideTo.y, slideTo.z));

        btCollisionWorld::ClosestConvexResultCallback slideCallback(slideFromTransition.getOrigin(),
                                                                    slideToTransition.getOrigin());

        slideCallback.m_collisionFilterGroup = static_cast<short>(btBroadphaseProxy::DefaultFilter);
        slideCallback.m_collisionFilterMask = static_cast<short>(btBroadphaseProxy::DefaultFilter);

        mBulletWorld->convexSweepTest(mPlayerShape.get(), slideFromTransition, slideToTransition, slideCallback);

        if (!slideCallback.hasHit())
            return posAfterHit + slideVec;

        const float slideAllow = std::max(0.0f, slideCallback.m_closestHitFraction - 0.01f);
        return posAfterHit + slideVec * slideAllow;
    } else {
        return posAfterHit;
    }
}