#include "system/PhysicsSystem.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "system/physics/ActorCollisionResolver.h"
#include "system/physics/EditorPickSystem.h"
#include "system/physics/FallRespawnTriggerSystem.h"
#include "system/physics/PlayerCollisionShapeGeometry.h"
#include "system/physics/PhysicsWorldBuilder.h"
#include "system/physics/StageCollisionBuilder.h"

#include <algorithm>
#include <btBulletDynamicsCommon.h>
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
    auto ellipsoidShape = std::make_unique<btConvexHullShape>();
    ellipsoidShape->setMargin(0.0f);

    for (int latitudeIndex = 0;
         latitudeIndex <= PlayerCollisionShapeGeometry::LatitudeSegmentCount;
         ++latitudeIndex) {
        for (int longitudeIndex = 0;
             longitudeIndex < PlayerCollisionShapeGeometry::LongitudeSegmentCount;
             ++longitudeIndex) {
            const glm::vec3 localSurfacePoint =
                PlayerCollisionShapeGeometry::CalculateLocalSurfacePoint(
                    mPlayerCollisionWidth,
                    mPlayerCollisionHeight,
                    mPlayerCollisionDepth,
                    latitudeIndex,
                    longitudeIndex);
            ellipsoidShape->addPoint(
                btVector3(
                    localSurfacePoint.x,
                    localSurfacePoint.y,
                    localSurfacePoint.z),
                false);
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

std::vector<PhysicsSystem::RayHitActor>
PhysicsSystem::RaycastStageSurfaces(
    const glm::vec3& rayFrom,
    const glm::vec3& rayTo) const
{
    std::vector<RayHitActor> hits;
    if (!mBulletWorld) {
        return hits;
    }

    SyncKinematicBodies();

    const btVector3 bulletRayFrom(
        rayFrom.x,
        rayFrom.y,
        rayFrom.z);
    const btVector3 bulletRayTo(
        rayTo.x,
        rayTo.y,
        rayTo.z);
    btCollisionWorld::AllHitsRayResultCallback callback(
        bulletRayFrom,
        bulletRayTo);
    callback.m_collisionFilterGroup =
        btBroadphaseProxy::DefaultFilter;
    callback.m_collisionFilterMask =
        btBroadphaseProxy::DefaultFilter;
    mBulletWorld->rayTest(
        bulletRayFrom,
        bulletRayTo,
        callback);

    if (!callback.hasHit()) {
        return hits;
    }

    hits.reserve(callback.m_collisionObjects.size());
    for (int hitIndex = 0;
         hitIndex < callback.m_collisionObjects.size();
         ++hitIndex) {
        const btCollisionObject* collisionObject =
            callback.m_collisionObjects[hitIndex];
        if (!collisionObject) {
            continue;
        }

        const btVector3& bulletHitPoint =
            callback.m_hitPointWorld[hitIndex];
        const btVector3& bulletHitNormal =
            callback.m_hitNormalWorld[hitIndex];

        RayHitActor hit;
        hit.actor = static_cast<Actor*>(
            collisionObject->getUserPointer());
        hit.hitPos = glm::vec3(
            bulletHitPoint.x(),
            bulletHitPoint.y(),
            bulletHitPoint.z());
        hit.hitNormal = glm::vec3(
            bulletHitNormal.x(),
            bulletHitNormal.y(),
            bulletHitNormal.z());
        hit.distance = glm::length(hit.hitPos - rayFrom);
        hits.emplace_back(hit);
    }

    std::sort(
        hits.begin(),
        hits.end(),
        [](const RayHitActor& left, const RayHitActor& right) {
            return left.distance < right.distance;
        });
    return hits;
}

std::optional<PhysicsSystem::RayHitActor> PhysicsSystem::CheckFallRespawnBySweep(
    const Actor* actor,
    const glm::vec3& from,
    const glm::vec3& to) const
{
    if (!mPlayerShape) {
        return std::nullopt;
    }

    const float collisionScaleMultiplier =
        actor ? actor->GetCollisionScaleMultiplier() : 1.0f;
    btUniformScalingShape scaledPlayerShape(
        mPlayerShape.get(),
        collisionScaleMultiplier);

    return mFallRespawnTriggerSystem->CheckFallRespawnBySweep(
        mBulletWorld.get(),
        &scaledPlayerShape,
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

    const float collisionScaleMultiplier =
        actor ? actor->GetCollisionScaleMultiplier() : 1.0f;
    btUniformScalingShape scaledPlayerShape(
        mPlayerShape.get(),
        collisionScaleMultiplier);

    SyncKinematicBodies();
    return mActorCollisionResolver->CheckCollision(
        mBulletWorld.get(),
        &scaledPlayerShape,
        actor,
        moveDelta,
        desiredPos,
        mPlayerCollisionCenterHeight * collisionScaleMultiplier,
        actorCollisionFilter);
}
