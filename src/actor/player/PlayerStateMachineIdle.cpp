#include "actor/player/PlayerStateMachine.h"

#include "actor/Enemy.h"
#include "actor/Player.h"
#include "actor/player/PlayerCombat.h"
#include "actor/player/PlayerInput.h"
#include "actor/player/PlayerJewelGauge.h"
#include "actor/player/PlayerMovement.h"
#include "actor/player/PlayerStatus.h"
#include "actor/player/PlayerTargetingAssist.h"
#include "system/AudioSystem.h"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

void PlayerStateMachine::UpdateIdle(Player& player, PlayerInput& input, PlayerMovement& movement, PlayerCombat& combat,
                                    PlayerJewelGauge& jewelGauge, PlayerStatus& status, float deltaTime)
{
    if (!player.GetIsActive()) {
        return;
    }

    if (TryStartAssistStrongAttack(player, input, movement, combat, deltaTime)) {
        return;
    }

    if (TryStartCharging(player, input, movement, combat)) {
        return;
    }

    ApplyIdleGravity(player, movement, combat, deltaTime);

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

bool PlayerStateMachine::TryStartAssistStrongAttack(Player& player, PlayerInput& input, PlayerMovement& movement,
                                                     PlayerCombat& combat, float deltaTime)
{
    if (!player.GetGame()->IsAssistControlStyle()) {
        return false;
    }

    if (input.GetSpecialAttackPressed() ||
        input.GetBufferedAttackInput() != PlayerAttackInputKind::Normal ||
        combat.GetAttackCooldownRemaining() > 0.0f || combat.IsSpecialCharging() || combat.GetCanSpecialAttack()) {
        return false;
    }

    constexpr float assistStrongTargetRangeMargin = 2.0f;
    const float targetRange = combat.GetStrongAttackRange() + assistStrongTargetRangeMargin;
    Enemy* target = PlayerTargetingAssist::FindAssistStrongTarget(
        player,
        targetRange,
        combat.GetNormalAttackAngle());
    if (!target) {
        return false;
    }

    mAttackDirectionTarget = target;
    PlayerTargetingAssist::FaceTarget(player, movement, *target);
    movement.StartStrongAttackMovementTowards(player, target->GetPos());

    input.ConsumeBufferedAttackInput();
    mCoyoteTimeRemaining = 0.0f;

    ChangeState(PlayerActionState::StrongAttacking);
    combat.StartAssistStrongAttacking(player, deltaTime);
    return true;
}

bool PlayerStateMachine::TryStartCharging(
    Player& player,
    PlayerInput& input,
    PlayerMovement& movement,
    PlayerCombat& combat)
{
    const bool hasNormalAttackRequest =
        input.GetBufferedAttackInput() == PlayerAttackInputKind::Normal;
    const bool canStartCharging =
        !player.GetOnGround() &&
        hasNormalAttackRequest &&
        input.GetAttackPressed() &&
        !input.GetSpecialAttackPressed() &&
        !combat.GetIsStrongAttacked();

    if (!canStartCharging) {
        return false;
    }

    // Strongが実際に届く距離にいる空中敵を対象にする。
    // 正面の攻撃範囲内を優先し、正面にいなければ全方向の最寄りを選ぶ。
    mAttackDirectionTarget = PlayerTargetingAssist::FindAttackTarget(
        player,
        combat.GetStrongAttackRange(),
        combat.GetNormalAttackAngle(),
        true);

    if (mAttackDirectionTarget) {
        PlayerTargetingAssist::FaceTarget(
            player,
            movement,
            *mAttackDirectionTarget);
    }

    input.ConsumeBufferedAttackInput();
    ChangeState(PlayerActionState::Charging);
    combat.StartCharging(player);
    return true;
}


void PlayerStateMachine::ApplyIdleGravity(
    Player& player,
    PlayerMovement& movement,
    PlayerCombat& combat,
    float deltaTime)
{
    if (!combat.IsAirAttackFloating()) {
        movement.ApplyJumpGravity(player, deltaTime);
    }
}

bool PlayerStateMachine::TryStartJumping(Player& player, PlayerInput& input, PlayerMovement& movement,
                                         PlayerCombat& combat, float deltaTime)
{
    const bool jumpStarted = input.GetJumpPressed() && !input.GetJumpPressedPrev();
    const bool hasGroundGrace = player.GetOnGround() || mCoyoteTimeRemaining > 0.0f;
    const bool canStartJumping = jumpStarted && hasGroundGrace;

    if (!canStartJumping || combat.IsSpecialCharging() || combat.GetCanSpecialAttack()) {
        return false;
    }

    mCoyoteTimeRemaining = 0.0f;
    movement.StartJumpMovement(player, deltaTime);
    player.NotifyJumpStarted();

    player.GetGame()->GetAudioSystem()->PlaySE("jump_se");
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
        movement.UpdateFacingDirectionFromInput(player, input);
    }

    if (combat.CanMoveDuringAttack() && !combat.IsSpecialCharging() && !combat.GetCanSpecialAttack()) {
        movement.MoveFromInput(player, input, deltaTime);
    }

    const bool isFalling = glm::dot(player.GetVelocity(), player.GetUpVec()) < 0.0f;
    if (isFalling) {
        player.SetShouldJudgeLanding(true);
    }
}

