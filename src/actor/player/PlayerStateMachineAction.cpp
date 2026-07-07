#include "actor/player/PlayerStateMachine.h"

#include "actor/Player.h"
#include "actor/player/PlayerCombat.h"
#include "actor/player/PlayerGrounding.h"
#include "actor/player/PlayerInput.h"
#include "actor/player/PlayerJewelGauge.h"
#include "actor/player/PlayerMovement.h"
#include "actor/player/PlayerStatus.h"
#include "system/AudioSystem.h"

void PlayerStateMachine::UpdateDodging(Player& player, PlayerMovement& movement, PlayerGrounding& grounding,
                                       PlayerCombat& combat, float deltaTime)
{
    movement.MoveDuringDodging(player, combat, grounding, deltaTime);

    movement.ReduceDodgeTimer(deltaTime);
    if (movement.GetDodgeTimer() <= 0.0f) {
        StartIdle();
    }
}

void PlayerStateMachine::UpdateAttacking(Player& player, PlayerInput& input, PlayerMovement& movement,
                                         PlayerCombat& combat, PlayerStatus& status, float deltaTime)
{
    if (player.GetOnGround()) {
        movement.MoveDuringAttacking(player, combat, deltaTime);
    }

    if (movement.CanWalk(combat)) {
        movement.UpdateWalk(player, input, deltaTime);
    }

    combat.ReduceAttackMotionTimer(deltaTime);
    if (combat.GetAttackMotionTimer() <= 0.0f) {
        StartIdle();
    }
}

void PlayerStateMachine::UpdateCharging(Player& player, PlayerInput& input, PlayerMovement& movement,
                                        PlayerCombat& combat, float deltaTime)
{
    const bool isAttackBtnReleased = !input.GetAttackPressed();
    if (isAttackBtnReleased) {
        ChangeState(PlayerActionState::StrongAttacking);
        combat.StartStrongAttacking(player, deltaTime);
        return;
    }

    if (combat.GetAttackPressTimer() < 0.0f) {
        return;
    }

    combat.ReduceAttackPressTimer(deltaTime);
    if (combat.GetAttackPressTimer() >= 0.0f) {
        movement.MoveDuringCharging(player, deltaTime);
        return;
    }

    combat.FinishCharging(player, movement);
}

void PlayerStateMachine::UpdateStrongAttacking(Player& player, PlayerMovement& movement, PlayerCombat& combat,
                                               PlayerStatus& status, float deltaTime)
{
    movement.MoveDuringStrongAttacking(player, combat, deltaTime);

    combat.ReduceStrongAttackTimer(deltaTime);
    if (combat.GetStrongAttackTimer() >= 0.0f) {
        return;
    }

    StartIdle();

    if (!combat.GetIsCharged()) {
        return;
    }

    if (!combat.GetIsStrongAttackHit()) {
        combat.Attack(player, movement, status, deltaTime);
    }

    if (combat.GetIsStrongAttackHit()) {
        combat.ClearStrongAttackHit();
        player.GetGame()->OnStrongAttacked(movement.GetPlayerNum());
    }
}

void PlayerStateMachine::UpdateKnockedBack(Player& player, PlayerMovement& movement, PlayerCombat& combat,
                                           PlayerStatus& status, float deltaTime)
{
    movement.MoveDuringKnockBack(player, deltaTime);

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
