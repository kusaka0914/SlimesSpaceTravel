#include "actor/player/PlayerAttackHitDetector.h"

#include "Game.h"

#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "actor/enemy/EnemyCollisionGeometry.h"
#include "actor/player/PlayerCombat.h"
#include "system/PhysicsSystem.h"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

namespace {
constexpr float geometryEpsilon = 0.000001f;
using EnemyCollisionGeometry::CalculateClosestPoint;
using EnemyCollisionGeometry::CalculateSupportDistance;
using EnemyCollisionGeometry::CreateCandidatePoints;
using EnemyCollisionGeometry::DoesSegmentIntersectExpandedBounds;
using EnemyCollisionGeometry::ModelBounds;
using EnemyCollisionGeometry::TryCreateModelBounds;
}

std::vector<Enemy*> PlayerAttackHitDetector::FindHitEnemies(Player& player, const PlayerCombat& combat) const
{
    std::vector<Enemy*> hitEnemies;

    if (!player.GetCurrentPlanet()) {
        return hitEnemies;
    }

    for (Enemy* enemy : player.GetCurrentPlanet()->GetEnemies()) {
        if (!enemy ||
            enemy->GetIsDead() ||
            !enemy->GetIsActive() ||
            !player.GetCurrentPlanet()->
                ArePositionsOnSameSurfaceFace(
                    player.GetPos(),
                    enemy->GetPos())) {
            continue;
        }

        if (combat.GetAttackKind() == PlayerAttackKind::Strong && enemy->GetOnGround()) {
            continue;
        }

        const bool isGroundedEnemyBelowAirAttack =
            combat.IsAirAttacking() &&
            enemy->GetOnGround();
        if (isGroundedEnemyBelowAirAttack &&
            !IsGroundedEnemyWithinAirAttackHeight(
                player,
                *enemy)) {
            continue;
        }

        if (IsEnemyModelHitByAttack(
                player,
                *enemy,
                combat.GetAttackRange(),
                combat.GetAttackAngle())) {
            hitEnemies.push_back(enemy);
        }
    }

    return hitEnemies;
}

bool PlayerAttackHitDetector::
IsGroundedEnemyWithinAirAttackHeight(
    const Player& player,
    const Enemy& enemy) const
{
    const PhysicsSystem* physicsSystem =
        player.GetGame()
            ? player.GetGame()->GetPhysicsSystem()
            : nullptr;
    if (!physicsSystem) {
        return false;
    }

    const glm::vec3 playerUp = player.GetUpVec();
    const float playerUpLengthSquared =
        glm::dot(playerUp, playerUp);
    if (playerUpLengthSquared <= 0.000001f) {
        return false;
    }

    const glm::vec3 upDirection =
        playerUp / std::sqrt(playerUpLengthSquared);
    const float playerHeightAboveEnemy =
        glm::dot(
            player.GetPos() - enemy.GetPos(),
            upDirection);
    if (playerHeightAboveEnemy <= 0.0f) {
        return true;
    }

    const float scaledPlayerHalfHeight =
        physicsSystem->GetPlayerCollisionHeight() *
        player.GetCollisionScaleMultiplier() *
        0.5f;
    constexpr float airAttackDownwardReach = 0.35f;
    ModelBounds enemyBounds;
    const float enemyModelTopOffset =
        TryCreateModelBounds(enemy, enemyBounds)
            ? glm::dot(
                  enemyBounds.center - enemy.GetPos(),
                  upDirection) +
                CalculateSupportDistance(
                    enemyBounds,
                    upDirection)
            : std::max(0.0f, enemy.GetRadius());
    const float maximumReachableHeight =
        enemyModelTopOffset +
        scaledPlayerHalfHeight +
        airAttackDownwardReach;
    return playerHeightAboveEnemy <=
           maximumReachableHeight;
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
            !enemy->GetIsActive() ||
            !currentPlanet->ArePositionsOnSameSurfaceFace(
                player.GetPos(),
                enemy->GetPos())) {
            continue;
        }

        if (IsEnemyModelWithinRadius(
                player,
                *enemy,
                safeRange)) {
            hitEnemies.emplace_back(enemy);
        }
    }

    return hitEnemies;
}

