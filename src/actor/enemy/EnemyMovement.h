#pragma once

#include <glm/glm.hpp>

class Enemy;
class EnemyStateMachine;
class EnemyStatus;

class EnemyMovement {
public:
    void UpdateFacingVec(Enemy& enemy, EnemyStatus& status, float deltaTime);

    void MoveToPlayer(Enemy& enemy, const EnemyStatus& status, float deltaTime);
    void MoveDuringAttacking(Enemy& enemy, const EnemyStatus& status, const EnemyStateMachine& stateMachine, float deltaTime);
    void MoveDuringKnockBack(Enemy& enemy, const EnemyStatus& status, float deltaTime);

    void LaunchIntoAir(Enemy& enemy, EnemyStatus& status, EnemyStateMachine& stateMachine, float deltaTime);
    void UpdateInAir(Enemy& enemy, EnemyStatus& status, EnemyStateMachine& stateMachine, float deltaTime);

    glm::vec3 CalculateCollisionAdjustedPos(Enemy& enemy, const glm::vec3& moveDelta);
    glm::vec3 ClampMoveToGround(const Enemy& enemy, const glm::vec3& desiredPos) const;
    bool HasGroundBelow(const Enemy& enemy, const glm::vec3& checkPos) const;
};
