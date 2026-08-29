#pragma once

#include <glm/glm.hpp>

class Enemy;
class PhysicsSystem;

class EnemyGrounding {
public:
    explicit EnemyGrounding(PhysicsSystem& physicsSystem);

    glm::vec3 ClampMoveToGround(const Enemy& enemy, const glm::vec3& desiredPos) const;
    bool HasGroundBelow(const Enemy& enemy, const glm::vec3& checkPos) const;

private:
    PhysicsSystem& mPhysicsSystem;
};
