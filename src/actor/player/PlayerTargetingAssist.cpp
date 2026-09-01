#include "actor/player/PlayerTargetingAssist.h"

#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "actor/enemy/EnemyCollisionGeometry.h"
#include "actor/player/PlayerMovement.h"

#include <cmath>
#include <limits>
#include <vector>

#include <glm/glm.hpp>

namespace {
constexpr float directionEpsilonSquared = 0.000001f;

bool TryNormalize(const glm::vec3& value, glm::vec3& normalized)
{
    const float lengthSquared = glm::dot(value, value);
    if (lengthSquared <= directionEpsilonSquared) {
        return false;
    }

    normalized = value / std::sqrt(lengthSquared);
    return true;
}

glm::vec3 ProjectOntoTangentPlane(const glm::vec3& direction, const glm::vec3& up)
{
    return direction - up * glm::dot(direction, up);
}

bool IsValidEnemy(const Player& player, const Enemy* enemy)
{
    const Planet* planet = player.GetCurrentPlanet();
    return
        enemy &&
        planet &&
        enemy->GetIsActive() &&
        enemy->IsAlive() &&
        !enemy->GetIsDead() &&
        enemy->GetCurrentPlanet() == planet &&
        planet->ArePositionsOnSameSurfaceFace(
            player.GetPos(),
            enemy->GetPos());
}

bool IsActiveLaunchedEnemyOnCurrentPlanet(
    const Player& player,
    const Enemy* enemy)
{
    const Planet* planet = player.GetCurrentPlanet();
    return
        enemy &&
        planet &&
        enemy->GetIsActive() &&
        enemy->IsAlive() &&
        !enemy->GetIsDead() &&
        enemy->IsLaunched() &&
        enemy->GetCurrentPlanet() == planet;
}

bool IsLaunchedEnemyNearRecovery(
    const Enemy& enemy,
    float maximumLaunchedTimerSeconds)
{
    const float launchedTimerSeconds = enemy.GetLaunchedTimer();
    if (launchedTimerSeconds >= 0.0f) {
        return launchedTimerSeconds <= maximumLaunchedTimerSeconds;
    }

    glm::vec3 upDirection;
    if (!TryNormalize(enemy.GetUpVec(), upDirection)) {
        return false;
    }




    return glm::dot(enemy.GetVelocity(), upDirection) <= 0.0f;
}

float CalculateDistanceSquaredToEnemySurface(
    const Player& player,
    const Enemy& enemy)
{
    EnemyCollisionGeometry::ModelBounds modelBounds;
    const glm::vec3 closestPoint =
        EnemyCollisionGeometry::TryCreateModelBounds(
            enemy,
            modelBounds)
            ? EnemyCollisionGeometry::CalculateClosestPoint(
                  modelBounds,
                  player.GetPos())
            : enemy.GetPos();
    const glm::vec3 offset = closestPoint - player.GetPos();
    return glm::dot(offset, offset);
}
}

Enemy* PlayerTargetingAssist::FindAttackTarget(
    const Player& player,
    float attackRange,
    float attackAngle,
    bool requireLaunchedTarget)
{
    Planet* planet = player.GetCurrentPlanet();
    if (!planet || attackRange <= 0.0f) {
        return nullptr;
    }

    glm::vec3 normalizedFacingDirection;
    const bool hasFacingDirection =
        TryNormalize(player.GetFacingForwardVec(), normalizedFacingDirection);

    const float facingThreshold = std::cos(attackAngle * 0.5f);

    Enemy* nearestForwardTarget = nullptr;
    float nearestForwardDistanceSquared = std::numeric_limits<float>::max();

    Enemy* nearestAnyDirectionTarget = nullptr;
    float nearestAnyDirectionDistanceSquared = std::numeric_limits<float>::max();

    for (Enemy* enemy : planet->GetEnemies()) {
        if (!IsValidEnemy(player, enemy)) {
            continue;
        }

        if (requireLaunchedTarget && !enemy->IsLaunched()) {
            continue;
        }

        const glm::vec3 toEnemyCenter = enemy->GetPos() - player.GetPos();
        const float distanceSquared =
            CalculateDistanceSquaredToEnemySurface(player, *enemy);
        if (distanceSquared > attackRange * attackRange) {
            continue;
        }

        // PlayerAttackHitDetectorと同じ距離条件を使う。
        const float effectiveRange = attackRange + enemy->GetRadius();
        const float effectiveRangeSquared = effectiveRange * effectiveRange;
        if (distanceSquared > effectiveRangeSquared) {
            continue;
        }

        if (distanceSquared < nearestAnyDirectionDistanceSquared) {
            nearestAnyDirectionDistanceSquared = distanceSquared;
            nearestAnyDirectionTarget = enemy;
        }

        if (!hasFacingDirection) {
            continue;
        }

        glm::vec3 directionToTarget;
        if (!TryNormalize(toEnemyCenter, directionToTarget)) {
            directionToTarget = normalizedFacingDirection;
        }

        const float facingDot =
            glm::dot(normalizedFacingDirection, directionToTarget);

        if (facingDot < facingThreshold) {
            continue;
        }

        if (distanceSquared < nearestForwardDistanceSquared) {
            nearestForwardDistanceSquared = distanceSquared;
            nearestForwardTarget = enemy;
        }
    }

    // 現在の攻撃方向に敵がいる場合は、背後のより近い敵よりも優先する。
    if (nearestForwardTarget) {
        return nearestForwardTarget;
    }

    // 正面の攻撃範囲内に誰もいない場合だけ、全方向の最寄りへ振り向く。
    return nearestAnyDirectionTarget;
}


