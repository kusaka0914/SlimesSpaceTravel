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

void PlayerStateMachine::UpdateDodging(Player& player, PlayerMovement& movement, PlayerGrounding& grounding,
                                       PlayerCombat& combat, float deltaTime)
{
    movement.ApplyDodgeMovement(player, combat, grounding, deltaTime);

    movement.ReduceDodgeTimer(deltaTime);
    if (movement.GetDodgeTimer() <= 0.0f) {
        StartIdle();
    }
}

void PlayerStateMachine::UpdateAttacking(Player& player, PlayerInput& input, PlayerMovement& movement,
                                         PlayerCombat& combat, PlayerStatus& status, float deltaTime)
{
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
        StartIdle();
    }
}

void PlayerStateMachine::UpdateCharging(
    Player& player,
    PlayerInput& input,
    PlayerMovement& movement,
    PlayerCombat& combat,
    float deltaTime)
{
    const bool hasValidChargeTarget =
        mAttackDirectionTarget &&
        mAttackDirectionTarget->GetIsActive() &&
        mAttackDirectionTarget->IsAlive() &&
        !mAttackDirectionTarget->GetIsDead() &&
        !mAttackDirectionTarget->IsOnGround() &&
        mAttackDirectionTarget->GetCurrentPlanet() == player.GetCurrentPlanet();

    if (hasValidChargeTarget) {
        // 対象が移動しても、チャージ中は毎フレームその敵へ向き直す。
        PlayerTargetingAssist::FaceTarget(
            player,
            movement,
            *mAttackDirectionTarget);
    } else {
        mAttackDirectionTarget = nullptr;
    }

    const bool isAttackBtnReleased = !input.GetAttackPressed();
    if (isAttackBtnReleased) {
        if (mAttackDirectionTarget) {
            // 離した瞬間の敵の位置へStrongの突進方向を固定する。
            movement.StartStrongAttackMovementTowards(
                player,
                mAttackDirectionTarget->GetPos());
        } else {
            movement.ClearStrongAttackDirectionOverride();
        }

        ChangeState(PlayerActionState::StrongAttacking);
        combat.StartStrongAttacking(player, deltaTime);
        return;
    }

    if (combat.GetAttackPressTimer() < 0.0f) {
        return;
    }

    combat.ReduceAttackPressTimer(deltaTime);
    if (combat.GetAttackPressTimer() >= 0.0f) {
        // プレイヤーは敵を向いているため、既存の後退処理で敵と逆方向へ移動する。
        movement.ApplyChargeMovement(player, deltaTime);
        return;
    }

    combat.FinishCharging(player, movement);
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
        // 手動チャージとアシストStrongの両方で、対象方向へ向きと突進方向を維持する。
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

    combat.ReduceSpecialChargingTimer(deltaTime);

    if (specialChargingTimerPrev >= 2.0f && combat.GetSpecialChargingTimer() <= 2.0f) {
        player.GetGame()->VibrateControllerForPlayer(movement.GetPlayerNum(), 10000, 0, 1000);
        jewelGauge.Consume(1);
        player.GetGame()->GetAudioSystem()->PlaySE("charging_se");
    } else if (specialChargingTimerPrev >= 1.0f && combat.GetSpecialChargingTimer() <= 1.0f) {
        player.GetGame()->VibrateControllerForPlayer(movement.GetPlayerNum(), 20000, 0, 1000);
        jewelGauge.Consume(1);
        player.GetGame()->GetAudioSystem()->PlaySE("charging_se");
    } else if (specialChargingTimerPrev >= 0.0f && combat.GetSpecialChargingTimer() <= 0.0f) {
        player.GetGame()->VibrateControllerForPlayer(movement.GetPlayerNum(), 30000, 0, 1000);
        player.GetGame()->GetAudioSystem()->PlaySE("charged_se");
    }

    if (combat.GetSpecialChargingTimer() <= 0.0f) {
        combat.SetCanSpecialAttack(true);
    }

    if (combat.GetSpecialChargingTimer() <= 0.0f && input.GetAttackPressed() && !input.GetAttackPressedPrev()) {
        combat.SpecialAttack(player, movement, jewelGauge, deltaTime);
    }

    if (input.GetAttackPressed() && !input.GetAttackPressedPrev()) {
        combat.FinishSpecialAttackCharging();
    }
}

void PlayerStateMachine::UpdateContinuousAttacking(Player& player, PlayerMovement& movement, PlayerCombat& combat,
                                                   PlayerStatus& status, float deltaTime)
{
    combat.UpdateContinuousAttacking(player, movement, status, deltaTime);
}
