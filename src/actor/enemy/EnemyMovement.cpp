#include "actor/enemy/EnemyMovement.h"

#include "Game.h"
#include "actor/Enemy.h"
#include "actor/Player.h"
#include "actor/enemy/EnemyStateMachine.h"
#include "actor/enemy/EnemyStatus.h"
#include "system/PhysicsSystem.h"
#include "utils/MathUtils.h"

#include <cmath>
#include <glm/glm.hpp>

void EnemyMovement::UpdateFacingVec(Enemy& enemy, EnemyStatus& status, float deltaTime)
{
    Player* nearestPlayer = status.GetNearestPlayer();
    if (!nearestPlayer) {
        return;
    }

    glm::vec3 toPlayer = nearestPlayer->GetPos() - enemy.GetPos();
    if (glm::length(toPlayer) < 1e-6f) {
        return;
    }

    toPlayer = glm::normalize(toPlayer);
    constexpr float turnSpeed = 5.0f;
    const float t = 1.0f - std::exp(-turnSpeed * deltaTime);

    const glm::vec3 facingForward = glm::normalize(glm::mix(enemy.GetFacingForwardVec(), toPlayer, t));
    enemy.SetFacingForwardForEnemy(facingForward);
    enemy.SetFacingYawForEnemy(enemy.GetGame()->GetMathUtils()->GetYawFromDirection(enemy.GetUpVec(), facingForward) +
                               3.14159265f);
}

void EnemyMovement::MoveToPlayer(Enemy& enemy, const EnemyStatus& status, float deltaTime)
{
    const glm::vec3 moveDelta = enemy.GetFacingForwardVec() * status.GetMoveSpeed() * deltaTime;
    enemy.SetPos(CalculateCollisionAdjustedPos(enemy, moveDelta));
}

void EnemyMovement::MoveDuringAttacking(Enemy& enemy, const EnemyStatus& status, const EnemyStateMachine& stateMachine,
                                        float deltaTime)
{
    glm::vec3 moveDelta;
    if (stateMachine.IsProgressing(status)) {
        moveDelta = enemy.GetFacingForwardVec() * status.GetAttackSpeed() * deltaTime;
    } else {
        moveDelta = -enemy.GetFacingForwardVec() * status.GetAttackSpeed() * deltaTime;
    }

    enemy.SetPos(CalculateCollisionAdjustedPos(enemy, moveDelta));
}

void EnemyMovement::MoveDuringKnockBack(Enemy& enemy, const EnemyStatus& status, float deltaTime)
{
    const glm::vec3 moveDelta = status.GetKnockBackFrom() * status.GetKnockBackSpeed() * deltaTime;
    enemy.SetPos(CalculateCollisionAdjustedPos(enemy, moveDelta));
}

void EnemyMovement::LaunchIntoAir(Enemy& enemy, EnemyStatus& status, EnemyStateMachine& stateMachine, float deltaTime)
{
    enemy.GetGame()->OnEnemyLaunched();

    constexpr float launchSpeed = 5.0f;
    enemy.AddVelocity(enemy.GetUpVec() * launchSpeed);
    enemy.AddPos(enemy.GetVelocity() * deltaTime);

    enemy.SetOnGroundForEnemy(false);
    status.SetStandByAttackTimer(-1.0f);
    status.SetAttackMotionTimer(-1.0f);
    enemy.SetShouldJudgeLandingForEnemy(false);
    enemy.GetGame()->SetHitStopTimer(0.3f);

    stateMachine.StartIdle(enemy);
}

void EnemyMovement::UpdateInAir(Enemy& enemy, EnemyStatus& status, EnemyStateMachine& stateMachine, float deltaTime)
{
    if (status.GetLaunchedTimer() >= 0.0f) {
        status.DecreaseLaunchedTimer(deltaTime);

        if (status.GetLaunchedTimer() >= 0.0f) {
            return;
        }

        stateMachine.FinishLaunched(enemy, status);
        return;
    }

    const glm::vec3 prevVelocity = enemy.GetVelocity();
    enemy.ApplyGravityForEnemy(deltaTime);

    const float vPrev = glm::dot(prevVelocity, enemy.GetUpVec());
    const float vNow = glm::dot(enemy.GetVelocity(), enemy.GetUpVec());

    const bool isTop = vPrev > 0.0f && vNow <= 0.0f;
    if (isTop) {
        status.SetLaunchedTimer(status.GetDefaultLaunchedTimer());
    }
}

glm::vec3 EnemyMovement::CalculateCollisionAdjustedPos(Enemy& enemy, const glm::vec3& moveDelta)
{
    glm::vec3 desiredPos = enemy.GetPos() + moveDelta;

    desiredPos = enemy.GetGame()->GetPhysicsSystem()->CheckCollision(&enemy, moveDelta, desiredPos);

    if (enemy.IsAlive()) {
        desiredPos = mGrounding.ClampMoveToGround(enemy, desiredPos);
    }

    return desiredPos;
}