Enemy* PlayerTargetingAssist::FindAssistStrongTarget(
    const Player& player,
    float maxDistance,
    float attackAngle)
{
    Planet* planet = player.GetCurrentPlanet();
    if (!planet || maxDistance <= 0.0f) {
        return nullptr;
    }

    glm::vec3 normalizedFacingDirection;
    const bool hasFacingDirection =
        TryNormalize(player.GetFacingForwardVec(), normalizedFacingDirection);

    const float facingThreshold = std::cos(attackAngle * 0.5f);
    const float maxDistanceSquared = maxDistance * maxDistance;

    Enemy* nearestForwardTarget = nullptr;
    float nearestForwardDistanceSquared = std::numeric_limits<float>::max();

    Enemy* nearestAnyDirectionTarget = nullptr;
    float nearestAnyDirectionDistanceSquared = std::numeric_limits<float>::max();

    for (Enemy* enemy : planet->GetEnemies()) {
        if (!IsValidEnemy(player, enemy)) {
            continue;
        }

        // アシストStrongは、ガードを全破壊されて打ち上げ状態の敵だけを対象にする。
        if (enemy->GetBreakCount() != 0 || !enemy->IsLaunched()) {
            continue;
        }

        const glm::vec3 toEnemyCenter = enemy->GetPos() - player.GetPos();
        const float distanceSquared =
            CalculateDistanceSquaredToEnemySurface(player, *enemy);
        if (distanceSquared > maxDistanceSquared) {
            continue;
        }

        if (distanceSquared < nearestAnyDirectionDistanceSquared) {
            nearestAnyDirectionDistanceSquared = distanceSquared;
            nearestAnyDirectionTarget = enemy;
        }

        if (!hasFacingDirection) {
            continue;
        }

        glm::vec3 directionToTarget;
        if (!TryNormalize(toEnemyCenter, directionToTarget)) {
            directionToTarget = normalizedFacingDirection;
        }

        const float facingDot =
            glm::dot(normalizedFacingDirection, directionToTarget);

        if (facingDot < facingThreshold) {
            continue;
        }

        if (distanceSquared < nearestForwardDistanceSquared) {
            nearestForwardDistanceSquared = distanceSquared;
            nearestForwardTarget = enemy;
        }
    }

    // 通常攻撃と同様、正面の攻撃範囲内を優先する。
    if (nearestForwardTarget) {
        return nearestForwardTarget;
    }

    return nearestAnyDirectionTarget;
}

Enemy* PlayerTargetingAssist::FindNearestLaunchedTargetOnCurrentPlanet(
    const Player& player)
{
    Planet* planet = player.GetCurrentPlanet();
    if (!planet) {
        return nullptr;
    }

    Enemy* nearestTarget = nullptr;
    float nearestDistanceSquared =
        std::numeric_limits<float>::max();

    for (Enemy* enemy : planet->GetEnemies()) {
        if (!IsActiveLaunchedEnemyOnCurrentPlanet(
                player,
                enemy)) {
            continue;
        }

        const float distanceSquared =
            CalculateDistanceSquaredToEnemySurface(player, *enemy);
        if (distanceSquared >= nearestDistanceSquared) {
            continue;
        }

        nearestTarget = enemy;
        nearestDistanceSquared = distanceSquared;
    }

    return nearestTarget;
}

Enemy* PlayerTargetingAssist::FindNearestLaunchedTargetNearRecoveryOnCurrentPlanet(
    const Player& player,
    float maximumLaunchedTimerSeconds)
{
    Planet* planet = player.GetCurrentPlanet();
    if (!planet ||
        maximumLaunchedTimerSeconds < 0.0f) {
        return nullptr;
    }

    Enemy* nearestTarget = nullptr;
    float nearestDistanceSquared = std::numeric_limits<float>::max();

    for (Enemy* enemy : planet->GetEnemies()) {
        if (!IsActiveLaunchedEnemyOnCurrentPlanet(
                player,
                enemy) ||
            !IsLaunchedEnemyNearRecovery(
                *enemy,
                maximumLaunchedTimerSeconds)) {
            continue;
        }

        const float distanceSquared =
            CalculateDistanceSquaredToEnemySurface(player, *enemy);
        if (distanceSquared >= nearestDistanceSquared) {
            continue;
        }

        nearestTarget = enemy;
        nearestDistanceSquared = distanceSquared;
    }

    return nearestTarget;
}

bool PlayerTargetingAssist::FaceTarget(Player& player, PlayerMovement& movement, const Enemy& target)
{
    glm::vec3 up;
    if (!TryNormalize(player.GetUpVec(), up)) {
        up = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    glm::vec3 tangentDirection;
    if (!TryNormalize(ProjectOntoTangentPlane(target.GetPos() - player.GetPos(), up), tangentDirection)) {
        return false;
    }

    movement.FaceDirection(player, tangentDirection);
    return true;
}