std::vector<Enemy*> PlayerAttackHitDetector::FindEnemiesTouchingAirDodgeMovement(
    Player& player,
    const glm::vec3& movementStart,
    const glm::vec3& movementEnd,
    float horizontalHitboxScale,
    float verticalHitboxScale) const
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
            physicsSystem->GetPlayerCollisionDepth()) *
        player.GetCollisionScaleMultiplier() *
        std::max(0.0f, horizontalHitboxScale);
    const glm::vec3 playerUp = player.GetUpVec();
    const float playerUpLengthSquared =
        glm::dot(playerUp, playerUp);
    if (playerUpLengthSquared <= 0.000001f) {
        return hitEnemies;
    }

    const glm::vec3 upDirection =
        playerUp / std::sqrt(playerUpLengthSquared);
    const float safeVerticalHitboxScale =
        std::max(0.0001f, verticalHitboxScale);
    const auto scaleVerticalDistanceForHitTest =
        [&upDirection, safeVerticalHitboxScale](const glm::vec3& direction) {
            const float verticalDistance =
                glm::dot(direction, upDirection);
            const glm::vec3 horizontalDirection =
                direction - upDirection * verticalDistance;
            return horizontalDirection +
                   upDirection *
                       (verticalDistance /
                        safeVerticalHitboxScale);
        };

    const glm::vec3 movement =
        movementEnd - movementStart;
    const glm::vec3 scaledMovement =
        scaleVerticalDistanceForHitTest(movement);
    const float scaledMovementLengthSquared =
        glm::dot(scaledMovement, scaledMovement);

    for (Enemy* enemy : currentPlanet->GetEnemies()) {
        if (!enemy || enemy->GetIsDead() ||
            !enemy->GetIsActive() ||
            !currentPlanet->ArePositionsOnSameSurfaceFace(
                player.GetPos(),
                enemy->GetPos())) {
            continue;
        }

        ModelBounds enemyBounds;
        if (TryCreateModelBounds(*enemy, enemyBounds)) {
            if (DoesAirDodgePathTouchEnemyModel(
                    player,
                    *enemy,
                    movementStart,
                    movementEnd,
                    horizontalHitboxScale,
                    verticalHitboxScale)) {
                hitEnemies.emplace_back(enemy);
            }
            continue;
        }

        // モデルの境界情報がない古いアセットだけ、従来の半径判定に戻す。
        if (scaledMovementLengthSquared <= geometryEpsilon) {
            continue;
        }

        float positionOnSegment = 0.0f;
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
        const glm::vec3 closestPlayerPosition =
            movementStart + movement * positionOnSegment;
        const float contactDistance =
            playerHorizontalRadius +
            std::max(0.0f, enemy->GetRadius());
        const glm::vec3 playerToEnemy =
            enemy->GetPos() - closestPlayerPosition;
        const glm::vec3 scaledPlayerToEnemy =
            scaleVerticalDistanceForHitTest(playerToEnemy);
        if (glm::dot(scaledPlayerToEnemy, scaledPlayerToEnemy) <=
            contactDistance * contactDistance) {
            hitEnemies.emplace_back(enemy);
        }
    }

    return hitEnemies;
}

bool PlayerAttackHitDetector::IsEnemyModelHitByAttack(
    const Player& player,
    const Enemy& enemy,
    float attackRange,
    float attackAngle) const
{
    ModelBounds enemyBounds;
    if (!TryCreateModelBounds(enemy, enemyBounds)) {
        const glm::vec3 playerToEnemy =
            enemy.GetPos() - player.GetPos();
        const float distance = glm::length(playerToEnemy);
        const glm::vec3 facingDirection =
            glm::normalize(player.GetFacingForwardVec());
        const glm::vec3 directionToEnemy =
            distance > geometryEpsilon
                ? playerToEnemy / distance
                : facingDirection;
        return IsEnemyHitByAttack(
            distance,
            glm::dot(facingDirection, directionToEnemy),
            attackRange + enemy.GetRadius(),
            attackAngle);
    }

    const glm::vec3 facingDirection =
        glm::normalize(player.GetFacingForwardVec());
    const std::array<glm::vec3, 9> candidatePoints =
        CreateCandidatePoints(
            enemyBounds,
            player.GetPos());
    const float facingThreshold =
        std::cos(attackAngle * 0.5f);

    for (const glm::vec3& candidatePoint : candidatePoints) {
        const glm::vec3 playerToCandidate =
            candidatePoint - player.GetPos();
        const float distance = glm::length(playerToCandidate);
        const glm::vec3 directionToCandidate =
            distance > geometryEpsilon
                ? playerToCandidate / distance
                : facingDirection;
        if (distance <= attackRange &&
            glm::dot(facingDirection, directionToCandidate) >=
                facingThreshold) {
            return true;
        }
    }

    return false;
}

