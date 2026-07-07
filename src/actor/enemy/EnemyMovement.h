#pragma once

#include "actor/enemy/EnemyGrounding.h"

#include <glm/glm.hpp>

class Enemy;
class EnemyStateMachine;
class EnemyStatus;

class EnemyMovement {
public:
    void UpdateFacingVec(Enemy& enemy, EnemyStatus& status, float deltaTime);

    void MoveToPlayer(Enemy& enemy, const EnemyStatus& status, float deltaTime);
    void MoveDuringAttacking(Enemy& enemy, const EnemyStatus& status, const EnemyStateMachine& stateMachine,
                             float deltaTime);
    void MoveDuringKnockBack(Enemy& enemy, const EnemyStatus& status, float deltaTime);

    void LaunchIntoAir(Enemy& enemy, EnemyStatus& status, EnemyStateMachine& stateMachine, float deltaTime);
    void UpdateInAir(Enemy& enemy, EnemyStatus& status, EnemyStateMachine& stateMachine, float deltaTime);

    glm::vec3 CalculateCollisionAdjustedPos(Enemy& enemy, const glm::vec3& moveDelta);

private:
    EnemyGrounding mGrounding;
};
