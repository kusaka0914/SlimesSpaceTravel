#pragma once

#include <glm/glm.hpp>

class Enemy;
class Planet;

struct EnemyAttackFrame {
    glm::vec3 origin{0.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    glm::vec3 forward{0.0f, 0.0f, 1.0f};
    glm::vec3 left{1.0f, 0.0f, 0.0f};
};

struct EnemyMeleeAttackPreviewArea {
    float forwardLength = 0.0f;
    float halfWidth = 0.0f;
};

EnemyAttackFrame ResolveEnemyAttackFrame(const Enemy& enemy);
EnemyMeleeAttackPreviewArea CalculateEnemyMeleeAttackPreviewArea(
    const Enemy& enemy);

bool IsPositionInsideMeleeAttack(
    const EnemyAttackFrame& attackFrame,
    const glm::vec3& position,
    float forwardLength,
    float halfWidth);
bool IsPositionInsideFanAttack(
    const EnemyAttackFrame& attackFrame,
    const glm::vec3& position,
    float range,
    float angleRadians);

bool IsPositionInsideRadialAttack(
    const EnemyAttackFrame& attackFrame,
    const glm::vec3& position,
    float range);

// These use the inverse of the sphere projection used by the attack preview,
// keeping the visible curved area and its hit test on exactly the same surface.
bool IsPositionInsideSphereSurfaceMeleeAttack(
    const Planet& planet,
    const EnemyAttackFrame& attackFrame,
    const glm::vec3& position,
    float forwardLength,
    float halfWidth);
bool IsPositionInsideSphereSurfaceFanAttack(
    const Planet& planet,
    const EnemyAttackFrame& attackFrame,
    const glm::vec3& position,
    float range,
    float angleRadians);
bool IsPositionInsideSphereSurfaceRadialAttack(
    const Planet& planet,
    const EnemyAttackFrame& attackFrame,
    const glm::vec3& position,
    float range);
