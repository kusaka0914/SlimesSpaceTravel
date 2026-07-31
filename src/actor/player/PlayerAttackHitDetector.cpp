#include "actor/player/PlayerAttackHitDetector.h"

#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "actor/player/PlayerCombat.h"
#include "system/PhysicsSystem.h"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

std::vector<Enemy*> PlayerAttackHitDetector::FindHitEnemies(Player& player, const PlayerCombat& combat) const
{
    std::vector<Enemy*> hitEnemies;

    if (!player.GetCurrentPlanet()) {
        return hitEnemies;
    }

    for (Enemy* enemy : player.GetCurrentPlanet()->GetEnemies()) {
        if (enemy->GetIsDead()) {
            continue;
        }

        if (combat.GetAttackKind() == PlayerAttackKind::Strong && enemy->GetOnGround()) {
            continue;
        }

        const glm::vec3 enemyPos = enemy->GetPos();
        const glm::vec3 toEnemy =
            glm::normalize((enemyPos + enemy->GetFacingForwardVec() * (enemy->GetRadius() - 1.0f)) - player.GetPos());

        const float dist = glm::length(enemyPos - player.GetPos());
        const float dot = glm::dot(player.GetFacingForwardVec(), toEnemy);
        const float effectiveRange = combat.GetAttackRange() + enemy->GetRadius();

        if (IsEnemyHitByAttack(dist, dot, effectiveRange, combat.GetAttackAngle())) {
            hitEnemies.push_back(enemy);
        }
    }

    return hitEnemies;
}

std::vector<Enemy*> PlayerAttackHitDetector::FindEnemiesInRadius(
    Player& player,
    float range) const
{
    std::vector<Enemy*> hitEnemies;

    Planet* currentPlanet = player.GetCurrentPlanet();
    if (!currentPlanet) {
        return hitEnemies;
    }

    const float safeRange =
        std::max(0.0f, range);
    for (Enemy* enemy : currentPlanet->GetEnemies()) {
        if (!enemy || enemy->GetIsDead() ||
            !enemy->GetIsActive()) {
            continue;
        }

        const float effectiveRange =
            safeRange +
            std::max(0.0f, enemy->GetRadius());
        const glm::vec3 playerToEnemy =
            enemy->GetPos() - player.GetPos();
        if (glm::dot(playerToEnemy, playerToEnemy) <=
            effectiveRange * effectiveRange) {
            hitEnemies.emplace_back(enemy);
        }
    }

    return hitEnemies;
}

std::vector<Enemy*> PlayerAttackHitDetector::FindEnemiesTouchingAirDodgeMovement(
    Player& player,
    const glm::vec3& movementStart,
    const glm::vec3& movementEnd) const
{
    std::vector<Enemy*> hitEnemies;

    Planet* currentPlanet = player.GetCurrentPlanet();
    PhysicsSystem* physicsSystem =
        player.GetGame()
            ? player.GetGame()->GetPhysicsSystem()
            : nullptr;
    if (!currentPlanet || !physicsSystem) {
        return hitEnemies;
    }

    const float playerHorizontalRadius =
        0.5f *
        std::max(
            physicsSystem->GetPlayerCollisionWidth(),
            physicsSystem->GetPlayerCollisionDepth());
    const glm::vec3 playerUp = player.GetUpVec();
    const float playerUpLengthSquared =
        glm::dot(playerUp, playerUp);
    if (playerUpLengthSquared <= 0.000001f) {
        return hitEnemies;
    }

    const glm::vec3 upDirection =
        playerUp / std::sqrt(playerUpLengthSquared);
    constexpr float verticalHitRangeMultiplier = 2.0f;
    const auto scaleVerticalDistanceForHitTest =
        [&upDirection](const glm::vec3& direction) {
            const float verticalDistance =
                glm::dot(direction, upDirection);
            const glm::vec3 horizontalDirection =
                direction - upDirection * verticalDistance;
            return horizontalDirection +
                   upDirection *
                       (verticalDistance /
                        verticalHitRangeMultiplier);
        };

    const glm::vec3 movement =
        movementEnd - movementStart;
    const glm::vec3 scaledMovement =
        scaleVerticalDistanceForHitTest(movement);
    const float scaledMovementLengthSquared =
        glm::dot(scaledMovement, scaledMovement);

    for (Enemy* enemy : currentPlanet->GetEnemies()) {
        if (!enemy || enemy->GetIsDead() ||
            !enemy->GetIsActive()) {
            continue;
        }

        float positionOnSegment = 0.0f;
        if (scaledMovementLengthSquared > 0.000001f) {
            const glm::vec3 scaledEnemyOffset =
                scaleVerticalDistanceForHitTest(
                    enemy->GetPos() - movementStart);
            positionOnSegment =
                glm::dot(
                    scaledEnemyOffset,
                    scaledMovement) /
                scaledMovementLengthSquared;
            positionOnSegment =
                std::clamp(positionOnSegment, 0.0f, 1.0f);
        }

        const glm::vec3 closestPlayerPosition =
            movementStart + movement * positionOnSegment;
        const float contactDistance =
            playerHorizontalRadius +
            std::max(0.0f, enemy->GetRadius());
        const glm::vec3 playerToEnemy =
            enemy->GetPos() - closestPlayerPosition;
        const glm::vec3 scaledPlayerToEnemy =
            scaleVerticalDistanceForHitTest(playerToEnemy);
        if (glm::dot(
                scaledPlayerToEnemy,
                scaledPlayerToEnemy) <=
            contactDistance * contactDistance) {
            hitEnemies.emplace_back(enemy);
        }
    }

    return hitEnemies;
}

bool PlayerAttackHitDetector::IsEnemyHitByAttack(float dist, float dot, float effectiveRange, float attackAngle) const
{
    const float threshold = std::cos(attackAngle * 0.5f);
    return dist <= effectiveRange && dot >= threshold;
}
