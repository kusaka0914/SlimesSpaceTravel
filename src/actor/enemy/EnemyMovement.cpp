#include "actor/enemy/EnemyMovement.h"

#include "Game.h"
#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "actor/enemy/EnemyStateMachine.h"
#include "actor/enemy/EnemyStatus.h"
#include "system/PhysicsSystem.h"
#include "utils/MathUtils.h"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

namespace {
constexpr float directionLengthEpsilonSquared = 0.000001f;

bool TryNormalizeDirection(
    const glm::vec3& direction,
    glm::vec3& normalizedDirection)
{
    const float directionLengthSquared =
        glm::dot(direction, direction);
    if (directionLengthSquared <=
        directionLengthEpsilonSquared) {
        return false;
    }

    normalizedDirection =
        direction /
        std::sqrt(directionLengthSquared);
    return true;
}

bool TryProjectDirectionOntoSurfaceTangent(
    const glm::vec3& direction,
    const glm::vec3& upVector,
    glm::vec3& normalizedTangentialDirection)
{
    glm::vec3 normalizedUpDirection;
    if (!TryNormalizeDirection(
            upVector,
            normalizedUpDirection)) {
        return false;
    }

    const glm::vec3 tangentialDirection =
        direction -
        normalizedUpDirection *
            glm::dot(
                direction,
                normalizedUpDirection);
    return TryNormalizeDirection(
        tangentialDirection,
        normalizedTangentialDirection);
}

bool TryCalculateTangentialDirectionToPlayer(
    const Enemy& enemy,
    const Player& player,
    glm::vec3& tangentialDirectionToPlayer)
{
    return TryProjectDirectionOntoSurfaceTangent(
        player.GetPos() - enemy.GetPos(),
        enemy.GetUpVec(),
        tangentialDirectionToPlayer);
}

bool IsWithinCurrentEllipseFaceMovementArea(
    const Enemy& enemy,
    const glm::vec3& requestedPosition)
{
    const Planet* planet = enemy.GetCurrentPlanet();
    if (!planet ||
        planet->GetPlanetShape() !=
            Planet::PlanetShape::Ellipse) {
        return true;
    }







    const Planet::EllipseSurfaceProjection surfaceProjection =
        planet->CalculateEllipseSurfaceProjection(
            requestedPosition);
    const glm::vec3 absoluteScale =
        glm::abs(planet->GetScale());

    int verticalAxisIndex = 0;
    if (absoluteScale.y < absoluteScale[verticalAxisIndex]) {
        verticalAxisIndex = 1;
    }
    if (absoluteScale.z < absoluteScale[verticalAxisIndex]) {
        verticalAxisIndex = 2;
    }

    constexpr float minimumAxisRadius = 0.001f;
    const float verticalAxisRadius =
        std::max(
            absoluteScale[verticalAxisIndex],
            minimumAxisRadius);
    const float projectedVerticalRatio =
        (surfaceProjection.position[verticalAxisIndex] -
         planet->GetPos()[verticalAxisIndex]) /
        verticalAxisRadius;

    const Planet::EllipseSurfaceFace currentHemisphere =
        planet->ResolveEllipseSurfaceHemisphere(
            enemy.GetPos());




    constexpr float faceInteriorBoundaryRatio = 0.45f;
    return currentHemisphere ==
               Planet::EllipseSurfaceFace::Front
        ? projectedVerticalRatio >= faceInteriorBoundaryRatio
        : projectedVerticalRatio <= -faceInteriorBoundaryRatio;
}

glm::vec3 ClampToCurrentEllipseFaceMovementArea(
    const Enemy& enemy,
    const glm::vec3& requestedPosition)
{
    if (IsWithinCurrentEllipseFaceMovementArea(
            enemy,
            requestedPosition)) {
        return requestedPosition;
    }

    const glm::vec3 currentPosition = enemy.GetPos();
    if (!IsWithinCurrentEllipseFaceMovementArea(
            enemy,
            currentPosition)) {
        return currentPosition;
    }

    float allowedRatio = 0.0f;
    float blockedRatio = 1.0f;
    constexpr int boundarySearchIterations = 16;
    for (int iteration = 0;
         iteration < boundarySearchIterations;
         ++iteration) {
        const float middleRatio =
            (allowedRatio + blockedRatio) * 0.5f;
        const glm::vec3 middlePosition =
            glm::mix(
                currentPosition,
                requestedPosition,
                middleRatio);
        if (IsWithinCurrentEllipseFaceMovementArea(
                enemy,
                middlePosition)) {
            allowedRatio = middleRatio;
        } else {
            blockedRatio = middleRatio;
        }
    }

    return glm::mix(
        currentPosition,
        requestedPosition,
        allowedRatio);
}
}

