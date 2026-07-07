#include "actor/player/PlayerStateMachine.h"

#include "actor/Player.h"
#include "actor/player/PlayerCombat.h"
#include "actor/player/PlayerInput.h"
#include "actor/player/PlayerJewelGauge.h"
#include "actor/player/PlayerMovement.h"
#include "actor/player/PlayerStatus.h"
#include "system/AudioSystem.h"

#include <cmath>
#include <glm/glm.hpp>

void PlayerStateMachine::UpdateIdle(Player& player, PlayerInput& input, PlayerMovement& movement, PlayerCombat& combat,
                                    PlayerJewelGauge& jewelGauge, PlayerStatus& status, float deltaTime)
{
    if (!player.GetIsActive()) {
        return;
    }

    if (TryStartCharging(player, input, combat)) {
        return;
    }

    ApplyIdleGravity(player, combat, deltaTime);

    if (TryStartJumping(player, input, movement, combat, deltaTime)) {
        return;
    }

    if (TryRecover(player, input, jewelGauge, status)) {
        return;
    }

    UpdateIdleMovement(player, input, movement, combat, status, deltaTime);

    if (TryStartSpecialAttack(input, combat, jewelGauge)) {
        return;
    }

    if (TryReduceTired(input, movement, combat, status)) {
        return;
    }

    if (TryStartContinuousAttack(input, combat, jewelGauge)) {
        return;
    }

    UpdateSpecialAttackIfNeeded(player, input, movement, combat, jewelGauge, deltaTime);

    if (TryUpdateContinuousAttack(player, movement, combat, status, deltaTime)) {
        return;
    }

    if (TryStartDodging(player, input, movement, combat, status)) {
        return;
    }

    TryStartAttack(player, input, movement, combat, status, deltaTime);
}

bool PlayerStateMachine::TryStartCharging(Player& player, PlayerInput& input, PlayerCombat& combat)
{
    const bool canStartCharging = !player.GetOnGround() && input.GetAttackPressed() && !combat.GetIsStrongAttacked();
    if (!canStartCharging) {
        return false;
    }

    ChangeState(PlayerActionState::Charging);
    combat.StartCharging(player);
    return true;
}

void PlayerStateMachine::ApplyIdleGravity(Player& player, PlayerCombat& combat, float deltaTime)
{
    if (!combat.IsAirAttackFloating()) {
        player.ApplyGravityToSelf(deltaTime);
    }
}

bool PlayerStateMachine::TryStartJumping(Player& player, PlayerInput& input, PlayerMovement& movement,
                                         PlayerCombat& combat, float deltaTime)
{
    const bool canStartJumping = input.GetJumpPressed() && player.GetOnGround();
    if (!canStartJumping || combat.IsSpecialCharging() || combat.GetCanSpecialAttack()) {
        return false;
    }

    movement.StartJumping(player, deltaTime);
    return true;
}

bool PlayerStateMachine::TryRecover(Player& player, PlayerInput& input, PlayerJewelGauge& jewelGauge,
                                    PlayerStatus& status)
{
    const bool canRecover = input.GetRecoverPressed() && !input.GetRecoverPressedPrev() && jewelGauge.CanConsume(1) &&
                            status.GetHp() != status.GetMaxHp();
    if (!canRecover) {
        return false;
    }

    jewelGauge.Consume(1);
    status.Heal(1.0f);
    player.GetGame()->GetAudioSystem()->PlaySE("recover_se");
    return true;
}

void PlayerStateMachine::UpdateIdleMovement(Player& player, PlayerInput& input, PlayerMovement& movement,
                                            PlayerCombat& combat, PlayerStatus& status, float deltaTime)
{
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
}

bool PlayerStateMachine::TryStartSpecialAttack(PlayerInput& input, PlayerCombat& combat, PlayerJewelGauge& jewelGauge)
{
    const bool canSpecialAttack = input.GetSpecialAttackPressed() && input.GetAttackPressed() &&
                                  !input.GetAttackPressedPrev() && jewelGauge.CanConsume(2);
    if (!canSpecialAttack) {
        return false;
    }

    combat.StartSpecialAttackCharging();
    return true;
}

bool PlayerStateMachine::TryReduceTired(PlayerInput& input, PlayerMovement& movement, PlayerCombat& combat,
                                        PlayerStatus& status)
{
    if (!(input.GetWideAttackPressed() && !input.GetWideAttackPressedPrev() && status.IsTired())) {
        return false;
    }

    ReduceTired(status, movement, combat);
    return true;
}

bool PlayerStateMachine::TryStartContinuousAttack(PlayerInput& input, PlayerCombat& combat,
                                                  PlayerJewelGauge& jewelGauge)
{
    const bool canContinuousAttacking = input.GetSpecialAttackPressed() && input.GetWideAttackPressed() &&
                                        !input.GetWideAttackPressedPrev() && jewelGauge.CanConsume(1);
    if (!canContinuousAttacking) {
        return false;
    }

    jewelGauge.Consume(1);
    combat.StartContinuousAttacking();
    return true;
}

void PlayerStateMachine::UpdateSpecialAttackIfNeeded(Player& player, PlayerInput& input, PlayerMovement& movement,
                                                     PlayerCombat& combat, PlayerJewelGauge& jewelGauge,
                                                     float deltaTime)
{
    if (combat.IsSpecialCharging() || combat.GetCanSpecialAttack()) {
        UpdateSpecialAttackCharging(player, input, movement, combat, jewelGauge, deltaTime);
    }
}

bool PlayerStateMachine::TryUpdateContinuousAttack(Player& player, PlayerMovement& movement, PlayerCombat& combat,
                                                   PlayerStatus& status, float deltaTime)
{
    if (!combat.IsContinuousAttacking()) {
        return false;
    }

    UpdateContinuousAttacking(player, movement, combat, status, deltaTime);
    return true;
}

bool PlayerStateMachine::TryStartDodging(Player& player, PlayerInput& input, PlayerMovement& movement,
                                         PlayerCombat& combat, PlayerStatus& status)
{
    const bool canStartDodging = movement.CanDodge(combat) && input.GetDodgePressed() && !input.GetDodgePressedPrev();
    if (!canStartDodging) {
        return false;
    }

    ChangeState(PlayerActionState::Dodging);
    movement.StartDodging(player, input, status);
    return true;
}

bool PlayerStateMachine::TryStartAttack(Player& player, PlayerInput& input, PlayerMovement& movement,
                                        PlayerCombat& combat, PlayerStatus& status, float deltaTime)
{
    const bool canStartAttacking = combat.GetAttackCooldownRemaining() <= 0.0f &&
                                   ((input.GetAttackPressed() || input.GetWideAttackPressed()) &&
                                    !input.GetAttackPressedPrev() && !input.GetWideAttackPressedPrev());

    if (!canStartAttacking || combat.IsSpecialCharging() || combat.GetCanSpecialAttack()) {
        return false;
    }

    if (!player.GetOnGround() && !input.GetWideAttackPressed()) {
        return false;
    }

    ChangeState(PlayerActionState::Attacking);
    combat.StartAttacking(player, input, movement, status, deltaTime);
    return true;
}
