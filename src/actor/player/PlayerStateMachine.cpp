#include "actor/player/PlayerStateMachine.h"

#include "actor/Player.h"
#include "actor/player/PlayerCombat.h"
#include "actor/player/PlayerInput.h"
#include "actor/player/PlayerMovement.h"
#include "actor/player/PlayerRespawn.h"
#include "actor/player/PlayerStatus.h"
#include "system/AudioSystem.h"
#include "system/SceneSystem.h"

#include <cmath>
#include <glm/glm.hpp>

void PlayerStateMachine::Update(Player& player, PlayerInput& input, PlayerMovement& movement, PlayerCombat& combat,
                                PlayerStatus& status, PlayerRespawn& respawn, float deltaTime)
{
    if (!player.GetGame()->GetSceneSystem()->IsPlaying()) {
        return;
    }

    const glm::vec3 prevPos = player.GetPos();

    if (status.IsAlive()) {
        UpdateAlive(player, input, movement, combat, status, respawn, deltaTime);
        respawn.CheckFallRespawn(player, combat, status, prevPos);
    } else {
        status.Die(*player.GetGame());
    }
}

void PlayerStateMachine::UpdateAlive(Player& player, PlayerInput& input, PlayerMovement& movement, PlayerCombat& combat,
                                     PlayerStatus& status, PlayerRespawn& respawn, float deltaTime)
{
    movement.UpdateWorldVec(player, input);
    movement.UpdateBoatRide(player, respawn);

    if (combat.GetJewelCount() < 2 && combat.GetJewelTimer() <= 0.0f) {
        StartJewelTimer(combat);
    }

    switch (combat.GetActionState()) {
    case PlayerActionState::Idle:
        UpdateIdle(player, input, movement, combat, status, deltaTime);
        break;
    case PlayerActionState::Dodging:
        UpdateDodging(player, movement, combat, deltaTime);
        break;
    case PlayerActionState::Attacking:
        UpdateAttacking(player, input, movement, combat, status, deltaTime);
        break;
    case PlayerActionState::Charging:
        UpdateCharging(player, input, movement, combat, deltaTime);
        break;
    case PlayerActionState::StrongAttacking:
        UpdateStrongAttacking(player, movement, combat, status, deltaTime);
        break;
    case PlayerActionState::KnockedBack:
        UpdateKnockedBack(player, movement, combat, status, deltaTime);
        break;
    }

    UpdateTimer(input, movement, combat, status, deltaTime);
    input.EndFrame();
}

void PlayerStateMachine::UpdateIdle(Player& player, PlayerInput& input, PlayerMovement& movement, PlayerCombat& combat,
                                    PlayerStatus& status, float deltaTime)
{
    if (!player.GetIsActive()) {
        return;
    }

    const bool canStartCharging = !player.GetOnGround() && input.GetAttackPressed() && !combat.GetIsStrongAttacked();
    if (canStartCharging) {
        combat.StartCharging(player);
        return;
    }

    if (!combat.IsAirAttackFloating()) {
        player.ApplyGravityToSelf(deltaTime);
    }

    const bool canStartJumping = input.GetJumpPressed() && player.GetOnGround();
    if (canStartJumping && !combat.IsSpecialCharging() && !combat.GetCanSpecialAttack()) {
        movement.StartJumping(player, deltaTime);
        return;
    }

    const bool canRecover = input.GetRecoverPressed() && !input.GetRecoverPressedPrev() && combat.GetJewelCount() > 0 &&
                            status.GetHp() != status.GetMaxHp();

    if (canRecover) {
        combat.SetJewelCount(combat.GetJewelCount() - 1);
        status.Heal(1.0f);
        player.GetGame()->GetAudioSystem()->PlaySE("recover_se");
    }

    const bool isMoving = std::abs(input.GetMoveForward()) > 0.01f || std::abs(input.GetMoveLeft()) > 0.01f;
    if (isMoving && !status.IsTired()) {
        movement.ChangeFaceDir(player, input);
    }

    if (movement.CanWalk(combat) && !combat.IsSpecialCharging() && !combat.GetCanSpecialAttack()) {
        movement.UpdateWalk(player, input, deltaTime);
    }

    const bool isFalling = glm::dot(player.GetVelocity(), player.GetUpVec()) < 0.0f;
    if (isFalling) {
        player.SetShouldJudgeLanding(true);
    }

    const bool canSpecialAttack = input.GetSpecialAttackPressed() && input.GetAttackPressed() &&
                                  !input.GetAttackPressedPrev() && combat.GetJewelCount() >= 2;

    if (canSpecialAttack) {
        combat.StartSpecialAttackCharging();
        return;
    }

    if (input.GetWideAttackPressed() && !input.GetWideAttackPressedPrev() && status.IsTired()) {
        ReduceTired(status, movement, combat);
        return;
    }

    const bool canContinuousAttacking = input.GetSpecialAttackPressed() && input.GetWideAttackPressed() &&
                                        !input.GetWideAttackPressedPrev() && combat.GetJewelCount() >= 1;

    if (canContinuousAttacking) {
        combat.StartContinuousAttacking();
        return;
    }

    if (combat.IsSpecialCharging() || combat.GetCanSpecialAttack()) {
        UpdateSpecialAttackCharging(player, input, movement, combat, deltaTime);
    }

    if (combat.IsContinuousAttacking()) {
        UpdateContinuousAttacking(player, movement, combat, status, deltaTime);
        return;
    }

    const bool canStartDodging = movement.CanDodge(combat) && input.GetDodgePressed() && !input.GetDodgePressedPrev();

    if (canStartDodging) {
        movement.StartDodging(player, input, combat, status);
        return;
    }

    const bool canStartAttacking = combat.GetAttackCooldownRemaining() <= 0.0f &&
                                   ((input.GetAttackPressed() || input.GetWideAttackPressed()) &&
                                    !input.GetAttackPressedPrev() && !input.GetWideAttackPressedPrev());

    if (canStartAttacking && !combat.IsSpecialCharging() && !combat.GetCanSpecialAttack()) {
        combat.StartAttacking(player, input, movement, status, deltaTime);
        return;
    }
}