EnemyMovement::EnemyMovement(
    PhysicsSystem& physicsSystem,
    MathUtils& mathUtils)
    : mPhysicsSystem(physicsSystem),
      mMathUtils(mathUtils),
      mGrounding(physicsSystem)
{
}

void EnemyMovement::UpdateFacingVec(Enemy& enemy, EnemyStatus& status, float deltaTime)
{
    Player* nearestPlayer = status.GetNearestPlayer();
    if (!nearestPlayer) {
        return;
    }

    glm::vec3 tangentialDirectionToPlayer;
    if (!TryCalculateTangentialDirectionToPlayer(
            enemy,
            *nearestPlayer,
            tangentialDirectionToPlayer)) {
        return;
    }

    glm::vec3 currentTangentialFacing;
    if (!TryProjectDirectionOntoSurfaceTangent(
            enemy.GetFacingForwardVec(),
            enemy.GetUpVec(),
            currentTangentialFacing)) {
        currentTangentialFacing =
            tangentialDirectionToPlayer;
    }

    constexpr float turnSpeed = 5.0f;
    const float interpolationRatio =
        1.0f -
        std::exp(-turnSpeed * deltaTime);
    glm::vec3 facingForward;
    if (!TryNormalizeDirection(
            glm::mix(
                currentTangentialFacing,
                tangentialDirectionToPlayer,
                interpolationRatio),
            facingForward)) {
        facingForward =
            tangentialDirectionToPlayer;
    }

    enemy.SetFacingForwardForEnemy(facingForward);
    enemy.SetFacingYawForEnemy(mMathUtils.GetYawFromDirection(enemy.GetUpVec(), facingForward) +
                               3.14159265f);
}

void EnemyMovement::FaceNearestPlayerImmediately(Enemy& enemy, const EnemyStatus& status)
{
    Player* nearestPlayer = status.GetNearestPlayer();
    if (!nearestPlayer) {
        return;
    }

    glm::vec3 facingForward;
    if (!TryCalculateTangentialDirectionToPlayer(
            enemy,
            *nearestPlayer,
            facingForward)) {
        return;
    }

    enemy.SetFacingForwardForEnemy(facingForward);
    enemy.SetFacingYawForEnemy(mMathUtils.GetYawFromDirection(enemy.GetUpVec(), facingForward) +
                               3.14159265f);
}

void EnemyMovement::MoveToPlayer(Enemy& enemy, const EnemyStatus& status, float deltaTime)
{



    glm::vec3 tangentialMoveDirection;
    if (!TryProjectDirectionOntoSurfaceTangent(
            enemy.GetFacingForwardVec(),
            enemy.GetUpVec(),
            tangentialMoveDirection)) {
        return;
    }

    const glm::vec3 moveDelta =
        tangentialMoveDirection *
        status.GetMoveSpeed() *
        deltaTime;
    enemy.SetPos(CalculateCollisionAdjustedPos(enemy, moveDelta));
}

