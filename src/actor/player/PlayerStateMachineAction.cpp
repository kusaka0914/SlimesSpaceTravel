#include "actor/player/PlayerStateMachine.h"

#include "actor/Enemy.h"
#include "actor/Player.h"
#include "actor/player/PlayerCombat.h"
#include "actor/player/PlayerGrounding.h"
#include "actor/player/PlayerInput.h"
#include "actor/player/PlayerJewelGauge.h"
#include "actor/player/PlayerMovement.h"
#include "actor/player/PlayerStatus.h"
#include "actor/player/PlayerTargetingAssist.h"
#include "system/AudioSystem.h"

void PlayerStateMachine::UpdateDodging(Player& player, PlayerInput& input, PlayerMovement& movement,
                                       PlayerGrounding& grounding, PlayerCombat& combat, float deltaTime)
{
    const glm::vec3 movementStart = player.GetPos();
    movement.ApplyDodgeMovement(player, combat, grounding, deltaTime);
    combat.UpdateAirDodgeAttack(
        player,
        movement,
        movementStart,
        player.GetPos());

    movement.ReduceDodgeTimer(deltaTime);
    if (movement.GetDodgeTimer() <= 0.0f) {
        const bool didFinishAirDodge =
            combat.IsAirDodgeAttackActive() &&
            !player.GetOnGround();
        const bool shouldResumeAirMovementImmediately =
            didFinishAirDodge && mShouldSkipAirDodgePostHover;
        combat.EndAirDodgeAttack();
        if (didFinishAirDodge && !shouldResumeAirMovementImmediately) {
            movement.StopAirborneVerticalMovement(player);
            movement.StartAirborneActionHover(
                movement.GetAirDodgePostHoverDurationSeconds());
        }
        if (shouldResumeAirMovementImmediately) {
            // 攻撃から回避したケースだけは、回避終了フレームから
            // 空中入力と落下を再開する。これで着地まで停止しない。
            combat.CancelCurrentAttack();
            movement.CancelAirborneActionHover();
            movement.ApplyJumpGravityAndInputMovement(
                player,
                input,
                deltaTime);
        }
        mShouldSkipAirDodgePostHover = false;
        StartIdle();
    }
}

void PlayerStateMachine::UpdateAttacking(Player& player, PlayerInput& input, PlayerMovement& movement,
                                         PlayerCombat& combat, PlayerStatus& status, float deltaTime)
{
    if (!player.GetOnGround()) {
        movement.StopAirborneVerticalMovement(player);
    }

    const bool wasAirWeakAttacking = !player.GetOnGround();
    if (TryStartDodging(
            player,
            input,
            movement,
            combat,
            status)) {
        if (wasAirWeakAttacking) {
            combat.CancelAirAttackForDodge();
        } else {
            combat.CancelCurrentAttack();
        }
        // 空中弱攻撃そのものは着地まで移動不能のままにするが、
        // 回避でキャンセルできた場合は通常操作へ即座に戻す。
        mShouldSkipAirDodgePostHover = wasAirWeakAttacking;
        mAllowsAirMovementAfterDodge = wasAirWeakAttacking;
        mAttackDirectionTarget = nullptr;
        return;
    }

    if (combat.HasPendingAttackHit()) {
        const bool hasValidDirectionTarget =
            mAttackDirectionTarget &&
            mAttackDirectionTarget->GetIsActive() &&
            mAttackDirectionTarget->IsAlive() &&
            !mAttackDirectionTarget->GetIsDead() &&
            mAttackDirectionTarget->GetCurrentPlanet() == player.GetCurrentPlanet();

        if (hasValidDirectionTarget) {
            // 攻撃開始時に選んだ最寄りの敵へ、判定が出る瞬間まで向きを維持する。
            PlayerTargetingAssist::FaceTarget(player, movement, *mAttackDirectionTarget);
        } else {
            mAttackDirectionTarget = nullptr;
            movement.UpdateFacingDirectionFromInput(player, input);
        }

        const bool didResolveAttack = combat.UpdatePendingAttackHit(player, movement, status, deltaTime);
        if (didResolveAttack) {
            mAttackDirectionTarget = nullptr;
        }
        return;
    }

    if (player.GetOnGround()) {
        movement.ApplyAttackMovement(player, combat, deltaTime);
    }

    combat.ReduceAttackMotionTimer(deltaTime);
    if (combat.GetAttackMotionTimer() <= 0.0f) {
        mAttackDirectionTarget = nullptr;
        if (!player.GetOnGround() &&
            combat.IsAirAttacking()) {
            movement.StopAirborneVerticalMovement(player);
            movement.StartAirborneActionHover(
                movement.GetAirWeakAttackPostHoverDurationSeconds());
        }
        StartIdle();
    }
}

