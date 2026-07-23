#include "actor/enemy/EnemyStateMachine.h"

#include "Game.h"
#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "actor/Star.h"
#include "actor/enemy/EnemyCombat.h"
#include "actor/enemy/EnemyMovement.h"
#include "actor/enemy/EnemyStatus.h"
#include "system/AudioSystem.h"
#include "system/ParticleSystem.h"

#include <glm/glm.hpp>

namespace {

constexpr float directionEpsilon = 1e-6f;

bool TryNormalize(const glm::vec3& value, glm::vec3& normalizedValue)
{
    const float length = glm::length(value);
    if (length < directionEpsilon) {
        return false;
    }

    normalizedValue = value / length;
    return true;
}

glm::vec3 GetNormalizedUpDirection(const Enemy& enemy)
{
    glm::vec3 upDirection;
    if (TryNormalize(enemy.GetUpVec(), upDirection)) {
        return upDirection;
    }

    return glm::vec3(0.0f, 1.0f, 0.0f);
}

glm::vec3 ProjectOntoTangentPlane(const glm::vec3& direction, const glm::vec3& upDirection)
{
    return direction - upDirection * glm::dot(direction, upDirection);
}

glm::vec3 GetDyingKnockBackDirection(const Enemy& enemy, const EnemyStatus& status,
                                      const glm::vec3& upDirection)
{
    glm::vec3 knockBackDirection;
    const glm::vec3 projectedKnockBack = ProjectOntoTangentPlane(status.GetKnockBackFrom(), upDirection);

    if (TryNormalize(projectedKnockBack, knockBackDirection)) {
        return knockBackDirection;
    }

    const glm::vec3 projectedFacingDirection =
        ProjectOntoTangentPlane(-enemy.GetFacingForwardVec(), upDirection);

    if (TryNormalize(projectedFacingDirection, knockBackDirection)) {
        return knockBackDirection;
    }

    return glm::vec3(0.0f);
}

void InitializeDyingMovement(Enemy& enemy, EnemyStatus& status)
{
    const glm::vec3 upDirection = GetNormalizedUpDirection(enemy);
    const glm::vec3 knockBackDirection = GetDyingKnockBackDirection(enemy, status, upDirection);

    constexpr float dyingLaunchSpeed = 5.0f;
    const glm::vec3 dyingVelocity =
        knockBackDirection * status.GetKnockBackSpeed() + upDirection * dyingLaunchSpeed;

    status.SetKnockBackFrom(knockBackDirection);
    enemy.SetVelocity(dyingVelocity);
    enemy.SetOnGroundForEnemy(false);

    // 死亡演出中に接地判定で速度を0へ戻されないようにする。
    // 消滅までの短時間は、衝突解決を行いながら放物運動させる。
    enemy.SetShouldJudgeLandingForEnemy(false);
}

void EmitEnemyDefeatEffect(Enemy& enemy, const EnemyStatus& status)
{
    ParticleSystem* particleSystem = enemy.GetGame()->GetParticleSystem();
    if (!particleSystem) {
        return;
    }

    const glm::vec3 upDirection = GetNormalizedUpDirection(enemy);

    ParticleSpawnContext context;
    context.position = enemy.GetPos() + upDirection * enemy.GetRadius() * 0.5f;
    context.normal = upDirection;
    context.direction = upDirection;
    context.scale = status.GetIsBoss() ? 1.8f : 1.0f;

    particleSystem->Emit("enemy_defeat", context);
}

} // namespace

EnemyStateMachine::EnemyStateMachine()
    : mLifeState(LifeState::Alive),
      mActionState(ActionState::Idle)
{
}

void EnemyStateMachine::UpdateAlive(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, EnemyCombat& combat,
                                     float deltaTime)
{
    if (mActionState == ActionState::KnockedBack) {
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
    movement.MoveDuringDying(enemy, deltaTime);

    status.DecreaseDyingTimer(deltaTime);
    if (status.GetDyingTimer() <= 0.0f) {
        FinishDying(enemy, status);
    }
}

void EnemyStateMachine::UpdateBehavior(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, EnemyCombat& combat,
                                        float deltaTime)
{
    switch (mActionState) {
    case ActionState::Idle:
        UpdateIdle(enemy, status, combat);
        break;

    case ActionState::Tracking:
        UpdateTracking(enemy, status, movement, combat, deltaTime);
        break;

    case ActionState::PreparingAttack:
        UpdatePreparingAttack(enemy, status, movement, deltaTime);
        break;

    case ActionState::Attacking:
        UpdateAttacking(enemy, status, movement, combat, deltaTime);
        break;

    case ActionState::KnockedBack:
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

void EnemyStateMachine::UpdatePreparingAttack(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement,
                                               float deltaTime)
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
    (void)enemy;
    mActionState = ActionState::Idle;
}

void EnemyStateMachine::StartTracking(Enemy& enemy)
{
    (void)enemy;
    mActionState = ActionState::Tracking;
}

void EnemyStateMachine::StartPreparingAttack(Enemy& enemy, EnemyStatus& status)
{
    (void)enemy;
    mActionState = ActionState::PreparingAttack;
    status.ResetStandByAttackTimer();
}

void EnemyStateMachine::StartAttacking(Enemy& enemy, EnemyStatus& status)
{
    (void)enemy;
    mActionState = ActionState::Attacking;
    status.ResetAttackMotionTimer();
    status.ClearIsHit();
    status.SetIsJustBeforeAttack(false);
    status.SetCanCounteredTimer(0.1f);
    status.SetCanCountered(true);
    status.ClearHitPlayers();
}

void EnemyStateMachine::StartKnockedBack(Enemy& enemy, EnemyStatus& status, float knockBackTimer)
{
    mActionState = ActionState::KnockedBack;
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
    mLifeState = LifeState::Dying;
    status.SetDyingTimer(1.0f);
    status.SetHpZero();

    constexpr float dyingKnockBackTimer = 0.5f;
    StartKnockedBack(enemy, status, dyingKnockBackTimer);
    InitializeDyingMovement(enemy, status);

    enemy.GetGame()->GetAudioSystem()->PlaySE("defeat_se");
}

void EnemyStateMachine::FinishDying(Enemy& enemy, const EnemyStatus& status)
{
    EmitEnemyDefeatEffect(enemy, status);

    mLifeState = LifeState::Dead;
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
    (void)enemy;
    return IsAlive();
}
