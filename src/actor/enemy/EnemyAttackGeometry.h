#pragma once

#include <glm/glm.hpp>

class Enemy;

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

bool IsPositionInsideFanAttack(
    const EnemyAttackFrame& attackFrame,
    const glm::vec3& position,
    float range,
    float angleRadians);

bool IsPositionInsideRadialAttack(
    const EnemyAttackFrame& attackFrame,
    const glm::vec3& position,
    float range);