bool PlayerAttackHitDetector::IsEnemyModelWithinRadius(
    const Player& player,
    const Enemy& enemy,
    float range) const
{
    ModelBounds enemyBounds;
    if (!TryCreateModelBounds(enemy, enemyBounds)) {
        const glm::vec3 playerToEnemy =
            enemy.GetPos() - player.GetPos();
        const float effectiveRange =
            range + std::max(0.0f, enemy.GetRadius());
        return glm::dot(playerToEnemy, playerToEnemy) <=
            effectiveRange * effectiveRange;
    }

    const glm::vec3 closestPoint =
        CalculateClosestPoint(
            enemyBounds,
            player.GetPos());
    const glm::vec3 playerToClosestPoint =
        closestPoint - player.GetPos();
    return glm::dot(playerToClosestPoint, playerToClosestPoint) <=
        range * range;
}

bool PlayerAttackHitDetector::DoesAirDodgePathTouchEnemyModel(
    const Player& player,
    const Enemy& enemy,
    const glm::vec3& movementStart,
    const glm::vec3& movementEnd,
    float horizontalHitboxScale,
    float verticalHitboxScale) const
{
    ModelBounds enemyBounds;
    if (!TryCreateModelBounds(enemy, enemyBounds)) {
        return false;
    }

    const PhysicsSystem* physicsSystem =
        player.GetGame()
            ? player.GetGame()->GetPhysicsSystem()
            : nullptr;
    if (!physicsSystem) {
        return false;
    }

    const float collisionScaleMultiplier =
        player.GetCollisionScaleMultiplier();
    const float horizontalCollisionRadius =
        0.5f *
        std::max(
            physicsSystem->GetPlayerCollisionWidth(),
            physicsSystem->GetPlayerCollisionDepth()) *
        collisionScaleMultiplier *
        std::max(0.0f, horizontalHitboxScale);
    const float verticalCollisionHalfHeight =
        0.5f *
        physicsSystem->GetPlayerCollisionHeight() *
        collisionScaleMultiplier;
    glm::vec3 playerUpDirection = player.GetUpVec();
    const float playerUpLength = glm::length(playerUpDirection);
    if (playerUpLength <= geometryEpsilon) {
        return false;
    }
    playerUpDirection /= playerUpLength;

    const float verticalCollisionReach =
        verticalCollisionHalfHeight *
        std::max(0.0f, verticalHitboxScale);
    glm::vec3 expansion(0.0f);
    for (glm::length_t axisIndex = 0;
         axisIndex < enemyBounds.axes.size();
         ++axisIndex) {
        const glm::vec3 targetAxis = enemyBounds.axes[axisIndex];
        const float verticalAxisAlignment = glm::clamp(
            std::abs(glm::dot(targetAxis, playerUpDirection)),
            0.0f,
            1.0f);
        const float horizontalAxisAlignment = std::sqrt(
            std::max(
                0.0f,
                1.0f - verticalAxisAlignment * verticalAxisAlignment));




        expansion[axisIndex] =
            horizontalCollisionRadius * horizontalAxisAlignment +
            verticalCollisionReach * verticalAxisAlignment;
    }
    return DoesSegmentIntersectExpandedBounds(
        enemyBounds,
        movementStart,
        movementEnd,
        expansion);
}

bool PlayerAttackHitDetector::IsEnemyHitByAttack(float dist, float dot, float effectiveRange, float attackAngle) const
{
    const float threshold = std::cos(attackAngle * 0.5f);
    return dist <= effectiveRange && dot >= threshold;
}
