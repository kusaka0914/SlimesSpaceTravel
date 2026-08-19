#include "system/PhysicsSystem.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "system/mesh/LoadedModel.h"
#include "system/physics/ActorCollisionResolver.h"
#include "system/physics/ActorModelEllipsoidShapeCache.h"
#include "system/physics/EditorPickSystem.h"
#include "system/physics/EllipsoidCollisionShapeGeometry.h"
#include "system/physics/FallRespawnTriggerSystem.h"
#include "system/physics/PhysicsWorldBuilder.h"
#include "system/physics/StageCollisionBuilder.h"

#include <algorithm>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/NarrowPhaseCollision/btContinuousConvexCollision.h>
#include <BulletCollision/NarrowPhaseCollision/btGjkEpaPenetrationDepthSolver.h>
#include <BulletCollision/NarrowPhaseCollision/btVoronoiSimplexSolver.h>
#include <cmath>
#include <glm/gtc/quaternion.hpp>
#include <memory>

namespace {
constexpr float minimumModelHalfExtent = 0.001f;

btTransform CreateActorCollisionTransform(
    const Actor& actor,
    const glm::vec3& collisionCenter)
{
    const glm::quat& orientation = actor.GetOrientation();

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(
        btVector3(
            collisionCenter.x,
            collisionCenter.y,
            collisionCenter.z));
    transform.setRotation(
        btQuaternion(
            orientation.x,
            orientation.y,
            orientation.z,
            orientation.w));
    return transform;
}

btTransform CreateModelBoundsTransform(
    const Actor& actor,
    const glm::vec3& actorPosition,
    const glm::vec3& scaledLocalBoundsCenter)
{
    glm::vec3 modelForwardAxis =
        -actor.GetForwardVec();
    glm::vec3 modelUpAxis =
        actor.GetUpVec();
    glm::vec3 modelLateralAxis =
        actor.GetLeftVec();
    if (glm::length(modelForwardAxis) <= 0.000001f ||
        glm::length(modelUpAxis) <= 0.000001f ||
        glm::length(modelLateralAxis) <= 0.000001f) {
        return CreateActorCollisionTransform(
            actor,
            actorPosition);
    }

    modelForwardAxis = glm::normalize(modelForwardAxis);
    modelUpAxis = glm::normalize(modelUpAxis);
    modelLateralAxis = glm::normalize(modelLateralAxis);

    // Rendering mirrors the model Z axis in addition to rotating its axes.
    // A box is symmetric around its center, so the equivalent proper rotation
    // uses +left for the box axis and applies the mirror only to the center.
    const glm::vec3 correctedLocalBoundsCenter(
        scaledLocalBoundsCenter.x,
        scaledLocalBoundsCenter.y,
        -scaledLocalBoundsCenter.z);
    const glm::vec3 worldBoundsCenter =
        actorPosition +
        modelForwardAxis * correctedLocalBoundsCenter.x +
        modelUpAxis * correctedLocalBoundsCenter.y +
        modelLateralAxis * correctedLocalBoundsCenter.z;

    glm::mat3 modelBoundsOrientation(1.0f);
    modelBoundsOrientation[0] = modelForwardAxis;
    modelBoundsOrientation[1] = modelUpAxis;
    modelBoundsOrientation[2] = modelLateralAxis;
    const glm::quat orientation =
        glm::normalize(
            glm::quat_cast(modelBoundsOrientation));

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(
        btVector3(
            worldBoundsCenter.x,
            worldBoundsCenter.y,
            worldBoundsCenter.z));
    transform.setRotation(
        btQuaternion(
            orientation.x,
            orientation.y,
            orientation.z,
            orientation.w));
    return transform;
}

class AnyCollisionContactCallback final
    : public btCollisionWorld::ContactResultCallback {
public:
    btScalar addSingleResult(
        btManifoldPoint& contactPoint,
        const btCollisionObjectWrapper* object0Wrapper,
        int partId0,
        int triangleIndex0,
        const btCollisionObjectWrapper* object1Wrapper,
        int partId1,
        int triangleIndex1) override
    {
        (void)object0Wrapper;
        (void)partId0;
        (void)triangleIndex0;
        (void)object1Wrapper;
        (void)partId1;
        (void)triangleIndex1;

        constexpr float contactDistanceTolerance = 0.001f;
        if (contactPoint.getDistance() <=
            contactDistanceTolerance) {
            mHasContact = true;
        }
        return 0.0f;
    }

    bool HasContact() const { return mHasContact; }

private:
    bool mHasContact = false;
};

bool DoesConvexShapeSweepOverlapActorCollision(
    btDiscreteDynamicsWorld* world,
    btConvexShape* movingShape,
    const btTransform& movingFromTransform,
    const btTransform& movingToTransform,
    btConvexShape* targetShape,
    const btTransform& targetTransform)
{
    if (!world || !movingShape || !targetShape) {
        return false;
    }

    btCollisionObject movingCollisionObject;
    movingCollisionObject.setCollisionShape(movingShape);
    movingCollisionObject.setWorldTransform(movingToTransform);

    btCollisionObject targetCollisionObject;
    targetCollisionObject.setCollisionShape(targetShape);
    targetCollisionObject.setWorldTransform(targetTransform);

    AnyCollisionContactCallback contactCallback;
    world->contactPairTest(
        &movingCollisionObject,
        &targetCollisionObject,
        contactCallback);
    if (contactCallback.HasContact()) {
        return true;
    }

    const btVector3 movementDelta =
        movingToTransform.getOrigin() -
        movingFromTransform.getOrigin();
    if (movementDelta.length2() <= 0.000001f) {
        return false;
    }

    btVoronoiSimplexSolver simplexSolver;
    btGjkEpaPenetrationDepthSolver penetrationDepthSolver;
    btContinuousConvexCollision collisionCast(
        movingShape,
        targetShape,
        &simplexSolver,
        &penetrationDepthSolver);
    btConvexCast::CastResult castResult;
    castResult.m_allowedPenetration = 0.0f;
    return collisionCast.calcTimeOfImpact(
        movingFromTransform,
        movingToTransform,
        targetTransform,
        targetTransform,
        castResult);
}
} // namespace

