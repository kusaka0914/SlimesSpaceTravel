#include "actor/enemy/EnemyStateMachine.h"
#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "actor/Star.h"
#include "actor/enemy/EnemyCombat.h"
#include "actor/enemy/EnemyMovement.h"
#include "actor/enemy/EnemyStatus.h"
#include "system/AudioSystem.h"
#include "Game.h"
#include <glm/glm.hpp>

void EnemyStateMachine::UpdateAlive(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, EnemyCombat& combat,
                                    float deltaTime)
{
    if (enemy.GetActionState() == Enemy::ActionState::KnockedBack) {
        UpdateKnockedBack(enemy, status, movement, deltaTime);

        if (!enemy.IsOnGround()) {
            movement.UpdateInAir(enemy, status, *this, deltaTime);
        }

        return;
    }

    if (enemy.IsOnGround()) {
        UpdateBehavior(enemy, status, movement, combat, deltaTime);
        return;
    }

    movement.UpdateInAir(enemy, status, *this, deltaTime);
}

void EnemyStateMachine::UpdateDying(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, float deltaTime)
{
    movement.MoveDuringKnockBack(enemy, status, deltaTime);

    if (!enemy.IsOnGround()) {
        movement.UpdateInAir(enemy, status, *this, deltaTime);
    }

    status.DecreaseDyingTimer(deltaTime);
    if (status.GetDyingTimer() <= 0.0f) {
        FinishDying(enemy, status);
    }
}

void EnemyStateMachine::UpdateBehavior(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, EnemyCombat& combat,
                                       float deltaTime)
{
    switch (enemy.GetActionState()) {
    case Enemy::ActionState::Idle:
        UpdateIdle(enemy, status, combat);
        break;

    case Enemy::ActionState::Tracking:
        UpdateTracking(enemy, status, movement, combat, deltaTime);
        break;

    case Enemy::ActionState::PreparingAttack:
        UpdatePreparingAttack(enemy, status, movement, deltaTime);
        break;

    case Enemy::ActionState::Attacking:
        UpdateAttacking(enemy, status, movement, combat, deltaTime);
        break;

    case Enemy::ActionState::KnockedBack:
        UpdateKnockedBack(enemy, status, movement, deltaTime);
        break;
    }
}

void EnemyStateMachine::UpdateIdle(Enemy& enemy, EnemyStatus& status, EnemyCombat& combat)
{
    if (combat.IsPlayerInRange(enemy, status.GetNearestPlayer(), status.GetDetectionRange())) {
        StartTracking(enemy);
    }
}

void EnemyStateMachine::UpdateTracking(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, EnemyCombat& combat,
                                       float deltaTime)
{
    movement.UpdateFacingVec(enemy, status, deltaTime);
    movement.MoveToPlayer(enemy, status, deltaTime);
    TryStartPreparingAttack(enemy, status, combat);
}

void EnemyStateMachine::TryStartPreparingAttack(Enemy& enemy, EnemyStatus& status, EnemyCombat& combat)
{
    constexpr float attackStartRangeMargin = 1.5f;
    const float attackStartRange = enemy.GetRadius() + attackStartRangeMargin;

    if (combat.IsPlayerInRange(enemy, status.GetNearestPlayer(), attackStartRange)) {
        StartPreparingAttack(enemy, status);
    }
}

void EnemyStateMachine::UpdatePreparingAttack(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, float deltaTime)
{
    if (!status.GetIsJustBeforeAttack()) {
        movement.UpdateFacingVec(enemy, status, deltaTime);
    }

    status.DecreaseStandByAttackTimer(deltaTime);

    if (IsJustBeforeAttack(status)) {
        status.SetIsJustBeforeAttack(true);
        enemy.GetGame()->GetAudioSystem()->PlaySE("attack_pre_se");
    }

    if (status.GetStandByAttackTimer() <= 0.0f) {
        StartAttacking(enemy, status);
    }
}

void EnemyStateMachine::UpdateAttacking(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, EnemyCombat& combat,
                                        float deltaTime)
{
    movement.MoveDuringAttacking(enemy, status, *this, deltaTime);
    combat.TryApplyAttack(enemy, status, *this, deltaTime);

    status.DecreaseCanCounteredTimer(deltaTime);
    if (status.GetCanCounteredTimer() <= 0.0f) {
        status.SetCanCountered(false);
    }

    status.DecreaseAttackMotionTimer(deltaTime);
    if (status.GetAttackMotionTimer() <= 0.0f) {
        StartIdle(enemy);
    }
}