bool PlayerStateMachine::TryStartSpecialAttack(PlayerInput& input, PlayerCombat& combat,
                                               PlayerJewelGauge& jewelGauge)
{
    const bool canSpecialAttack = input.GetSpecialAttackPressed() && input.GetAttackPressed() &&
                                  !input.GetAttackPressedPrev() && jewelGauge.CanConsume(2);
    if (!canSpecialAttack) {
        return false;
    }

    input.ClearAttackBuffer();
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

    input.ClearAttackBuffer();
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
    const bool canStartDodging = movement.CanDodge(combat) && combat.CanDodgeDuringAttack() &&
                                 input.GetDodgePressed() && !input.GetDodgePressedPrev();

    if (!canStartDodging) {
        return false;
    }

    ChangeState(PlayerActionState::Dodging);

    movement.StartDodgeMovement(player, input);
    status.StartInvincible(movement.GetDodgeDuration());

    player.GetGame()->GetAudioSystem()->PlaySE("dodge_se");

    return true;
}

bool PlayerStateMachine::TryStartAttack(Player& player, PlayerInput& input, PlayerMovement& movement,
                                        PlayerCombat& combat, PlayerStatus& status, float deltaTime)
{
    const PlayerAttackInputKind attackInput = input.GetBufferedAttackInput();
    const bool hasAttackRequest = attackInput != PlayerAttackInputKind::None;
    const bool canStartAttacking = combat.GetAttackCooldownRemaining() <= 0.0f && hasAttackRequest;

    if (!canStartAttacking || combat.IsSpecialCharging() || combat.GetCanSpecialAttack()) {
        return false;
    }

    if (!player.GetOnGround() && attackInput != PlayerAttackInputKind::Wide) {
        return false;
    }

    const bool isNormalAttack = attackInput == PlayerAttackInputKind::Normal;
    const float attackRange =
        isNormalAttack ? combat.GetNormalAttackRange() : combat.GetWideAttackRange();
    const float attackAngle =
        isNormalAttack ? combat.GetNormalAttackAngle() : combat.GetWideAttackAngle();
    const bool requireAirborneTarget = !player.GetOnGround();

    // まず現在向いている攻撃範囲内の最寄りを選び、
    // そこに敵がいない場合だけ全方向の最寄りへ振り向く。
    mAttackDirectionTarget = PlayerTargetingAssist::FindAttackTarget(
        player, attackRange, attackAngle, requireAirborneTarget);

    if (mAttackDirectionTarget) {
        PlayerTargetingAssist::FaceTarget(player, movement, *mAttackDirectionTarget);
    }

    movement.ClearStrongAttackDirectionOverride();
    ChangeState(PlayerActionState::Attacking);
    combat.StartAttacking(player, attackInput, movement, status, deltaTime);
    input.ConsumeBufferedAttackInput();
    return true;
}