PhysicsSystem::PhysicsSystem(Game* game)
    : mGame(game)
{
    mWorldBuilder = std::make_unique<PhysicsWorldBuilder>();
    mStageCollisionBuilder = std::make_unique<StageCollisionBuilder>(mGame);
    mEditorPickSystem = std::make_unique<EditorPickSystem>(mGame);
    mFallRespawnTriggerSystem = std::make_unique<FallRespawnTriggerSystem>(mGame);
    mActorCollisionResolver = std::make_unique<ActorCollisionResolver>();
    mActorModelEllipsoidShapeCache =
        std::make_unique<ActorModelEllipsoidShapeCache>();
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

void PhysicsSystem::ClearForEditorStageRebuild()
{
    ClearBulletWorld();
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
    EllipsoidCollisionShapeGeometry::AddSurfacePoints(
        *ellipsoidShape,
        glm::vec3(
            mPlayerCollisionWidth,
            mPlayerCollisionHeight,
            mPlayerCollisionDepth));
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

bool PhysicsSystem::DoesActorModelSweepOverlapActorCollision(
    const Actor& movingActor,
    const glm::vec3& movementStart,
    const Actor& targetActor,
    const glm::vec3& movingModelHalfExtentPadding) const
{
    const LoadedModel* movingModel =
        movingActor.GetLoadedModel();
    if (!mBulletWorld ||
        !mPlayerShape ||
        !movingModel ||
        !movingModel->hasBounds) {
        return false;
    }

    const glm::vec3 movingScale =
        glm::abs(movingActor.GetScale());
    const glm::vec3 localBoundsSize =
        movingModel->boundsMaximum -
        movingModel->boundsMinimum;
    const glm::vec3 scaledHalfExtents =
        glm::max(
            localBoundsSize * movingScale * 0.5f +
                glm::max(
                    movingModelHalfExtentPadding,
                    glm::vec3(0.0f)),
            glm::vec3(minimumModelHalfExtent));
    const glm::vec3 scaledLocalBoundsCenter =
        (movingModel->boundsMinimum +
         movingModel->boundsMaximum) *
        0.5f * movingActor.GetScale();

    btBoxShape movingModelShape(
        btVector3(
            scaledHalfExtents.x,
            scaledHalfExtents.y,
            scaledHalfExtents.z));
    movingModelShape.setMargin(0.0f);

    const float targetCollisionScale =
        targetActor.GetCollisionScaleMultiplier();
    btUniformScalingShape targetCollisionShape(
        mPlayerShape.get(),
        targetCollisionScale);

    const btTransform movingFromTransform =
        CreateModelBoundsTransform(
            movingActor,
            movementStart,
            scaledLocalBoundsCenter);
    const btTransform movingToTransform =
        CreateModelBoundsTransform(
            movingActor,
            movingActor.GetPos(),
            scaledLocalBoundsCenter);

    glm::vec3 targetUpDirection =
        targetActor.GetUpVec();
    const float targetUpLengthSquared =
        glm::dot(
            targetUpDirection,
            targetUpDirection);
    if (targetUpLengthSquared <= 0.000001f) {
        targetUpDirection =
            glm::vec3(0.0f, 1.0f, 0.0f);
    } else {
        targetUpDirection /=
            std::sqrt(targetUpLengthSquared);
    }
    const glm::vec3 targetCollisionCenter =
        targetActor.GetPos() +
        targetUpDirection *
            mPlayerCollisionCenterHeight *
            targetCollisionScale;
    const btTransform targetTransform =
        CreateActorCollisionTransform(
            targetActor,
            targetCollisionCenter);

    return DoesConvexShapeSweepOverlapActorCollision(
        mBulletWorld.get(),
        &movingModelShape,
        movingFromTransform,
        movingToTransform,
        &targetCollisionShape,
        targetTransform);
}

bool PhysicsSystem::DoesActorEllipsoidModelSweepOverlapActorCollision(
    const Actor& movingActor,
    const glm::vec3& movementStart,
    const Actor& targetActor) const
{
    const ResolvedActorModelEllipsoidShape movingEllipsoid =
        ResolveActorModelEllipsoidShape(movingActor);
    if (!movingEllipsoid.shape || !mBulletWorld || !mPlayerShape) {
        return DoesActorModelSweepOverlapActorCollision(
            movingActor,
            movementStart,
            targetActor);
    }

    const float targetCollisionScale =
        targetActor.GetCollisionScaleMultiplier();
    btUniformScalingShape targetCollisionShape(
        mPlayerShape.get(),
        targetCollisionScale);

    glm::vec3 targetUpDirection = targetActor.GetUpVec();
    const float targetUpLengthSquared =
        glm::dot(targetUpDirection, targetUpDirection);
    if (targetUpLengthSquared <= 0.000001f) {
        targetUpDirection = glm::vec3(0.0f, 1.0f, 0.0f);
    } else {
        targetUpDirection /= std::sqrt(targetUpLengthSquared);
    }
    const glm::vec3 targetCollisionCenter =
        targetActor.GetPos() +
        targetUpDirection *
            mPlayerCollisionCenterHeight *
            targetCollisionScale;

    const btTransform movingFromTransform =
        ActorModelEllipsoidShapeCache::CreateWorldTransform(
            movingActor,
            movementStart,
            movingEllipsoid.scaledLocalBoundsCenter);
    const btTransform movingToTransform =
        ActorModelEllipsoidShapeCache::CreateWorldTransform(
            movingActor,
            movingActor.GetPos(),
            movingEllipsoid.scaledLocalBoundsCenter);
    const btTransform targetTransform =
        CreateActorCollisionTransform(
            targetActor,
            targetCollisionCenter);

    return DoesConvexShapeSweepOverlapActorCollision(
        mBulletWorld.get(),
        movingEllipsoid.shape,
        movingFromTransform,
        movingToTransform,
        &targetCollisionShape,
        targetTransform);
}

ResolvedActorModelEllipsoidShape
PhysicsSystem::ResolveActorModelEllipsoidShape(
    const Actor& actor) const
{
    return mActorModelEllipsoidShapeCache
        ? mActorModelEllipsoidShapeCache->Resolve(actor)
        : ResolvedActorModelEllipsoidShape{};
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
