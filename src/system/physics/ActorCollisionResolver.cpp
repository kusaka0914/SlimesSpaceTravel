#include "system/physics/ActorCollisionResolver.h"

#include "Game.h"

#include "actor/Actor.h"
#include "actor/Crystal.h"
#include "actor/Enemy.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Player.h"
#include "actor/enemy/EnemyCollisionGeometry.h"
#include "system/PhysicsSystem.h"
#include "system/physics/ActorModelEllipsoidShapeCache.h"

#include <algorithm>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/NarrowPhaseCollision/btContinuousConvexCollision.h>
#include <BulletCollision/NarrowPhaseCollision/btGjkEpaPenetrationDepthSolver.h>
#include <BulletCollision/NarrowPhaseCollision/btVoronoiSimplexSolver.h>
#include <cmath>
#include <limits>
#include <vector>

namespace {
constexpr float collisionSkinWidth = 0.005f;
constexpr float overheadContactMaximumUpDot = -0.5f;
constexpr float overheadPlatformPushDistance = 0.15f;
constexpr float modelCollisionEpsilon = 0.000001f;
constexpr float enemyTopSurfaceMinimumUpDot = 0.45f;
constexpr float enemyTopSlideFromFallRatio = 0.5f;
constexpr float enemyTopMaximumSlideDistance = 0.06f;
constexpr float enemyStopContactBuffer = 0.01f;
constexpr float walkableSurfaceMinimumUpDot = 0.65f;
constexpr float blockingSweepDirectionMaximumDot = -0.01f;

bool TryNormalizeDirection(
    const glm::vec3& direction,
    glm::vec3& normalizedDirection)
{
    const float directionLength = glm::length(direction);
    if (directionLength < 1e-6f) {
        return false;
    }

    normalizedDirection = direction / directionLength;
    return true;
}

glm::vec3 CalculateTangentialPushDirection(
    const Actor& movingActor,
    const Actor& blockingActor)
{
    glm::vec3 normalizedUpDirection;
    if (!TryNormalizeDirection(
            movingActor.GetUpVec(),
            normalizedUpDirection)) {
        normalizedUpDirection = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    const glm::vec3 awayFromBlockingActor =
        movingActor.GetPos() -
        blockingActor.GetPos();
    const glm::vec3 tangentialDirection =
        awayFromBlockingActor -
        normalizedUpDirection *
            glm::dot(
                awayFromBlockingActor,
                normalizedUpDirection);

    glm::vec3 normalizedTangentialDirection;
    if (TryNormalizeDirection(
            tangentialDirection,
            normalizedTangentialDirection)) {
        return normalizedTangentialDirection;
    }

    const glm::vec3 actorRightDirection =
        movingActor.GetRightVec();
    const glm::vec3 tangentialRightDirection =
        actorRightDirection -
        normalizedUpDirection *
            glm::dot(
                actorRightDirection,
                normalizedUpDirection);
    if (TryNormalizeDirection(
            tangentialRightDirection,
            normalizedTangentialDirection)) {
        return normalizedTangentialDirection;
    }

    const glm::vec3 fallbackReferenceAxis =
        std::abs(normalizedUpDirection.y) < 0.9f
            ? glm::vec3(0.0f, 1.0f, 0.0f)
            : glm::vec3(1.0f, 0.0f, 0.0f);
    return glm::normalize(
        glm::cross(
            normalizedUpDirection,
            fallbackReferenceAxis));
}

glm::vec3 CalculateOutwardSweepNormal(
    const btVector3& bulletSweepNormal,
    const glm::vec3& movingCollisionCenter,
    const glm::vec3& blockingCollisionCenter)
{
    glm::vec3 outwardDirection(0.0f);
    TryNormalizeDirection(
        movingCollisionCenter - blockingCollisionCenter,
        outwardDirection);

    glm::vec3 sweepNormal(
        bulletSweepNormal.x(),
        bulletSweepNormal.y(),
        bulletSweepNormal.z());
    if (!TryNormalizeDirection(sweepNormal, sweepNormal)) {
        return outwardDirection;
    }

    if (glm::dot(sweepNormal, outwardDirection) < 0.0f) {
        sweepNormal = -sweepNormal;
    }
    return sweepNormal;
}

glm::vec3 CalculateSurfaceSlideDelta(
    const glm::vec3& remainingMovement,
    const glm::vec3& outwardCollisionNormal)
{
    const float inwardMovementDistance =
        glm::dot(
            remainingMovement,
            outwardCollisionNormal);
    if (inwardMovementDistance >= 0.0f) {
        return remainingMovement;
    }

    return remainingMovement -
           outwardCollisionNormal * inwardMovementDistance;
}

glm::vec3 CalculateAutomaticEnemyTopSlide(
    const Actor& movingActor,
    const Actor& blockingEnemy,
    const glm::vec3& requestedMovement,
    const glm::vec3& movingActorUp,
    const glm::vec3& outwardCollisionNormal,
    const glm::vec3& surfaceSlideDelta)
{
    const bool isOnEnemyTopSurface =
        glm::dot(outwardCollisionNormal, movingActorUp) >=
        enemyTopSurfaceMinimumUpDot;
    const bool alreadySlidingAlongSurface =
        glm::length(surfaceSlideDelta) > modelCollisionEpsilon;
    if (!isOnEnemyTopSurface || alreadySlidingAlongSurface) {
        return glm::vec3(0.0f);
    }

    const float requestedDownwardDistance =
        std::max(
            0.0f,
            -glm::dot(requestedMovement, movingActorUp));
    const float automaticSlideDistance =
        std::min(
            requestedDownwardDistance *
                enemyTopSlideFromFallRatio,
            enemyTopMaximumSlideDistance);
    if (automaticSlideDistance <= modelCollisionEpsilon) {
        return glm::vec3(0.0f);
    }

    return CalculateTangentialPushDirection(
               movingActor,
               blockingEnemy) *
           automaticSlideDistance;
}

float CalculateExpandedBoundsClearance(
    const EnemyCollisionGeometry::ModelBounds& bounds,
    const glm::vec3& position,
    const glm::vec3& expansion)
{
    const glm::vec3 offset = position - bounds.center;
    float clearance = -std::numeric_limits<float>::max();
    for (glm::length_t axisIndex = 0;
         axisIndex < bounds.axes.size();
         ++axisIndex) {
        const float expandedExtent =
            bounds.halfExtents[axisIndex] +
            expansion[axisIndex];
        clearance = std::max(
            clearance,
            std::abs(glm::dot(offset, bounds.axes[axisIndex])) -
                expandedExtent);
    }
    return clearance;
}

bool ShouldStopBeforeEnemy(
    const EnemyCollisionGeometry::ModelBounds& blockingBounds,
    const glm::vec3& movementStart,
    const glm::vec3& movementEnd,
    const glm::vec3& expansion)
{
    const float startClearance =
        CalculateExpandedBoundsClearance(
            blockingBounds,
            movementStart,
            expansion);
    const float endClearance =
        CalculateExpandedBoundsClearance(
            blockingBounds,
            movementEnd,
            expansion);

    constexpr float clearanceEpsilon = 0.000001f;
    const bool startsOverlapping =
        startClearance <= clearanceEpsilon;
    if (startsOverlapping) {



        // 生成時の重なりや外部足場による移動で敵を閉じ込めない。既存の重なりを減らす方向だけ許可する。
        return endClearance <=
            startClearance + clearanceEpsilon;
    }



    // フレーム間の全経路を検査し、高速な攻撃が別の敵を通り抜けないようにする。
    return EnemyCollisionGeometry::DoesSegmentIntersectExpandedBounds(
        blockingBounds,
        movementStart,
        movementEnd,
        expansion);
}

btTransform CreatePlayerCollisionTransform(
    const Actor& actor,
    const glm::vec3& collisionCenter)
{
    const glm::quat& actorOrientation = actor.GetOrientation();

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(
        btVector3(
            collisionCenter.x,
            collisionCenter.y,
            collisionCenter.z));
    transform.setRotation(
        btQuaternion(
            actorOrientation.x,
            actorOrientation.y,
            actorOrientation.z,
            actorOrientation.w));
    return transform;
}

class ClosestBlockingStageSweepCallback final
    : public btCollisionWorld::ClosestConvexResultCallback {
public:
    ClosestBlockingStageSweepCallback(
        const btVector3& from,
        const btVector3& to,
        const glm::vec3& actorUpDirection,
        const glm::vec3& sweepDirection)
        : btCollisionWorld::ClosestConvexResultCallback(from, to),
          mActorUpDirection(actorUpDirection),
          mSweepDirection(sweepDirection)
    {
    }

    btScalar addSingleResult(
        btCollisionWorld::LocalConvexResult& convexResult,
        bool normalInWorldSpace) override
    {
        btVector3 hitNormal = convexResult.m_hitNormalLocal;
        if (!normalInWorldSpace && convexResult.m_hitCollisionObject) {
            hitNormal =
                convexResult.m_hitCollisionObject
                    ->getWorldTransform()
                    .getBasis() *
                hitNormal;
        }

        glm::vec3 blockingNormal(
            hitNormal.x(),
            hitNormal.y(),
            hitNormal.z());
        if (!TryNormalizeDirection(blockingNormal, blockingNormal)) {
            return 1.0f;
        }

        const bool isWalkableSurface =
            glm::dot(blockingNormal, mActorUpDirection) >=
            walkableSurfaceMinimumUpDot;
        const bool isMovingIntoSurface =
            glm::dot(blockingNormal, mSweepDirection) <=
            blockingSweepDirectionMaximumDot;
        if (isWalkableSurface || !isMovingIntoSurface) {
            return 1.0f;
        }

        return btCollisionWorld::ClosestConvexResultCallback::
            addSingleResult(convexResult, normalInWorldSpace);
    }

private:
    glm::vec3 mActorUpDirection{0.0f, 1.0f, 0.0f};
    glm::vec3 mSweepDirection{0.0f};
};

class DeepestPenetrationContactCallback final
    : public btCollisionWorld::ContactResultCallback {
public:
    explicit DeepestPenetrationContactCallback(
        const btCollisionObject* playerCollisionObject)
        : mPlayerCollisionObject(playerCollisionObject)
    {
        m_collisionFilterGroup =
            static_cast<short>(btBroadphaseProxy::DefaultFilter);
        m_collisionFilterMask =
            static_cast<short>(btBroadphaseProxy::DefaultFilter);
    }

    btScalar addSingleResult(
        btManifoldPoint& contactPoint,
        const btCollisionObjectWrapper* object0Wrapper,
        int partId0,
        int triangleIndex0,
        const btCollisionObjectWrapper* object1Wrapper,
        int partId1,
        int triangleIndex1) override
    {
        (void)partId0;
        (void)triangleIndex0;
        (void)partId1;
        (void)triangleIndex1;

        if (contactPoint.getDistance() >= 0.0f ||
            !object0Wrapper ||
            !object1Wrapper) {
            return 0.0f;
        }

        const btCollisionObject* object0 =
            object0Wrapper->getCollisionObject();
        const btCollisionObject* object1 =
            object1Wrapper->getCollisionObject();
        const bool playerIsObject0 =
            object0 == mPlayerCollisionObject;
        const bool playerIsObject1 =
            object1 == mPlayerCollisionObject;
        if (!playerIsObject0 && !playerIsObject1) {
            return 0.0f;
        }

        const float penetrationDepth =
            -static_cast<float>(contactPoint.getDistance());
        if (penetrationDepth <= mPenetrationDepth) {
            return 0.0f;
        }

        glm::vec3 outwardNormal(
            contactPoint.m_normalWorldOnB.x(),
            contactPoint.m_normalWorldOnB.y(),
            contactPoint.m_normalWorldOnB.z());
        if (playerIsObject1) {
            outwardNormal = -outwardNormal;
        }

        const float normalLength = glm::length(outwardNormal);
        if (normalLength < 1e-6f) {
            return 0.0f;
        }

        mPenetrationDepth = penetrationDepth;
        mOutwardNormal = outwardNormal / normalLength;
        mBlockingCollisionObject =
            playerIsObject0
                ? object1
                : object0;
        return 0.0f;
    }

    bool HasPenetration() const
    {
        return mPenetrationDepth > 0.0f;
    }

    float GetPenetrationDepth() const
    {
        return mPenetrationDepth;
    }

    const glm::vec3& GetOutwardNormal() const
    {
        return mOutwardNormal;
    }

    const btCollisionObject* GetBlockingCollisionObject() const
    {
        return mBlockingCollisionObject;
    }

private:
    const btCollisionObject* mPlayerCollisionObject = nullptr;
    const btCollisionObject* mBlockingCollisionObject = nullptr;
    float mPenetrationDepth = 0.0f;
    glm::vec3 mOutwardNormal{0.0f};
};

std::optional<glm::vec3> ResolveMovementAgainstEnemyEllipsoid(
    btDiscreteDynamicsWorld* world,
    btConvexShape* movingActorShape,
    Actor& movingActor,
    const glm::vec3& desiredPosition,
    float collisionCenterHeight,
    Enemy& blockingEnemy,
    btConvexShape* enemyEllipsoidShape,
    const glm::vec3& enemyScaledLocalBoundsCenter)
{
    if (!world || !movingActorShape || !enemyEllipsoidShape) {
        return std::nullopt;
    }

    glm::vec3 movingActorUp;
    if (!TryNormalizeDirection(
            movingActor.GetUpVec(),
            movingActorUp)) {
        movingActorUp = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    const glm::vec3 movementStart = movingActor.GetPos();
    const glm::vec3 movementDelta =
        desiredPosition - movementStart;
    const glm::vec3 collisionCenterOffset =
        movingActorUp * collisionCenterHeight;
    const btTransform movingFromTransform =
        CreatePlayerCollisionTransform(
            movingActor,
            movementStart + collisionCenterOffset);
    const btTransform movingToTransform =
        CreatePlayerCollisionTransform(
            movingActor,
            desiredPosition + collisionCenterOffset);
    const btTransform enemyTransform =
        ActorModelEllipsoidShapeCache::CreateWorldTransform(
            blockingEnemy,
            blockingEnemy.GetPos(),
            enemyScaledLocalBoundsCenter);

    if (glm::length(movementDelta) > modelCollisionEpsilon) {
        btVoronoiSimplexSolver simplexSolver;
        btGjkEpaPenetrationDepthSolver penetrationDepthSolver;
        btContinuousConvexCollision collisionCast(
            movingActorShape,
            enemyEllipsoidShape,
            &simplexSolver,
            &penetrationDepthSolver);
        btConvexCast::CastResult castResult;
        castResult.m_allowedPenetration = 0.0f;
        if (collisionCast.calcTimeOfImpact(
                movingFromTransform,
                movingToTransform,
                enemyTransform,
                enemyTransform,
                castResult)) {
            const float movementLength =
                glm::length(movementDelta);
            const float skinFraction =
                collisionSkinWidth / movementLength;
            const float allowedMovementFraction =
                std::max(
                    0.0f,
                    static_cast<float>(castResult.m_fraction) -
                        skinFraction);
            const glm::vec3 collisionPosition =
                movementStart +
                movementDelta * allowedMovementFraction;
            const glm::vec3 collisionCenter =
                collisionPosition + collisionCenterOffset;
            const btVector3& bulletEnemyCenter =
                enemyTransform.getOrigin();
            const glm::vec3 enemyCollisionCenter(
                bulletEnemyCenter.x(),
                bulletEnemyCenter.y(),
                bulletEnemyCenter.z());
            const glm::vec3 outwardCollisionNormal =
                CalculateOutwardSweepNormal(
                    castResult.m_normal,
                    collisionCenter,
                    enemyCollisionCenter);
            if (glm::length(outwardCollisionNormal) <=
                modelCollisionEpsilon) {
                return collisionPosition;
            }

            const glm::vec3 remainingMovement =
                movementDelta *
                (1.0f - allowedMovementFraction);
            const glm::vec3 surfaceSlideDelta =
                CalculateSurfaceSlideDelta(
                    remainingMovement,
                    outwardCollisionNormal);
            const glm::vec3 automaticTopSlide =
                CalculateAutomaticEnemyTopSlide(
                    movingActor,
                    blockingEnemy,
                    movementDelta,
                    movingActorUp,
                    outwardCollisionNormal,
                    surfaceSlideDelta);
            return collisionPosition +
                   outwardCollisionNormal * collisionSkinWidth +
                   surfaceSlideDelta +
                   automaticTopSlide;
        }
    }

    btCollisionObject movingCollisionObject;
    movingCollisionObject.setCollisionShape(movingActorShape);
    movingCollisionObject.setWorldTransform(movingToTransform);

    btCollisionObject enemyCollisionObject;
    enemyCollisionObject.setCollisionShape(enemyEllipsoidShape);
    enemyCollisionObject.setWorldTransform(enemyTransform);

    DeepestPenetrationContactCallback contactCallback(
        &movingCollisionObject);
    world->contactPairTest(
        &movingCollisionObject,
        &enemyCollisionObject,
        contactCallback);
    if (!contactCallback.HasPenetration()) {
        return std::nullopt;
    }

    const glm::vec3& outwardCollisionNormal =
        contactCallback.GetOutwardNormal();
    const glm::vec3 automaticTopSlide =
        CalculateAutomaticEnemyTopSlide(
            movingActor,
            blockingEnemy,
            movementDelta,
            movingActorUp,
            outwardCollisionNormal,
            glm::vec3(0.0f));
    return desiredPosition +
           outwardCollisionNormal *
               (contactCallback.GetPenetrationDepth() +
                collisionSkinWidth) +
           automaticTopSlide;
}
}

ActorMovementCollisionResult ActorCollisionResolver::CheckCollision(
    btDiscreteDynamicsWorld* world,
    btConvexShape* playerShape,
    Actor* actor,
    const glm::vec3& moveDelta,
    const glm::vec3& desiredPos,
    float collisionCenterHeight,
    ActorCollisionFilter actorCollisionFilter) const
{
    if (!actor) {
        return {desiredPos, glm::vec3(0.0f), false};
    }

    glm::vec3 actorResolvedDesiredPosition = desiredPos;
    glm::vec3 stageMovementDelta = moveDelta;
    if (auto conflictPosition =
            CheckConflictActors(
                world,
                playerShape,
                actor,
                actorResolvedDesiredPosition,
                collisionCenterHeight,
                actorCollisionFilter)) {
        actorResolvedDesiredPosition = *conflictPosition;
        stageMovementDelta +=
            actorResolvedDesiredPosition -
            desiredPos;
    }

    if (!world || !playerShape) {
        return {
            actorResolvedDesiredPosition,
            glm::vec3(0.0f),
            false};
    }

    ActorMovementCollisionResult collisionResult{
        actorResolvedDesiredPosition,
        glm::vec3(0.0f),
        false};
    if (glm::length(stageMovementDelta) > 1e-6f) {
        if (auto sweepResolution =
                CheckConflictWall(
                    world,
                    playerShape,
                    actor,
                    stageMovementDelta,
                    actorResolvedDesiredPosition,
                    collisionCenterHeight)) {
            collisionResult.resolvedPosition =
                sweepResolution->position;
            collisionResult.blockingNormal =
                sweepResolution->blockingNormal;
            collisionResult.didHitStage = true;
            collisionResult.didBlockRequestedMovement =
                sweepResolution->didBlockRequestedMovement;
        }
    }

    const StageOverlapResolution overlapResolution =
        ResolveStageOverlap(
        world,
        playerShape,
        actor,
        collisionResult.resolvedPosition,
        collisionCenterHeight);
    collisionResult.resolvedPosition = overlapResolution.position;
    collisionResult.hasUnresolvedStageOverlap =
        overlapResolution.hasRemainingOverlap;

    if (!overlapResolution.hadOverlap) {
        return collisionResult;
    }

    const float sweepNormalUpDot =
        glm::dot(collisionResult.blockingNormal, actor->GetUpVec());
    const float overlapNormalUpDot =
        glm::dot(overlapResolution.blockingNormal, actor->GetUpVec());
    if (!collisionResult.didHitStage ||
        overlapNormalUpDot < sweepNormalUpDot) {
        collisionResult.blockingNormal =
            overlapResolution.blockingNormal;
    }
    collisionResult.didHitStage = true;
    return collisionResult;
}

bool ActorCollisionResolver::DoesSweepHitBlockingStage(
    btDiscreteDynamicsWorld* world,
    btConvexShape* actorShape,
    const Actor& actor,
    const glm::vec3& fromPosition,
    const glm::vec3& toPosition,
    float collisionCenterHeight) const
{
    if (!world || !actorShape) {
        return false;
    }

    glm::vec3 actorUpDirection;
    if (!TryNormalizeDirection(
            actor.GetUpVec(),
            actorUpDirection)) {
        actorUpDirection = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    glm::vec3 sweepDirection;
    if (!TryNormalizeDirection(
            toPosition - fromPosition,
            sweepDirection)) {
        return false;
    }

    const glm::vec3 fromCollisionCenter =
        fromPosition +
        actorUpDirection * collisionCenterHeight;
    const glm::vec3 toCollisionCenter =
        toPosition +
        actorUpDirection * collisionCenterHeight;
    const btTransform fromTransform =
        CreatePlayerCollisionTransform(
            actor,
            fromCollisionCenter);
    const btTransform toTransform =
        CreatePlayerCollisionTransform(
            actor,
            toCollisionCenter);

    ClosestBlockingStageSweepCallback sweepCallback(
        fromTransform.getOrigin(),
        toTransform.getOrigin(),
        actorUpDirection,
        sweepDirection);
    sweepCallback.m_collisionFilterGroup =
        static_cast<short>(btBroadphaseProxy::DefaultFilter);
    sweepCallback.m_collisionFilterMask =
        static_cast<short>(btBroadphaseProxy::DefaultFilter);
    world->convexSweepTest(
        actorShape,
        fromTransform,
        toTransform,
        sweepCallback);
    return sweepCallback.hasHit();
}

std::optional<glm::vec3> ActorCollisionResolver::CheckConflictActors(
    btDiscreteDynamicsWorld* world,
    btConvexShape* movingActorShape,
    Actor* actor,
    const glm::vec3& desiredPos,
    float collisionCenterHeight,
    ActorCollisionFilter actorCollisionFilter) const
{
    if (!actor || !actor->GetCurrentPlanet()) {
        return std::nullopt;
    }

    const std::vector<Enemy*>& enemies =
        actor->GetCurrentPlanet()->GetEnemies();

    for (Enemy* enemy : enemies) {
        if (actorCollisionFilter ==
            ActorCollisionFilter::IgnoreEnemies) {
            continue;
        }

        if (enemy == actor) {
            continue;
        }

        const bool shouldIgnoreAirborneEnemy =
            actorCollisionFilter ==
                ActorCollisionFilter::IgnoreAirborneEnemies &&
            !enemy->IsOnGround();
        if (shouldIgnoreAirborneEnemy) {
            continue;
        }

        if (actorCollisionFilter ==
            ActorCollisionFilter::StopAtEnemies) {
            EnemyCollisionGeometry::ModelBounds enemyBounds;
            if (EnemyCollisionGeometry::TryCreateModelBounds(
                    *enemy,
                    enemyBounds)) {
                const float movingActorCollisionRadius =
                    std::max(
                        0.0f,
                        actor->GetRadius() *
                            actor->GetCollisionScaleMultiplier());
                const glm::vec3 collisionExpansion(
                    movingActorCollisionRadius +
                    collisionSkinWidth +
                    enemyStopContactBuffer);
                if (ShouldStopBeforeEnemy(
                        enemyBounds,
                        actor->GetPos(),
                        desiredPos,
                        collisionExpansion)) {
                    return actor->GetPos();
                }




                // 掃引判定は要求位置までを確認済みなので、終点の押し出しを行わず重なり始めた敵が外へ移動できるようにする。
                continue;
            }
        }

        if (auto conflictPos =
                CheckConflictActor(
                    world,
                    movingActorShape,
                    actor,
                    enemy,
                    desiredPos,
                    collisionCenterHeight)) {
            if (actorCollisionFilter ==
                ActorCollisionFilter::StopAtEnemies) {



                // 敵の移動はキネマティックなので、現在位置を返して停止し、どちらにも分離インパルスを加えない。
                return actor->GetPos();
            }
            return *conflictPos;
        }
    }

    const std::vector<Crystal*>& crystals =
        actor->GetCurrentPlanet()->GetCrystals();
    for (Crystal* crystal : crystals) {
        if (crystal == actor) {
            continue;
        }

        if (auto conflictPos =
                CheckConflictActor(
                    world,
                    movingActorShape,
                    actor,
                    crystal,
                    desiredPos,
                    collisionCenterHeight)) {
            return *conflictPos;
        }
    }

    const std::vector<NPC*>& npcs =
        actor->GetCurrentPlanet()->GetNPCs();
    for (NPC* npc : npcs) {
        if (npc == actor) {
            continue;
        }

        if (auto conflictPos =
                CheckConflictActor(
                    world,
                    movingActorShape,
                    actor,
                    npc,
                    desiredPos,
                    collisionCenterHeight)) {
            return *conflictPos;
        }
    }

    return std::nullopt;
}

std::optional<glm::vec3>
ActorCollisionResolver::CheckConflictActor(
    btDiscreteDynamicsWorld* world,
    btConvexShape* movingActorShape,
    Actor* movingActor,
    Actor* blockingActor,
    const glm::vec3& desiredPos,
    float collisionCenterHeight) const
{
    if (!movingActor ||
        !blockingActor ||
        !blockingActor->GetIsActive()) {
        return std::nullopt;
    }

    const glm::vec3 blockingActorPosition =
        blockingActor->GetPos();
    const glm::vec3 fromBlockingActorToDesiredPosition =
        desiredPos -
        blockingActorPosition;

    if (Enemy* blockingEnemy = dynamic_cast<Enemy*>(blockingActor)) {
        PhysicsSystem* physicsSystem =
            movingActor->GetGame()
                ? movingActor->GetGame()->GetPhysicsSystem()
                : nullptr;
        const ResolvedActorModelEllipsoidShape enemyEllipsoid =
            physicsSystem
                ? physicsSystem->ResolveActorModelEllipsoidShape(
                      *blockingEnemy)
                : ResolvedActorModelEllipsoidShape{};
        const bool canUsePlayerCollisionShape =
            dynamic_cast<Player*>(movingActor) != nullptr;
        if (enemyEllipsoid.shape &&
            world &&
            movingActorShape &&
            canUsePlayerCollisionShape) {
            return ResolveMovementAgainstEnemyEllipsoid(
                world,
                movingActorShape,
                *movingActor,
                desiredPos,
                collisionCenterHeight,
                *blockingEnemy,
                enemyEllipsoid.shape,
                enemyEllipsoid.scaledLocalBoundsCenter);
        }

        EnemyCollisionGeometry::ModelBounds enemyBounds;
        if (EnemyCollisionGeometry::TryCreateModelBounds(
                *blockingEnemy,
                enemyBounds)) {
            const float movingActorCollisionRadius =
                std::max(
                    0.0f,
                    movingActor->GetRadius() *
                        movingActor->GetCollisionScaleMultiplier());
            const glm::vec3 expandedHalfExtents =
                enemyBounds.halfExtents +
                glm::vec3(movingActorCollisionRadius);
            const glm::vec3 localOffset =
                desiredPos - enemyBounds.center;
            glm::vec3 localPosition(0.0f);
            for (glm::length_t axisIndex = 0;
                 axisIndex < enemyBounds.axes.size();
                 ++axisIndex) {
                localPosition[axisIndex] = glm::dot(
                    localOffset,
                    enemyBounds.axes[axisIndex]);
                if (std::abs(localPosition[axisIndex]) >
                    expandedHalfExtents[axisIndex]) {
                    return std::nullopt;
                }
            }

            glm::length_t nearestFaceAxis = 0;
            float nearestFacePenetration =
                expandedHalfExtents.x -
                std::abs(localPosition.x);
            for (glm::length_t axisIndex = 1;
                 axisIndex < enemyBounds.axes.size();
                 ++axisIndex) {
                const float facePenetration =
                    expandedHalfExtents[axisIndex] -
                    std::abs(localPosition[axisIndex]);
                if (facePenetration < nearestFacePenetration) {
                    nearestFaceAxis = axisIndex;
                    nearestFacePenetration = facePenetration;
                }
            }

            float outwardSign =
                localPosition[nearestFaceAxis] >= 0.0f
                    ? 1.0f
                    : -1.0f;
            if (std::abs(localPosition[nearestFaceAxis]) <=
                modelCollisionEpsilon) {
                const float axisDirection = glm::dot(
                    fromBlockingActorToDesiredPosition,
                    enemyBounds.axes[nearestFaceAxis]);
                outwardSign = axisDirection >= 0.0f ? 1.0f : -1.0f;
            }

            const glm::vec3 outwardDirection =
                enemyBounds.axes[nearestFaceAxis] *
                outwardSign;
            glm::vec3 movingActorUp;
            if (!TryNormalizeDirection(
                    movingActor->GetUpVec(),
                    movingActorUp)) {
                movingActorUp = glm::vec3(0.0f, 1.0f, 0.0f);
            }

            const glm::vec3 requestedMovement =
                desiredPos - movingActor->GetPos();
            const glm::vec3 automaticTopSlide =
                CalculateAutomaticEnemyTopSlide(
                    *movingActor,
                    *blockingEnemy,
                    requestedMovement,
                    movingActorUp,
                    outwardDirection,
                    glm::vec3(0.0f));
            return desiredPos +
                outwardDirection *
                    (nearestFacePenetration + collisionSkinWidth) +
                automaticTopSlide;
        }
    }

    const float blockingRadius =
        blockingActor->GetRadius();
    if (blockingRadius <= 0.0f) {
        return std::nullopt;
    }

    const float distanceSquared =
        glm::dot(
            fromBlockingActorToDesiredPosition,
            fromBlockingActorToDesiredPosition);
    if (distanceSquared >= blockingRadius * blockingRadius) {
        return std::nullopt;
    }

    glm::vec3 normalizedUpDirection;
    if (!TryNormalizeDirection(
            movingActor->GetUpVec(),
            normalizedUpDirection)) {
        normalizedUpDirection =
            glm::vec3(0.0f, 1.0f, 0.0f);
    }

    const float verticalDistanceFromBlockingActor =
        glm::dot(
            fromBlockingActorToDesiredPosition,
            normalizedUpDirection);
    const bool isEnemyAboveMovingActor =
        dynamic_cast<Enemy*>(blockingActor) &&
        verticalDistanceFromBlockingActor < 0.0f;

    if (isEnemyAboveMovingActor) {
        const glm::vec3 tangentialOffset =
            fromBlockingActorToDesiredPosition -
            normalizedUpDirection *
                verticalDistanceFromBlockingActor;
        const float tangentialDistance =
            glm::length(tangentialOffset);
        const float requiredTangentialDistance =
            std::sqrt(
                std::max(
                    0.0f,
                    blockingRadius * blockingRadius -
                    verticalDistanceFromBlockingActor *
                        verticalDistanceFromBlockingActor));

        glm::vec3 tangentialPushDirection;
        if (!TryNormalizeDirection(
                tangentialOffset,
                tangentialPushDirection)) {
            tangentialPushDirection =
                CalculateTangentialPushDirection(
                    *movingActor,
                    *blockingActor);
        }

        const float tangentialCorrectionDistance =
            std::max(
                collisionSkinWidth,
                requiredTangentialDistance -
                    tangentialDistance +
                    collisionSkinWidth);
        return desiredPos +
               tangentialPushDirection *
                   tangentialCorrectionDistance;
    }

    glm::vec3 outwardDirection;
    if (!TryNormalizeDirection(
            fromBlockingActorToDesiredPosition,
            outwardDirection)) {
        outwardDirection =
            CalculateTangentialPushDirection(
                *movingActor,
                *blockingActor);
    }

    return blockingActorPosition +
           outwardDirection *
               blockingRadius;
}

std::optional<ActorCollisionResolver::StageSweepResolution>
ActorCollisionResolver::CheckConflictWall(
    btDiscreteDynamicsWorld* world,
    btConvexShape* playerShape,
    Actor* actor,
    const glm::vec3& moveDelta,
    const glm::vec3& desiredPos,
    float collisionCenterHeight) const
{
    if (!world || !playerShape || !actor) {
        return std::nullopt;
    }

    glm::vec3 currentPos = actor->GetPos();
    glm::vec3 actorUpVec = actor->GetUpVec();
    glm::vec3 sweepFrom =
        currentPos + actorUpVec * collisionCenterHeight;
    glm::vec3 sweepTo =
        desiredPos + actorUpVec * collisionCenterHeight;

    const btTransform fromTransform =
        CreatePlayerCollisionTransform(*actor, sweepFrom);
    const btTransform toTransform =
        CreatePlayerCollisionTransform(*actor, sweepTo);

    btCollisionWorld::ClosestConvexResultCallback sweepCallback(fromTransform.getOrigin(), toTransform.getOrigin());

    sweepCallback.m_collisionFilterGroup = static_cast<short>(btBroadphaseProxy::DefaultFilter);
    sweepCallback.m_collisionFilterMask = static_cast<short>(btBroadphaseProxy::DefaultFilter);
    world->convexSweepTest(playerShape, fromTransform, toTransform, sweepCallback);

    if (!sweepCallback.hasHit()) {
        return std::nullopt;
    }

    const float allowFrac = std::max(0.0f, sweepCallback.m_closestHitFraction - 0.01f);
    const glm::vec3 posAfterHit = currentPos + moveDelta * allowFrac;
    const glm::vec3 hitNormGlm(sweepCallback.m_hitNormalWorld.x(), sweepCallback.m_hitNormalWorld.y(),
                               sweepCallback.m_hitNormalWorld.z());
    constexpr float blockingDirectionDotThreshold = -0.05f;
    const glm::vec3 requestedMovementDirection =
        glm::normalize(moveDelta);
    const bool didBlockRequestedMovement =
        glm::dot(hitNormGlm, requestedMovementDirection) <
        blockingDirectionDotThreshold;

    const glm::vec3 blocked = moveDelta * (1.0f - allowFrac);
    const glm::vec3 slideVec = blocked - hitNormGlm * glm::dot(blocked, hitNormGlm);
    const float slideEps = 1e-4f;

    if (glm::length(slideVec) > slideEps) {
        const glm::vec3 slideStartPosition =
            posAfterHit +
            hitNormGlm * collisionSkinWidth;
        glm::vec3 slideFrom =
            slideStartPosition +
            actorUpVec * collisionCenterHeight;
        glm::vec3 slideTo = slideFrom + slideVec;

        const btTransform slideFromTransition =
            CreatePlayerCollisionTransform(*actor, slideFrom);
        const btTransform slideToTransition =
            CreatePlayerCollisionTransform(*actor, slideTo);

        btCollisionWorld::ClosestConvexResultCallback slideCallback(slideFromTransition.getOrigin(),
                                                                    slideToTransition.getOrigin());

        slideCallback.m_collisionFilterGroup = static_cast<short>(btBroadphaseProxy::DefaultFilter);
        slideCallback.m_collisionFilterMask = static_cast<short>(btBroadphaseProxy::DefaultFilter);

        world->convexSweepTest(playerShape, slideFromTransition, slideToTransition, slideCallback);

        if (!slideCallback.hasHit()) {
            return StageSweepResolution{
                slideStartPosition + slideVec,
                hitNormGlm,
                didBlockRequestedMovement};
        }

        const float slideAllow = std::max(0.0f, slideCallback.m_closestHitFraction - 0.01f);
        const glm::vec3 slideHitNormal(
            slideCallback.m_hitNormalWorld.x(),
            slideCallback.m_hitNormalWorld.y(),
            slideCallback.m_hitNormalWorld.z());
        return StageSweepResolution{
            slideStartPosition + slideVec * slideAllow,
            slideHitNormal,
            didBlockRequestedMovement};
    }

    return StageSweepResolution{
        posAfterHit,
        hitNormGlm,
        didBlockRequestedMovement};
}

ActorCollisionResolver::StageOverlapResolution
ActorCollisionResolver::ResolveStageOverlap(
    btDiscreteDynamicsWorld* world,
    btConvexShape* playerShape,
    Actor* actor,
    const glm::vec3& position,
    float collisionCenterHeight) const
{
    if (!world || !playerShape || !actor) {
        return {position, glm::vec3(0.0f), false, false};
    }

    constexpr int maximumCorrectionCount = 4;

    glm::vec3 correctedPosition = position;
    glm::vec3 actorUp;
    if (!TryNormalizeDirection(
            actor->GetUpVec(),
            actorUp)) {
        actorUp = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    glm::vec3 mostCeilingFacingNormal(0.0f);
    float minimumNormalUpDot = 1.0f;
    bool hadOverlap = false;
    bool foundOverlapFreePosition = false;

    btCollisionObject playerCollisionObject;
    playerCollisionObject.setCollisionShape(playerShape);
    playerCollisionObject.setCollisionFlags(
        playerCollisionObject.getCollisionFlags() |
        btCollisionObject::CF_NO_CONTACT_RESPONSE);

    for (int correctionIndex = 0;
         correctionIndex < maximumCorrectionCount;
         ++correctionIndex) {
        const glm::vec3 collisionCenter =
            correctedPosition +
            actorUp * collisionCenterHeight;
        playerCollisionObject.setWorldTransform(
            CreatePlayerCollisionTransform(*actor, collisionCenter));

        DeepestPenetrationContactCallback contactCallback(
            &playerCollisionObject);
        world->contactTest(
            &playerCollisionObject,
            contactCallback);

        if (!contactCallback.HasPenetration()) {
            foundOverlapFreePosition = true;
            break;
        }

        hadOverlap = true;
        const glm::vec3& outwardNormal =
            contactCallback.GetOutwardNormal();
        const float normalUpDot =
            glm::dot(outwardNormal, actorUp);
        if (normalUpDot < minimumNormalUpDot) {
            minimumNormalUpDot = normalUpDot;
            mostCeilingFacingNormal = outwardNormal;
        }

        glm::vec3 correctionDirection = outwardNormal;
        float correctionDistance =
            contactCallback.GetPenetrationDepth() +
            collisionSkinWidth;

        const btCollisionObject* blockingCollisionObject =
            contactCallback.GetBlockingCollisionObject();
        Actor* blockingActor =
            blockingCollisionObject
                ? static_cast<Actor*>(
                      blockingCollisionObject->getUserPointer())
                : nullptr;
        Platform* blockingPlatform =
            dynamic_cast<Platform*>(blockingActor);
        const bool isMovingPlatformAboveActor =
            blockingPlatform &&
            blockingPlatform->UsesKinematicPhysics() &&
            normalUpDot <= overheadContactMaximumUpDot;
        if (isMovingPlatformAboveActor) {
            correctionDirection =
                CalculateTangentialPushDirection(
                    *actor,
                    *blockingPlatform);
            correctionDistance =
                std::max(
                    correctionDistance,
                    overheadPlatformPushDistance);
        }

        correctedPosition +=
            correctionDirection *
            correctionDistance;
    }

    bool hasRemainingOverlap = false;
    if (!foundOverlapFreePosition) {
        const glm::vec3 collisionCenter =
            correctedPosition +
            actorUp * collisionCenterHeight;
        playerCollisionObject.setWorldTransform(
            CreatePlayerCollisionTransform(*actor, collisionCenter));

        DeepestPenetrationContactCallback finalContactCallback(
            &playerCollisionObject);
        world->contactTest(
            &playerCollisionObject,
            finalContactCallback);
        hasRemainingOverlap =
            finalContactCallback.HasPenetration();
    }

    return {
        correctedPosition,
        mostCeilingFacingNormal,
        hadOverlap,
        hasRemainingOverlap};
}