bool EnemyMovement::MoveTowardPlayerQuickly(
    Enemy& enemy,
    const EnemyStatus& status,
    float speed,
    float stopDistance,
    float deltaTime)
{
    Player* nearestPlayer = status.GetNearestPlayer();
    if (!nearestPlayer) {
        return true;
    }

    const float distanceToPlayer = glm::length(
        nearestPlayer->GetPos() - enemy.GetPos());
    const float clampedStopDistance = std::max(0.0f, stopDistance);
    if (distanceToPlayer <= clampedStopDistance) {
        return true;
    }

    glm::vec3 moveDirection;
    if (!TryCalculateTangentialDirectionToPlayer(
            enemy,
            *nearestPlayer,
            moveDirection)) {
        return true;
    }

    const float requestedMoveDistance = std::min(
        std::max(0.0f, speed) * deltaTime,
        distanceToPlayer - clampedStopDistance);
    const glm::vec3 previousPosition = enemy.GetPos();
    enemy.SetPos(CalculateCollisionAdjustedPos(
        enemy,
        moveDirection * requestedMoveDistance));

    constexpr float minimumMovementDistance = 0.0001f;
    const float actualMovementDistance = glm::length(
        enemy.GetPos() - previousPosition);
    const float remainingDistance = glm::length(
        nearestPlayer->GetPos() - enemy.GetPos());
    return remainingDistance <= clampedStopDistance + 0.01f ||
           actualMovementDistance <= minimumMovementDistance;
}

float EnemyMovement::MoveAwayFromPlayerQuickly(
    Enemy& enemy,
    const EnemyStatus& status,
    float speed,
    float deltaTime)
{
    Player* nearestPlayer = status.GetNearestPlayer();
    if (!nearestPlayer) {
        return 0.0f;
    }

    glm::vec3 directionToPlayer;
    if (!TryCalculateTangentialDirectionToPlayer(
            enemy,
            *nearestPlayer,
            directionToPlayer)) {
        return 0.0f;
    }

    const glm::vec3 previousPosition = enemy.GetPos();
    const glm::vec3 requestedMovement =
        -directionToPlayer * std::max(0.0f, speed) * deltaTime;
    enemy.SetPos(CalculateCollisionAdjustedPos(
        enemy,
        requestedMovement));
    return glm::length(enemy.GetPos() - previousPosition);
}

void EnemyMovement::MoveDuringAttacking(
    Enemy& enemy,
    const EnemyStatus& status,
    const EnemyStateMachine& stateMachine,
    float deltaTime)
{
    if (status.HasHitAnyPlayer() ||
        !stateMachine.IsProgressing(status)) {
        return;
    }

    glm::vec3 tangentialAttackDirection;
    if (!TryProjectDirectionOntoSurfaceTangent(
            enemy.GetFacingForwardVec(),
            enemy.GetUpVec(),
            tangentialAttackDirection)) {
        return;
    }

    const glm::vec3 moveDelta =
        tangentialAttackDirection *
        status.GetAttackSpeed() *
        deltaTime;
    enemy.SetPos(CalculateCollisionAdjustedPos(enemy, moveDelta));
}

void EnemyMovement::MoveDuringKnockBack(Enemy& enemy, const EnemyStatus& status, float deltaTime)
{
    const glm::vec3 moveDelta =
        status.GetKnockBackFrom() *
        status.GetKnockBackSpeed() *
        deltaTime;
    enemy.SetPos(CalculateCollisionAdjustedPos(enemy, moveDelta));
}

void EnemyMovement::MoveDuringDying(Enemy& enemy, float deltaTime)
{
    glm::vec3 upDirection = enemy.GetUpVec();
    if (glm::length(upDirection) < 1e-6f) {
        return;
    }

    upDirection = glm::normalize(upDirection);

    constexpr float gravityAcceleration = 9.8f;
    const glm::vec3 gravityVelocityDelta = -upDirection * gravityAcceleration * deltaTime;
    const glm::vec3 velocity = enemy.GetVelocity() + gravityVelocityDelta;

    enemy.SetVelocity(velocity);

    const glm::vec3 moveDelta = velocity * deltaTime;
    enemy.SetPos(CalculateCollisionAdjustedPos(enemy, moveDelta));
}

