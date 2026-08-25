#include "actor/enemy/EnemyAttackGeometry.h"

#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "Game.h"
#include "actor/Player.h"
#include "system/PhysicsSystem.h"
#include "system/mesh/LoadedModel.h"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

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

bool TryGetSpherePreviewPlaneOffset(
    const Planet& planet,
    const EnemyAttackFrame& attackFrame,
    const glm::vec3& position,
    glm::vec3& planeOffset)
{
    if (planet.GetPlanetShape() != Planet::PlanetShape::Sphere) {
        return false;
    }

    constexpr float previewSurfaceOffset = 0.56f;
    constexpr float denominatorEpsilon = 0.0001f;
    const glm::vec3 planeOrigin =
        attackFrame.origin + attackFrame.up * previewSurfaceOffset;
    const glm::vec3 fromPlanetToOrigin =
        planeOrigin - planet.GetPos();
    const glm::vec3 fromPlanetToPosition =
        position - planet.GetPos();
    const float planeRadius = glm::length(fromPlanetToOrigin);
    const float positionRadius = glm::length(fromPlanetToPosition);
    if (planeRadius <= denominatorEpsilon ||
        positionRadius <= denominatorEpsilon) {
        return false;
    }

    const glm::vec3 planeNormal = fromPlanetToOrigin / planeRadius;
    const glm::vec3 positionDirection =
        fromPlanetToPosition / positionRadius;
    const float denominator = glm::dot(positionDirection, planeNormal);

    if (denominator <= denominatorEpsilon) {
        return false;
    }

    planeOffset = planeRadius *
        (positionDirection / denominator - planeNormal);
    return true;
}
}

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

float CalculateEnemyAttackFrontOffset(
    const Enemy& enemy,
    const EnemyAttackFrame& attackFrame)
{
    float frontOffset = std::max(0.0f, enemy.GetRadius());
    const LoadedModel* loadedModel = enemy.GetLoadedModel();
    if (!loadedModel || !loadedModel->hasBounds) {
        return frontOffset;
    }



    const glm::vec3 modelPositiveXAxis = -enemy.GetForwardVec();
    const float frontAxisAlignment =
        glm::dot(modelPositiveXAxis, attackFrame.forward) *
        (enemy.GetScale().x < 0.0f ? -1.0f : 1.0f);
    const float localFrontBound =
        frontAxisAlignment >= 0.0f
            ? loadedModel->boundsMaximum.x
            : loadedModel->boundsMinimum.x;
    return std::max(
        0.0f,
        std::abs(localFrontBound * enemy.GetScale().x));
}

EnemyMeleeAttackPreviewArea CalculateEnemyMeleeAttackPreviewArea(
    const Enemy& enemy,
    const EnemyAttackFrame& attackFrame)
{
    EnemyMeleeAttackPreviewArea previewArea;
    previewArea.forwardLength =
        std::max(0.0f, enemy.GetAttackRange());

    float enemyLateralExtent =
        std::max(0.0f, enemy.GetRadius());
    float enemyForwardStartOffset =
        CalculateEnemyAttackFrontOffset(enemy, attackFrame);
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

    previewArea.forwardStartOffset =
        enemyForwardStartOffset;
    previewArea.forwardLength += targetHorizontalRadius;
    previewArea.halfWidth =
        enemyLateralExtent +
        targetHorizontalRadius;
    return previewArea;
}

bool IsPositionInsideMeleeAttack(
    const EnemyAttackFrame& attackFrame,
    const glm::vec3& position,
    float forwardLength,
    float halfWidth)
{
    if (forwardLength <= 0.0f || halfWidth < 0.0f) {
        return false;
    }

    const glm::vec3 planarOffset =
        ProjectOntoAttackPlane(
            position - attackFrame.origin,
            attackFrame.up);
    const float forwardDistance =
        glm::dot(planarOffset, attackFrame.forward);
    const float sidewaysDistance =
        glm::dot(planarOffset, attackFrame.left);
    return forwardDistance > rangeComparisonTolerance &&
           forwardDistance <= forwardLength + rangeComparisonTolerance &&
           std::abs(sidewaysDistance) <=
               halfWidth + rangeComparisonTolerance;
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

bool IsPositionInsideSphereSurfaceMeleeAttack(
    const Planet& planet,
    const EnemyAttackFrame& attackFrame,
    const glm::vec3& position,
    float forwardLength,
    float halfWidth)
{
    glm::vec3 planeOffset;
    if (!TryGetSpherePreviewPlaneOffset(
            planet, attackFrame, position, planeOffset)) {
        return false;
    }

    return IsPositionInsideMeleeAttack(
        attackFrame,
        attackFrame.origin + planeOffset,
        forwardLength,
        halfWidth);
}

bool IsPositionInsideSphereSurfaceFanAttack(
    const Planet& planet,
    const EnemyAttackFrame& attackFrame,
    const glm::vec3& position,
    float range,
    float angleRadians)
{
    glm::vec3 planeOffset;
    if (!TryGetSpherePreviewPlaneOffset(
            planet, attackFrame, position, planeOffset)) {
        return false;
    }

    const float distanceSquared = glm::dot(planeOffset, planeOffset);
    const float toleratedRange = range + rangeComparisonTolerance;
    if (distanceSquared > toleratedRange * toleratedRange) {
        return false;
    }
    if (distanceSquared <= directionLengthEpsilonSquared) {
        return true;
    }

    const glm::vec3 direction = planeOffset / std::sqrt(distanceSquared);
    return glm::dot(attackFrame.forward, direction) +
               rangeComparisonTolerance >=
           std::cos(angleRadians * 0.5f);
}

bool IsPositionInsideSphereSurfaceRadialAttack(
    const Planet& planet,
    const EnemyAttackFrame& attackFrame,
    const glm::vec3& position,
    float range)
{
    return IsPositionInsideSphereSurfaceFanAttack(
        planet,
        attackFrame,
        position,
        range,
        glm::two_pi<float>());
}
