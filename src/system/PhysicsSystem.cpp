#include "system/PhysicsSystem.h"

#include "Game.h"
#include "Stage.h"
#include "system/physics/ActorCollisionResolver.h"
#include "system/physics/EditorPickSystem.h"
#include "system/physics/FallRespawnTriggerSystem.h"
#include "system/physics/PhysicsWorldBuilder.h"
#include "system/physics/StageCollisionBuilder.h"

#include <algorithm>
#include <btBulletDynamicsCommon.h>
#include <cmath>
#include <memory>

PhysicsSystem::PhysicsSystem(Game* game)
    : mGame(game)
{
    mWorldBuilder = std::make_unique<PhysicsWorldBuilder>();
    mStageCollisionBuilder = std::make_unique<StageCollisionBuilder>(mGame);
    mEditorPickSystem = std::make_unique<EditorPickSystem>(mGame);
    mFallRespawnTriggerSystem = std::make_unique<FallRespawnTriggerSystem>(mGame);
    mActorCollisionResolver = std::make_unique<ActorCollisionResolver>();
}

PhysicsSystem::~PhysicsSystem()
{
    ClearBulletWorld();
}

void PhysicsSystem::Initialize()
{
    ClearBulletWorld();

    if (!mGame || !mGame->GetCurrentStage()) {
        return;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();
    if (planets.empty()) {
        return;
    }

    mWorldBuilder->CreateBulletWorld(mBulletCollisionConfig, mBulletDispatcher, mBulletBroadphase, mBulletSolver,
                                     mBulletWorld);

    if (!mBulletWorld) {
        return;
    }

    CreateWorld();
}

void PhysicsSystem::ClearBulletWorld()
{
    mWorldBuilder->RemoveObjectsFromWorld(mBulletWorld.get(), mFallRespawnTriggerObjects, mEditorPickObjects,
                                          mBulletRigidBodies);

    mFallRespawnTriggerObjects.clear();
    mFallRespawnTriggerShapes.clear();

    mEditorPickObjects.clear();
    mEditorPickShapes.clear();
    mEditorPickTriangleMeshes.clear();

    // world から外した後に所有物を破棄
    mPlayerShape.reset();
    mBulletRigidBodies.clear();
    mBulletTriangleMeshShapes.clear();
    mBulletTriangleMeshes.clear();

    // world は依存先より先に破棄する
    mWorldBuilder->ResetBulletWorld(mBulletCollisionConfig, mBulletDispatcher, mBulletBroadphase, mBulletSolver,
                                    mBulletWorld);
}

void PhysicsSystem::CreateWorld()
{
    mStageCollisionBuilder->CreateStageCollisionBodies(mBulletWorld.get(), mBulletRigidBodies,
                                                       mBulletTriangleMeshShapes, mBulletTriangleMeshes);
    mFallRespawnTriggerSystem->CreateTriggerBodies(mBulletWorld.get(), mFallRespawnTriggerObjects,
                                                   mFallRespawnTriggerShapes);
    mEditorPickSystem->CreatePickBodies(mBulletWorld.get(), mEditorPickObjects, mEditorPickShapes,
                                        mEditorPickTriangleMeshes);
    CreatePlayerShape();
}

void PhysicsSystem::CreatePlayerShape()
{
    constexpr int latitudeSegmentCount = 8;
    constexpr int longitudeSegmentCount = 16;
    constexpr float pi = 3.14159265358979323846f;

    const float halfWidth = mPlayerCollisionWidth * 0.5f;
    const float halfHeight = mPlayerCollisionHeight * 0.5f;
    const float halfDepth = mPlayerCollisionDepth * 0.5f;

    auto ellipsoidShape = std::make_unique<btConvexHullShape>();
    ellipsoidShape->setMargin(0.0f);

    for (int latitudeIndex = 0;
         latitudeIndex <= latitudeSegmentCount;
         ++latitudeIndex) {
        const float latitudeRadians =
            -0.5f * pi +
            pi * static_cast<float>(latitudeIndex) /
                static_cast<float>(latitudeSegmentCount);
        const float heightRatio = std::sin(latitudeRadians);
        const float ringRadiusRatio = std::cos(latitudeRadians);

        for (int longitudeIndex = 0;
             longitudeIndex < longitudeSegmentCount;
             ++longitudeIndex) {
            const float longitudeRadians =
                2.0f * pi * static_cast<float>(longitudeIndex) /
                static_cast<float>(longitudeSegmentCount);
            const btVector3 surfacePoint(
                halfWidth * ringRadiusRatio * std::cos(longitudeRadians),
                halfHeight * heightRatio,
                halfDepth * ringRadiusRatio * std::sin(longitudeRadians));
            ellipsoidShape->addPoint(surfacePoint, false);
        }
    }

    ellipsoidShape->recalcLocalAabb();
    mPlayerShape = std::move(ellipsoidShape);
}

void PhysicsSystem::SetPlayerCollisionWidth(float width)
{
    constexpr float minimumDimension = 0.1f;
    mPlayerCollisionWidth = std::max(width, minimumDimension);
    CreatePlayerShape();
}

void PhysicsSystem::SetPlayerCollisionHeight(float height)
{
    constexpr float minimumDimension = 0.1f;
    mPlayerCollisionHeight = std::max(height, minimumDimension);
    CreatePlayerShape();
}

void PhysicsSystem::SetPlayerCollisionDepth(float depth)
{
    constexpr float minimumDimension = 0.1f;
    mPlayerCollisionDepth = std::max(depth, minimumDimension);
    CreatePlayerShape();
}

void PhysicsSystem::SetPlayerCollisionCenterHeight(float centerHeight)
{
    constexpr float minimumCenterHeight = 0.0f;
    mPlayerCollisionCenterHeight =
        std::max(centerHeight, minimumCenterHeight);
}

void PhysicsSystem::SyncKinematicBodies() const
{
    mStageCollisionBuilder->SyncKinematicBodies(mBulletWorld.get(), mBulletRigidBodies);
}

std::optional<PhysicsSystem::RayHitActor> PhysicsSystem::PickActorByRay(const glm::vec3& rayFrom,
                                                                        const glm::vec3& rayTo) const
{
    if (!mBulletWorld) {
        return std::nullopt;
    }

    SyncKinematicBodies();
    return mEditorPickSystem->PickActorByRay(mBulletWorld.get(), rayFrom, rayTo, mEditorPickObjects);
}

std::vector<PhysicsSystem::RayHitActor> PhysicsSystem::PickActorsByRay(const glm::vec3& rayFrom,
                                                                       const glm::vec3& rayTo) const
{
    if (!mBulletWorld) {
        return {};
    }

    SyncKinematicBodies();
    return mEditorPickSystem->PickActorsByRay(mBulletWorld.get(), rayFrom, rayTo, mEditorPickObjects);
}

std::optional<PhysicsSystem::RayHitActor> PhysicsSystem::RaycastStageSurface(
    const glm::vec3& rayFrom,
    const glm::vec3& rayTo) const
{
    if (!mBulletWorld) {
        return std::nullopt;
    }

    SyncKinematicBodies();

    const btVector3 bulletRayFrom(rayFrom.x, rayFrom.y, rayFrom.z);
    const btVector3 bulletRayTo(rayTo.x, rayTo.y, rayTo.z);
    btCollisionWorld::ClosestRayResultCallback callback(bulletRayFrom, bulletRayTo);
    callback.m_collisionFilterGroup = btBroadphaseProxy::DefaultFilter;
    callback.m_collisionFilterMask = btBroadphaseProxy::DefaultFilter;
    mBulletWorld->rayTest(bulletRayFrom, bulletRayTo, callback);

    if (!callback.hasHit()) {
        return std::nullopt;
    }

    const btVector3& hitPoint = callback.m_hitPointWorld;
    const btVector3& hitNormal = callback.m_hitNormalWorld;
    RayHitActor hit;
    hit.actor = callback.m_collisionObject
                    ? static_cast<Actor*>(callback.m_collisionObject->getUserPointer())
                    : nullptr;
    hit.hitPos = glm::vec3(hitPoint.x(), hitPoint.y(), hitPoint.z());
    hit.hitNormal = glm::vec3(hitNormal.x(), hitNormal.y(), hitNormal.z());
    hit.distance = glm::length(hit.hitPos - rayFrom);
    return hit;
}

std::optional<PhysicsSystem::RayHitActor> PhysicsSystem::CheckFallRespawnBySweep(
    const Actor* actor,
    const glm::vec3& from,
    const glm::vec3& to) const
{
    return mFallRespawnTriggerSystem->CheckFallRespawnBySweep(
        mBulletWorld.get(),
        mPlayerShape.get(),
        actor,
        from,
        to,
        mFallRespawnTriggerObjects);
}

ActorMovementCollisionResult PhysicsSystem::ResolveMovementCollision(
    Actor* actor,
    const glm::vec3& moveDelta,
    const glm::vec3& desiredPos,
    ActorCollisionFilter actorCollisionFilter)
{
    if (!mBulletWorld || !mPlayerShape) {
        return {desiredPos, glm::vec3(0.0f), false};
    }

    SyncKinematicBodies();
    return mActorCollisionResolver->CheckCollision(
        mBulletWorld.get(),
        mPlayerShape.get(),
        actor,
        moveDelta,
        desiredPos,
        mPlayerCollisionCenterHeight,
        actorCollisionFilter);
}