void PlayerStateMachine::UpdateDodging(Player& player, PlayerMovement& movement, PlayerCombat& combat, float deltaTime)
{
    movement.MoveDuringDodging(player, combat, deltaTime);

    movement.ReduceDodgeTimer(deltaTime);
    if (movement.GetDodgeTimer() <= 0.0f) {
        StartIdle(combat);
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
        StartIdle(combat);
    }
}

void PlayerStateMachine::UpdateCharging(Player& player, PlayerInput& input, PlayerMovement& movement,
                                        PlayerCombat& combat, float deltaTime)
{
    const bool isAttackBtnReleased = !input.GetAttackPressed();
    if (isAttackBtnReleased) {
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

    StartIdle(combat);

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
        StartIdle(combat);
    }
}

void PlayerStateMachine::UpdateSpecialAttackCharging(Player& player, PlayerInput& input, PlayerMovement& movement,
                                                     PlayerCombat& combat, float deltaTime)
{
    const float specialChargingTimerPrev = combat.GetSpecialChargingTimer();

    combat.ReduceSpecialChargingTimer(deltaTime);

    if (specialChargingTimerPrev >= 2.0f && combat.GetSpecialChargingTimer() <= 2.0f) {
        player.GetGame()->VibrateControllerForPlayer(movement.GetPlayerNum(), 10000, 0, 1000);
        combat.SetJewelCount(combat.GetJewelCount() - 1);
        player.GetGame()->GetAudioSystem()->PlaySE("charging_se");
    } else if (specialChargingTimerPrev >= 1.0f && combat.GetSpecialChargingTimer() <= 1.0f) {
        player.GetGame()->VibrateControllerForPlayer(movement.GetPlayerNum(), 20000, 0, 1000);
        combat.SetJewelCount(combat.GetJewelCount() - 1);
        player.GetGame()->GetAudioSystem()->PlaySE("charging_se");
    } else if (specialChargingTimerPrev >= 0.0f && combat.GetSpecialChargingTimer() <= 0.0f) {
        player.GetGame()->VibrateControllerForPlayer(movement.GetPlayerNum(), 30000, 0, 1000);
        player.GetGame()->GetAudioSystem()->PlaySE("charged_se");
    }

    if (combat.GetSpecialChargingTimer() <= 0.0f) {
        combat.SetCanSpecialAttack(true);
    }

    if (combat.GetSpecialChargingTimer() <= 0.0f && input.GetAttackPressed() && !input.GetAttackPressedPrev()) {
        combat.SpecialAttack(player, movement, deltaTime);
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

void PlayerStateMachine::UpdateTimer(PlayerInput& input, PlayerMovement& movement, PlayerCombat& combat,
                                     PlayerStatus& status, float deltaTime)
{
    combat.UpdateAirAttackFloatingTimer(deltaTime);
    movement.ReduceDodgeCooldown(deltaTime);

    if (combat.GetJewelTimer() >= 0.0f) {
        combat.UpdateJewelTimer(deltaTime);
    }

    combat.UpdateAttackCooldown(deltaTime);
    combat.UpdateAttackMoveLock(status, deltaTime);
    combat.UpdateAttackDodgeLock(deltaTime);
    status.UpdateInvincibleTimer(deltaTime);
    combat.UpdateRayCastTimer(deltaTime);
    input.UpdateInputAvailableTimer(deltaTime);

    if (combat.GetComboKeepTimer() > 0.0f) {
        combat.UpdateComboKeepTimer(deltaTime);
    }
}

void PlayerStateMachine::StartIdle(PlayerCombat& combat)
{
    combat.StartIdle();
}

void PlayerStateMachine::StartJewelTimer(PlayerCombat& combat)
{
    combat.SetJewelTimer(30.0f);
}

void PlayerStateMachine::StartTired(PlayerStatus& status, PlayerMovement& movement, PlayerCombat& combat,
                                    float lockTime)
{
    combat.StartTiredLock(status, movement, lockTime);
}

void PlayerStateMachine::ReduceTired(PlayerStatus& status, PlayerMovement& movement, PlayerCombat& combat)
{
    constexpr float reduceTime = 0.8f;
    combat.ReduceTiredLock(status, movement, reduceTime);
}