void PlayerStateMachine::UpdateStrongAttacking(
    Player& player,
    PlayerInput& input,
    PlayerMovement& movement,
    PlayerCombat& combat,
    PlayerStatus& status,
    float deltaTime)
{
    const bool hasValidStrongTarget =
        mAttackDirectionTarget &&
        mAttackDirectionTarget->GetIsActive() &&
        mAttackDirectionTarget->IsAlive() &&
        !mAttackDirectionTarget->GetIsDead() &&
        !mAttackDirectionTarget->IsOnGround() &&
        mAttackDirectionTarget->GetCurrentPlanet() == player.GetCurrentPlanet();

    if (hasValidStrongTarget) {
        // アシストStrong中は、対象方向へ向きと突進方向を維持する。
        PlayerTargetingAssist::FaceTarget(
            player,
            movement,
            *mAttackDirectionTarget);

        movement.UpdateStrongAttackDirectionTowards(
            player,
            mAttackDirectionTarget->GetPos());
    } else {
        mAttackDirectionTarget = nullptr;

        if (combat.HasPendingAttackHit() &&
            !combat.GetIsAssistStrongAttack()) {
            movement.UpdateFacingDirectionFromInput(player, input);
        }
    }

    if (combat.GetStrongAttackTimer() >= 0.0f) {
        movement.ApplyStrongAttackMovement(player, combat, deltaTime);
        combat.ReduceStrongAttackTimer(deltaTime);
    }

    combat.UpdatePendingAttackHit(
        player,
        movement,
        status,
        deltaTime);

    if (combat.HasPendingAttackHit() ||
        combat.GetStrongAttackTimer() >= 0.0f) {
        return;
    }

    mAttackDirectionTarget = nullptr;
    movement.ClearStrongAttackDirectionOverride();
    player.SetShouldJudgeLanding(true);
    StartIdle();

    if (!combat.GetIsCharged()) {
        return;
    }

    if (combat.GetIsStrongAttackHit()) {
        combat.ClearStrongAttackHit();
        player.GetGame()->OnStrongAttacked(
            movement.GetPlayerNum());
    }
}

void PlayerStateMachine::UpdateAirSlamAttacking(
    Player& player,
    PlayerMovement& movement,
    PlayerCombat& combat,
    float deltaTime)
{
    const bool didReachGround =
        movement.UpdateAirSlamMovement(
            player,
            combat,
            deltaTime);
    if (!didReachGround) {
        return;
    }

    const bool didHitEnemy =
        combat.ResolveAirSlamImpact(
            player,
            movement,
            deltaTime);
    if (didHitEnemy) {
        player.GetGame()->OnStrongAttacked(
            movement.GetPlayerNum());
    }

    combat.ClearStrongAttackHit();
    mAttackDirectionTarget = nullptr;
    movement.ClearStrongAttackDirectionOverride();
    player.SetShouldJudgeLanding(true);
    StartIdle();
}


void PlayerStateMachine::UpdateKnockedBack(Player& player, PlayerMovement& movement, PlayerCombat& combat,
                                           PlayerStatus& status, float deltaTime)
{
    movement.ApplyKnockBackMovement(player, deltaTime);

    status.UpdateDamageTimer(deltaTime);
    if (status.GetDamageTimer() <= 0.0f) {
        StartIdle();
    }
}

void PlayerStateMachine::UpdateSpecialAttackCharging(Player& player, PlayerInput& input, PlayerMovement& movement,
                                                     PlayerCombat& combat, PlayerJewelGauge& jewelGauge,
                                                     float deltaTime)
{
    const float specialChargingTimerPrev = combat.GetSpecialChargingTimer();
    const float chargeDurationSeconds =
        combat.GetChargedAttackChargeDurationSeconds();
    const float firstJewelConsumptionTimeSeconds =
        chargeDurationSeconds * (2.0f / 3.0f);
    const float secondJewelConsumptionTimeSeconds =
        chargeDurationSeconds * (1.0f / 3.0f);

    combat.ReduceSpecialChargingTimer(deltaTime);

    const float specialChargingTimer =
        combat.GetSpecialChargingTimer();
    if (specialChargingTimerPrev >= firstJewelConsumptionTimeSeconds &&
        specialChargingTimer < firstJewelConsumptionTimeSeconds) {
        player.GetGame()->VibrateControllerForPlayer(movement.GetPlayerNum(), 10000, 0, 1000);
        jewelGauge.Consume(1);
        player.GetGame()->GetAudioSystem()->PlaySE("charging_se");
    }
    if (specialChargingTimerPrev >= secondJewelConsumptionTimeSeconds &&
        specialChargingTimer < secondJewelConsumptionTimeSeconds) {
        player.GetGame()->VibrateControllerForPlayer(movement.GetPlayerNum(), 20000, 0, 1000);
        jewelGauge.Consume(1);
        player.GetGame()->GetAudioSystem()->PlaySE("charging_se");
    }
    if (specialChargingTimerPrev >= 0.0f && specialChargingTimer < 0.0f) {
        player.GetGame()->VibrateControllerForPlayer(movement.GetPlayerNum(), 30000, 0, 1000);
        player.GetGame()->GetAudioSystem()->PlaySE("charged_se");
    }

    if (specialChargingTimer <= 0.0f) {
        combat.SetCanSpecialAttack(true);
    }

    if (specialChargingTimer <= 0.0f && input.GetAttackPressed() && !input.GetAttackPressedPrev()) {
        combat.SpecialAttack(player, movement, jewelGauge, deltaTime);



        player.RequestStrongAttackAnimation();
    }

    if (input.GetAttackPressed() && !input.GetAttackPressedPrev()) {
        combat.FinishSpecialAttackCharging();
    }
}

void PlayerStateMachine::UpdateContinuousAttacking(Player& player, PlayerMovement& movement, PlayerCombat& combat,
                                                    PlayerStatus& status, float deltaTime)
{
    const bool didAttack =
        combat.UpdateContinuousAttacking(player, movement, status, deltaTime);
    if (didAttack) {
        player.RequestNextWeakAttackAnimation();
    }
}
