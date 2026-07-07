#pragma once

#include <glm/glm.hpp>

class Enemy;

class EnemyGrounding {
public:
    glm::vec3 ClampMoveToGround(const Enemy& enemy, const glm::vec3& desiredPos) const;
    bool HasGroundBelow(const Enemy& enemy, const glm::vec3& checkPos) const;
};