void EnemyMovement::ApplyAirDodgePush(
    Enemy& enemy,
    const glm::vec3& dodgeDirection,
    float pushSpeed,
    float pushDampingPerSecond)
{
    glm::vec3 tangentialPushDirection;
    if (!TryProjectDirectionOntoSurfaceTangent(
            dodgeDirection,
            enemy.GetUpVec(),
            tangentialPushDirection)) {
        return;
    }

    mAirDodgePushVelocity =
        tangentialPushDirection *
        std::max(0.0f, pushSpeed);
    mAirDodgePushDampingPerSecond =
        std::max(0.0f, pushDampingPerSecond);
}

void EnemyMovement::UpdateAirDodgePushMovement(
    Enemy& enemy,
    float deltaTime)
{
    const float pushSpeed =
        glm::length(mAirDodgePushVelocity);
    constexpr float minimumPushSpeed = 0.05f;
    if (pushSpeed < minimumPushSpeed) {
        mAirDodgePushVelocity = glm::vec3(0.0f);
        return;
    }

    glm::vec3 tangentialPushDirection;
    if (!TryProjectDirectionOntoSurfaceTangent(
            mAirDodgePushVelocity,
            enemy.GetUpVec(),
            tangentialPushDirection)) {
        mAirDodgePushVelocity = glm::vec3(0.0f);
        return;
    }

    const glm::vec3 movementDelta =
        tangentialPushDirection *
        pushSpeed *
        deltaTime;
    const ActorMovementCollisionResult collisionResult =
        mPhysicsSystem.ResolveMovementCollision(
            &enemy,
            movementDelta,
            enemy.GetPos() + movementDelta,
            ActorCollisionFilter::StopAtEnemies);
    const glm::vec3 faceConstrainedPosition =
        ClampToCurrentEllipseFaceMovementArea(
            enemy,
            collisionResult.resolvedPosition);
    const bool wasBlockedByEllipseFaceBoundary =
        glm::length(
            faceConstrainedPosition -
            collisionResult.resolvedPosition) >
        0.000001f;
    if (wasBlockedByEllipseFaceBoundary) {
        mAirDodgePushVelocity = glm::vec3(0.0f);
        enemy.SetPos(faceConstrainedPosition);
        return;
    }

    enemy.SetPos(faceConstrainedPosition);
    if (collisionResult.didHitStage) {
        mAirDodgePushVelocity = glm::vec3(0.0f);
        return;
    }

    const float dampedPushSpeed =
        pushSpeed *
        std::exp(
            -mAirDodgePushDampingPerSecond *
            deltaTime);
    mAirDodgePushVelocity =
        tangentialPushDirection *
        dampedPushSpeed;
}

void EnemyMovement::LaunchIntoAir(Enemy& enemy, EnemyStatus& status, EnemyStateMachine& stateMachine, float deltaTime)
{
    enemy.GetGame()->OnEnemyLaunched();

    mAirDodgePushVelocity = glm::vec3(0.0f);

    constexpr float gravityAcceleration = 9.8f;
    const float launchHeight =
        std::max(
            0.0f,
            status.GetLaunchHeight());
    const float launchSpeed =
        std::sqrt(
            2.0f *
            gravityAcceleration *
            launchHeight);
    enemy.AddVelocity(enemy.GetUpVec() * launchSpeed);
    enemy.AddPos(enemy.GetVelocity() * deltaTime);

    enemy.SetOnGroundForEnemy(false);
    status.SetStandByAttackTimer(-1.0f);
    status.SetAttackMotionTimer(-1.0f);
    enemy.SetShouldJudgeLandingForEnemy(false);
    enemy.GetGame()->SetHitStopTimer(0.3f);

    stateMachine.StartLaunched(enemy);
}

