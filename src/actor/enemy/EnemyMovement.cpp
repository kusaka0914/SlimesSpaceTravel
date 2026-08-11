#include "actor/enemy/EnemyMovement.h"

#include "Game.h"
#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "actor/enemy/EnemyStateMachine.h"
#include "actor/enemy/EnemyStatus.h"
#include "system/PhysicsSystem.h"
#include "utils/MathUtils.h"

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

bool CanRemainOnCurrentEllipseFace(
    const Enemy& enemy,
    const glm::vec3& requestedPosition)
{
    const Planet* planet = enemy.GetCurrentPlanet();
    if (!planet ||
        planet->GetPlanetShape() !=
            Planet::PlanetShape::Ellipse) {
        return true;
    }

    const Planet::EllipseSurfaceFace requestedFace =
        planet->ResolveEllipseSurfaceFace(
            requestedPosition);
    if (requestedFace ==
        Planet::EllipseSurfaceFace::Side) {
        return false;
    }

    const Planet::EllipseSurfaceFace currentHemisphere =
        planet->ResolveEllipseSurfaceHemisphere(
            enemy.GetPos());
    return currentHemisphere == requestedFace;
}
} // namespace

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
    enemy.SetFacingYawForEnemy(enemy.GetGame()->GetMathUtils()->GetYawFromDirection(enemy.GetUpVec(), facingForward) +
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
    enemy.SetFacingYawForEnemy(enemy.GetGame()->GetMathUtils()->GetYawFromDirection(enemy.GetUpVec(), facingForward) +
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

void EnemyMovement::MoveDuringAttacking(Enemy& enemy, const EnemyStatus& status, const EnemyStateMachine& stateMachine,
                                         float deltaTime)
{
    if (status.HasHitAnyPlayer()) {
        return;
    }

    glm::vec3 tangentialAttackDirection;
    if (!TryProjectDirectionOntoSurfaceTangent(
            enemy.GetFacingForwardVec(),
            enemy.GetUpVec(),
            tangentialAttackDirection)) {
        return;
    }

    glm::vec3 attackDirection;
    if (stateMachine.IsProgressing(status)) {
        attackDirection =
            tangentialAttackDirection;
    } else {
        attackDirection =
            -tangentialAttackDirection;
    }

    const glm::vec3 moveDelta =
        attackDirection *
        status.GetAttackSpeed() *
        deltaTime;
    enemy.SetPos(CalculateCollisionAdjustedPos(enemy, moveDelta));
}

void EnemyMovement::MoveDuringKnockBack(Enemy& enemy, const EnemyStatus& status, float deltaTime)
{
    const glm::vec3 moveDelta = status.GetKnockBackFrom() * status.GetKnockBackSpeed() * deltaTime;
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
    const glm::vec3& dodgeDirection)
{
    glm::vec3 tangentialPushDirection;
    if (!TryProjectDirectionOntoSurfaceTangent(
            dodgeDirection,
            enemy.GetUpVec(),
            tangentialPushDirection)) {
        return;
    }

    constexpr float airDodgePushSpeed = 6.0f;
    mAirDodgePushVelocity =
        tangentialPushDirection *
        airDodgePushSpeed;
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
    PhysicsSystem* physicsSystem =
        enemy.GetGame()
            ? enemy.GetGame()->GetPhysicsSystem()
            : nullptr;
    if (!physicsSystem) {
        return;
    }

    const ActorMovementCollisionResult collisionResult =
        physicsSystem->ResolveMovementCollision(
            &enemy,
            movementDelta,
            enemy.GetPos() + movementDelta);
    if (!CanRemainOnCurrentEllipseFace(
            enemy,
            collisionResult.resolvedPosition)) {
        mAirDodgePushVelocity = glm::vec3(0.0f);
        return;
    }

    enemy.SetPos(collisionResult.resolvedPosition);
    if (collisionResult.didHitStage) {
        mAirDodgePushVelocity = glm::vec3(0.0f);
        return;
    }

    constexpr float airDodgePushDampingPerSecond = 8.0f;
    const float dampedPushSpeed =
        pushSpeed *
        std::exp(
            -airDodgePushDampingPerSecond *
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

    stateMachine.StartIdle(enemy);
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
    enemy.ApplyGravityForEnemy(deltaTime);

    const float vPrev = glm::dot(prevVelocity, enemy.GetUpVec());
    const float vNow = glm::dot(enemy.GetVelocity(), enemy.GetUpVec());

    const bool isTop = vPrev > 0.0f && vNow <= 0.0f;
    if (isTop) {
        status.SetLaunchedTimer(status.GetDefaultLaunchedTimer());
    }
}

glm::vec3 EnemyMovement::CalculateCollisionAdjustedPos(Enemy& enemy, const glm::vec3& moveDelta)
{
    glm::vec3 desiredPos = enemy.GetPos() + moveDelta;

    const ActorMovementCollisionResult collisionResult =
        enemy.GetGame()->GetPhysicsSystem()->ResolveMovementCollision(
            &enemy,
            moveDelta,
            desiredPos);
    desiredPos = collisionResult.resolvedPosition;

    if (!CanRemainOnCurrentEllipseFace(
            enemy,
            desiredPos)) {
        return enemy.GetPos();
    }

    if (enemy.IsAlive() && enemy.IsOnGround()) {
        const glm::vec3 groundedPosition =
            mGrounding.ClampMoveToGround(enemy, desiredPos);
        if (!CanRemainOnCurrentEllipseFace(
                enemy,
                groundedPosition)) {
            return enemy.GetPos();
        }
        desiredPos = groundedPosition;
    }

    return desiredPos;
}
