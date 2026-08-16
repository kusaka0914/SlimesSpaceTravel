#include "actor/enemy/EnemyAttackGeometry.h"

#include "actor/Enemy.h"
#include "Game.h"
#include "actor/Player.h"
#include "system/PhysicsSystem.h"
#include "system/mesh/LoadedModel.h"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

namespace {
constexpr float directionLengthEpsilonSquared = 0.000001f;
constexpr float rangeComparisonTolerance = 0.0001f;

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

glm::vec3 ProjectOntoAttackPlane(
    const glm::vec3& direction,
    const glm::vec3& upDirection)
{
    return direction -
        upDirection *
            glm::dot(direction, upDirection);
}
} // namespace

EnemyAttackFrame ResolveEnemyAttackFrame(const Enemy& enemy)
{
    EnemyAttackFrame attackFrame;
    attackFrame.origin = enemy.GetPos();

    if (!TryNormalizeDirection(
            enemy.GetUpVec(),
            attackFrame.up)) {
        attackFrame.up = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    const glm::vec3 tangentialFacingDirection =
        ProjectOntoAttackPlane(
            enemy.GetFacingForwardVec(),
            attackFrame.up);
    if (!TryNormalizeDirection(
            tangentialFacingDirection,
            attackFrame.forward)) {
        const glm::vec3 tangentialModelForward =
            ProjectOntoAttackPlane(
                enemy.GetForwardVec(),
                attackFrame.up);
        if (!TryNormalizeDirection(
                tangentialModelForward,
                attackFrame.forward)) {
            attackFrame.forward = glm::vec3(0.0f, 0.0f, 1.0f);
        }
    }

    const glm::vec3 leftDirection =
        glm::cross(
            attackFrame.up,
            attackFrame.forward);
    if (!TryNormalizeDirection(
            leftDirection,
            attackFrame.left)) {
        attackFrame.left = glm::vec3(1.0f, 0.0f, 0.0f);
    }

    return attackFrame;
}

EnemyMeleeAttackPreviewArea CalculateEnemyMeleeAttackPreviewArea(
    const Enemy& enemy)
{
    EnemyMeleeAttackPreviewArea previewArea;
    previewArea.forwardLength =
        std::max(0.0f, enemy.GetAttackRange());

    float enemyLateralExtent =
        std::max(0.0f, enemy.GetRadius());
    float enemyForwardExtent =
        enemyLateralExtent;
    const LoadedModel* loadedModel =
        enemy.GetLoadedModel();
    if (loadedModel && loadedModel->hasBounds) {
        const glm::vec3 absoluteScale =
            glm::abs(enemy.GetScale());
        enemyLateralExtent =
            std::max(
                std::abs(
                    loadedModel->boundsMinimum.z *
                    absoluteScale.z),
                std::abs(
                    loadedModel->boundsMaximum.z *
                    absoluteScale.z));
        enemyForwardExtent =
            std::max(
                std::abs(
                    loadedModel->boundsMinimum.x *
                    absoluteScale.x),
                std::abs(
                    loadedModel->boundsMaximum.x *
                    absoluteScale.x));
    }

    float targetHorizontalRadius = 0.0f;
    const Game* game = enemy.GetGame();
    const PhysicsSystem* physicsSystem =
        game ? game->GetPhysicsSystem() : nullptr;
    if (physicsSystem) {
        float maximumTargetCollisionScale = 1.0f;
        for (const Player* player : game->GetPlayers()) {
            if (!player || !player->GetIsActive()) {
                continue;
            }
            maximumTargetCollisionScale =
                std::max(
                    maximumTargetCollisionScale,
                    player->GetCollisionScaleMultiplier());
        }

        targetHorizontalRadius =
            0.5f *
            std::max(
                physicsSystem->GetPlayerCollisionWidth(),
                physicsSystem->GetPlayerCollisionDepth()) *
            maximumTargetCollisionScale;
    }

    previewArea.forwardLength +=
        enemyForwardExtent +
        targetHorizontalRadius;
    previewArea.halfWidth =
        enemyLateralExtent +
        targetHorizontalRadius;
    return previewArea;
}

bool IsPositionInsideFanAttack(
    const EnemyAttackFrame& attackFrame,
    const glm::vec3& position,
    float range,
    float angleRadians)
{
    if (range <= 0.0f ||
        angleRadians <= 0.0f) {
        return false;
    }

    const glm::vec3 planarOffset =
        ProjectOntoAttackPlane(
            position - attackFrame.origin,
            attackFrame.up);
    const float planarDistanceSquared =
        glm::dot(planarOffset, planarOffset);
    const float toleratedRange =
        range + rangeComparisonTolerance;
    if (planarDistanceSquared >
        toleratedRange * toleratedRange) {
        return false;
    }

    if (planarDistanceSquared <=
        directionLengthEpsilonSquared) {
        return true;
    }

    const glm::vec3 directionToPosition =
        planarOffset /
        std::sqrt(planarDistanceSquared);
    const float halfAngleThreshold =
        std::cos(angleRadians * 0.5f);
    return glm::dot(
               attackFrame.forward,
               directionToPosition) +
            rangeComparisonTolerance >=
        halfAngleThreshold;
}

bool IsPositionInsideRadialAttack(
    const EnemyAttackFrame& attackFrame,
    const glm::vec3& position,
    float range)
{
    if (range <= 0.0f) {
        return false;
    }

    const glm::vec3 planarOffset =
        ProjectOntoAttackPlane(
            position - attackFrame.origin,
            attackFrame.up);
    const float toleratedRange =
        range + rangeComparisonTolerance;
    return glm::dot(planarOffset, planarOffset) <=
        toleratedRange * toleratedRange;
}