void EnemyMovement::UpdateInAir(Enemy& enemy, EnemyStatus& status, EnemyStateMachine& stateMachine, float deltaTime)
{
    UpdateAirDodgePushMovement(
        enemy,
        deltaTime);

    if (status.GetLaunchedTimer() >= 0.0f) {
        status.DecreaseLaunchedTimer(deltaTime);

        if (status.GetLaunchedTimer() >= 0.0f) {
            return;
        }

        stateMachine.FinishLaunched(enemy, status);
        return;
    }

    const glm::vec3 prevVelocity = enemy.GetVelocity();
    ApplyGravityWithContinuousCollision(
        enemy,
        deltaTime);

    if (enemy.IsOnGround()) {
        status.ClearLaunchedTimer();
        stateMachine.StartIdle(enemy);
        return;
    }

    const float vPrev = glm::dot(prevVelocity, enemy.GetUpVec());
    const float vNow = glm::dot(enemy.GetVelocity(), enemy.GetUpVec());

    const bool isTop = vPrev > 0.0f && vNow <= 0.0f;
    if (isTop) {
        status.SetLaunchedTimer(status.GetDefaultLaunchedTimer());
    }
}

void EnemyMovement::ApplyGravityWithContinuousCollision(
    Enemy& enemy,
    float deltaTime)
{
    glm::vec3 upDirection;
    if (!TryNormalizeDirection(
            enemy.GetUpVec(),
            upDirection)) {
        return;
    }

    constexpr float gravityAcceleration = 9.8f;
    const glm::vec3 velocity =
        enemy.GetVelocity() -
        upDirection *
            gravityAcceleration *
            deltaTime;
    enemy.SetVelocity(velocity);

    const glm::vec3 movementDelta =
        velocity *
        deltaTime;
    const ActorMovementCollisionResult collisionResult =
        mPhysicsSystem.ResolveMovementCollision(
            &enemy,
            movementDelta,
            enemy.GetPos() + movementDelta,
            ActorCollisionFilter::StopAtEnemies);
    const glm::vec3 faceConstrainedPosition =
        ClampToCurrentEllipseFaceMovementArea(
            enemy,
            collisionResult.resolvedPosition);
    enemy.SetPos(faceConstrainedPosition);

    const bool isMovingTowardGround =
        glm::dot(velocity, upDirection) <= 0.0f;
    const bool hitWalkableGround =
        collisionResult.didHitStage &&
        isMovingTowardGround &&
        CharacterActor::IsWalkableGroundNormal(
            collisionResult.blockingNormal,
            upDirection);
    if (hitWalkableGround) {
        enemy.SetShouldJudgeLandingForEnemy(true);
        enemy.Land(faceConstrainedPosition);
        return;
    }

    if (!collisionResult.didHitStage) {
        return;
    }

    glm::vec3 blockingNormal;
    if (!TryNormalizeDirection(
            collisionResult.blockingNormal,
            blockingNormal)) {
        enemy.SetVelocity(glm::vec3(0.0f));
        return;
    }

    const float velocityIntoSurface =
        glm::dot(velocity, blockingNormal);
    if (velocityIntoSurface < 0.0f) {
        enemy.SetVelocity(
            velocity -
            blockingNormal *
                velocityIntoSurface);
    }
}

glm::vec3 EnemyMovement::CalculateCollisionAdjustedPos(Enemy& enemy, const glm::vec3& moveDelta)
{
    glm::vec3 desiredPos = enemy.GetPos() + moveDelta;

    const ActorMovementCollisionResult collisionResult =
        mPhysicsSystem.ResolveMovementCollision(
            &enemy,
            moveDelta,
            desiredPos,
            ActorCollisionFilter::AllActors);
    desiredPos = collisionResult.resolvedPosition;

    desiredPos =
        ClampToCurrentEllipseFaceMovementArea(
            enemy,
            desiredPos);

    if (enemy.IsAlive() && enemy.IsOnGround()) {
        const glm::vec3 groundedPosition =
            mGrounding.ClampMoveToGround(enemy, desiredPos);
        desiredPos =
            ClampToCurrentEllipseFaceMovementArea(
                enemy,
                groundedPosition);
    }

    return desiredPos;
}
