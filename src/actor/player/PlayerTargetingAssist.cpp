#include "actor/player/PlayerTargetingAssist.h"

#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "actor/Player.h"
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
    return enemy && enemy->GetIsActive() && enemy->IsAlive() && !enemy->GetIsDead() &&
           enemy->GetCurrentPlanet() == player.GetCurrentPlanet();
}
} // namespace

Enemy* PlayerTargetingAssist::FindAttackTarget(
    const Player& player,
    float attackRange,
    float attackAngle,
    bool requireAirborneTarget)
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

        if (requireAirborneTarget && enemy->IsOnGround()) {
            continue;
        }

        const glm::vec3 toEnemyCenter = enemy->GetPos() - player.GetPos();
        const float distanceSquared = glm::dot(toEnemyCenter, toEnemyCenter);

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

        // PlayerAttackHitDetectorと同じ照準点・角度条件で、
        // 現在向いている方向の攻撃範囲内か判定する。
        const glm::vec3 targetPoint =
            enemy->GetPos() +
            enemy->GetFacingForwardVec() * (enemy->GetRadius() - 1.0f);

        glm::vec3 directionToTarget;
        if (!TryNormalize(targetPoint - player.GetPos(), directionToTarget)) {
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


Enemy* PlayerTargetingAssist::FindAssistStrongTarget(const Player& player, float maxDistance)
{
    Planet* planet = player.GetCurrentPlanet();
    if (!planet || maxDistance <= 0.0f) {
        return nullptr;
    }

    const float maxDistanceSquared = maxDistance * maxDistance;
    Enemy* nearestTarget = nullptr;
    float nearestDistanceSquared = maxDistanceSquared;

    for (Enemy* enemy : planet->GetEnemies()) {
        if (!IsValidEnemy(player, enemy)) {
            continue;
        }

        // ガードを全破壊され、打ち上げられている敵だけを対象にする。
        if (enemy->GetBreakCount() != 0 || enemy->IsOnGround()) {
            continue;
        }

        const glm::vec3 toEnemy = enemy->GetPos() - player.GetPos();
        const float distanceSquared = glm::dot(toEnemy, toEnemy);
        if (distanceSquared > nearestDistanceSquared) {
            continue;
        }

        nearestDistanceSquared = distanceSquared;
        nearestTarget = enemy;
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
