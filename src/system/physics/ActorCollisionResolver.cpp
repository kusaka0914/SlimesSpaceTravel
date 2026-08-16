#include "system/physics/ActorCollisionResolver.h"

#include "actor/Actor.h"
#include "actor/Crystal.h"
#include "actor/Enemy.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "system/PhysicsSystem.h"

#include <algorithm>
#include <btBulletDynamicsCommon.h>
#include <cmath>
#include <vector>

namespace {
constexpr float collisionSkinWidth = 0.005f;
constexpr float overheadContactMaximumUpDot = -0.5f;
constexpr float overheadPlatformPushDistance = 0.15f;

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
} // namespace

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
                actor,
                actorResolvedDesiredPosition,
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

std::optional<glm::vec3> ActorCollisionResolver::CheckConflictActors(
    Actor* actor,
    const glm::vec3& desiredPos,
    ActorCollisionFilter actorCollisionFilter) const
{
    if (!actor || !actor->GetCurrentPlanet()) {
        return std::nullopt;
    }

    const std::vector<Enemy*>& enemies =
        actor->GetCurrentPlanet()->GetEnemies();

    for (Enemy* enemy : enemies) {
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

        if (auto conflictPos =
                CheckConflictActor(
                    actor,
                    enemy,
                    desiredPos)) {
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
                    actor,
                    crystal,
                    desiredPos)) {
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
                    actor,
                    npc,
                    desiredPos)) {
            return *conflictPos;
        }
    }

    return std::nullopt;
}

std::optional<glm::vec3>
ActorCollisionResolver::CheckConflictActor(
    Actor* movingActor,
    Actor* blockingActor,
    const glm::vec3& desiredPos) const
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