void EnemyStateMachine::UpdateKnockedBack(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, float deltaTime)
{
    movement.MoveDuringKnockBack(enemy, status, deltaTime);

    status.DecreaseKnockBackTimer(deltaTime);
    if (status.GetKnockBackTimer() <= 0.0f) {
        StartIdle(enemy);
    }
}

void EnemyStateMachine::StartIdle(Enemy& enemy)
{
    enemy.SetActionState(Enemy::ActionState::Idle);
}

void EnemyStateMachine::StartTracking(Enemy& enemy)
{
    enemy.SetActionState(Enemy::ActionState::Tracking);
}

void EnemyStateMachine::StartPreparingAttack(Enemy& enemy, EnemyStatus& status)
{
    enemy.SetActionState(Enemy::ActionState::PreparingAttack);
    status.ResetStandByAttackTimer();
}

void EnemyStateMachine::StartAttacking(Enemy& enemy, EnemyStatus& status)
{
    enemy.SetActionState(Enemy::ActionState::Attacking);
    status.ResetAttackMotionTimer();
    status.ClearIsHit();
    status.SetIsJustBeforeAttack(false);
    status.SetCanCounteredTimer(0.1f);
    status.SetCanCountered(true);
    status.ClearHitPlayers();
}

void EnemyStateMachine::StartKnockedBack(Enemy& enemy, EnemyStatus& status, float knockBackTimer)
{
    enemy.SetActionState(Enemy::ActionState::KnockedBack);
    status.SetKnockBackTimer(knockBackTimer);

    if (status.GetNearestPlayer()) {
        const glm::vec3 knockBack = enemy.GetPos() - status.GetNearestPlayer()->GetPos();
        if (glm::length(knockBack) > 1e-6f) {
            status.SetKnockBackFrom(glm::normalize(knockBack));
        }
    } else if (glm::length(enemy.GetFacingForwardVec()) > 1e-6f) {
        status.SetKnockBackFrom(-glm::normalize(enemy.GetFacingForwardVec()));
    } else {
        status.SetKnockBackFrom(-enemy.GetForwardVec());
    }

    status.ClearLaunchedTimer();
}

void EnemyStateMachine::StartDying(Enemy& enemy, EnemyStatus& status)
{
    enemy.SetLifeState(Enemy::LifeState::Dying);
    status.SetDyingTimer(1.0f);
    status.SetHpZero();

    constexpr float dyingKnockBackTimer = 0.5f;
    StartKnockedBack(enemy, status, dyingKnockBackTimer);
    enemy.GetGame()->GetAudioSystem()->PlaySE("defeat_se");
}

void EnemyStateMachine::FinishDying(Enemy& enemy, const EnemyStatus& status)
{
    enemy.SetLifeState(Enemy::LifeState::Dead);
    enemy.SetIsActive(false);

    if (enemy.GetCurrentPlanet()) {
        enemy.GetCurrentPlanet()->OnEnemyDead();
    }

    if (!status.GetIsBoss() || !enemy.GetCurrentPlanet()) {
        return;
    }

    Star* star = enemy.GetCurrentPlanet()->GetStar();
    if (!star) {
        return;
    }

    star->SetIsActive(true);
}

void EnemyStateMachine::FinishLaunched(Enemy& enemy, EnemyStatus& status)
{
    status.ResetBreakCount();
    enemy.SetShouldJudgeLandingForEnemy(true);
    status.ClearLaunchedTimer();
}

bool EnemyStateMachine::IsJustBeforeAttack(const EnemyStatus& status) const
{
    if (status.GetIsJustBeforeAttack()) {
        return false;
    }

    constexpr float justBeforeAttackTime = 1.0f;
    return status.GetStandByAttackTimer() <= justBeforeAttackTime;
}

bool EnemyStateMachine::IsProgressing(const EnemyStatus& status) const
{
    return status.GetAttackMotionTimer() >= status.GetDefaultAttackMotionTimer() / 2.0f;
}

bool EnemyStateMachine::IsAlive(const Enemy& enemy) const
{
    return enemy.GetLifeState() == Enemy::LifeState::Alive;
}
