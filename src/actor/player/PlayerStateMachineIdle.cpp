#include "actor/player/PlayerStateMachine.h"

#include "actor/Enemy.h"
#include "actor/enemy/EnemyCollisionGeometry.h"
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

    if (player.IsAttachedToPlatform()) {
        // Adhesive platforms allow only the explicit jump escape. Skipping
        // the rest of the idle actions prevents assist attacks or dodges from
        // moving the player away from the first contact point.
        TryStartJumping(player, input, movement, combat, deltaTime);
        return;
    }

    if (TryStartAssistAirSlamAttack(
            player,
            input,
            movement,
            combat)) {
        return;
    }

    if (TryStartAirSlamAttack(
            player,
            input,
            movement,
            combat,
            deltaTime)) {
        return;
    }

    if (TryStartAssistStrongAttack(player, input, movement, combat, deltaTime)) {
        return;
    }

    if (TryStartAssistBrokenEnemyAirCombo(
            player,
            input,
            movement,
            combat,
            status,
            deltaTime)) {
        return;
    }

    const bool wasInputMovementApplied = ApplyIdleGravity(
        player,
        input,
        movement,
        combat,
        deltaTime);

    if (TryRecover(player, input, jewelGauge, status)) {
        return;
    }

    if (TryStartJumping(player, input, movement, combat, deltaTime)) {
        return;
    }

    UpdateIdleMovement(
        player,
        input,
        movement,
        combat,
        status,
        wasInputMovementApplied,
        deltaTime);

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

    if (TryStartAssistAirDodgeAttack(
            player,
            input,
            movement,
            combat,
            status)) {
        return;
    }

    TryStartAttack(player, input, movement, combat, status, deltaTime);
}

bool PlayerStateMachine::TryStartAssistBrokenEnemyAirCombo(
    Player& player,
    PlayerInput& input,
    PlayerMovement& movement,
    PlayerCombat& combat,
    PlayerStatus& status,
    float deltaTime)
{
    const bool hasWeakAttackRequest =
        input.GetBufferedAttackInput() == PlayerAttackInputKind::Wide;
    const bool canStartAirCombo =
        player.GetGame()->IsAssistControlStyle() &&
        player.GetOnGround() &&
        hasWeakAttackRequest &&
        combat.GetAttackCooldownRemaining() <= 0.0f &&
        !combat.IsSpecialCharging() &&
        !combat.GetCanSpecialAttack();
    if (!canStartAirCombo) {
        return false;
    }

    constexpr float assistAirComboMaximumTargetDistance = 8.0f;
    Enemy* target =
        PlayerTargetingAssist::FindNearestBrokenAirborneTarget(
            player,
            assistAirComboMaximumTargetDistance);
    if (!target) {
        return false;
    }

    glm::vec3 targetUpDirection = target->GetUpVec();
    const float targetUpLength = glm::length(targetUpDirection);
    if (targetUpLength > 0.0001f) {
        targetUpDirection /= targetUpLength;
        player.SetUpVec(targetUpDirection);
    }

    glm::vec3 entryDirection = player.GetPos() - target->GetPos();
    entryDirection -= targetUpDirection * glm::dot(entryDirection, targetUpDirection);
    const float entryDirectionLength = glm::length(entryDirection);
    if (entryDirectionLength > 0.0001f) {
        entryDirection /= entryDirectionLength;
    } else {
        entryDirection = -target->GetFacingForwardVec();
    }

    EnemyCollisionGeometry::ModelBounds targetBounds;
    const bool hasTargetModelBounds =
        EnemyCollisionGeometry::TryCreateModelBounds(
            *target,
            targetBounds);
    const float targetSurfaceDistance =
        hasTargetModelBounds
            ? EnemyCollisionGeometry::CalculateSupportDistance(
                  targetBounds,
                  entryDirection)
            : std::max(0.0f, target->GetRadius());
    const glm::vec3 targetCollisionCenter =
        hasTargetModelBounds
            ? targetBounds.center
            : target->GetPos();
    constexpr float playerEntryClearance = 0.8f;
    player.SetPos(
        targetCollisionCenter +
        entryDirection *
            (targetSurfaceDistance +
             playerEntryClearance));
    player.SetVelocity(glm::vec3(0.0f));
    player.SetOnGround(false);
    player.SetShouldJudgeLanding(false);
    player.RefreshFallbackUpVec();

    movement.CancelJumpApexHover();
    movement.CancelAirborneActionHover();
    movement.ResetEllipseAirborneSurfaceTravel();
    combat.PrepareAssistAirCombo();

    mAttackDirectionTarget = target;
    PlayerTargetingAssist::FaceTarget(player, movement, *target);
    movement.ClearStrongAttackDirectionOverride();
    ChangeState(PlayerActionState::Attacking);
    combat.StartAttacking(
        player,
        PlayerAttackInputKind::Wide,
        movement,
        status,
        deltaTime);
    input.ConsumeBufferedAttackInput();
    return true;
}

bool PlayerStateMachine::TryStartAssistAirSlamAttack(
    Player& player,
    PlayerInput& input,
    PlayerMovement& movement,
    PlayerCombat& combat)
{
    const bool hasWeakAttackRequest =
        input.GetBufferedAttackInput() == PlayerAttackInputKind::Wide;
    const bool canStartAirSlam =
        player.GetGame()->IsAssistControlStyle() &&
        !player.GetOnGround() &&
        hasWeakAttackRequest &&
        combat.HasSuccessfulAirDodgeAttack() &&
        combat.GetAttackCooldownRemaining() <= 0.0f &&
        !combat.GetIsStrongAttacked();
    if (!canStartAirSlam) {
        return false;
    }

    constexpr float assistAirSlamMaximumTargetDistance = 8.0f;
    constexpr float assistAirSlamLaunchedTimerThresholdSeconds = 0.5f;
    Enemy* target =
        PlayerTargetingAssist::FindNearestBrokenAirborneTargetNearRecovery(
            player,
            assistAirSlamMaximumTargetDistance,
            assistAirSlamLaunchedTimerThresholdSeconds);
    if (!target) {
        return false;
    }

    mAttackDirectionTarget = target;
    PlayerTargetingAssist::FaceTarget(player, movement, *target);
    movement.ClearStrongAttackDirectionOverride();
    movement.StartAirSlamMovement(player);
    combat.StartAirSlamAttack();
    input.ConsumeBufferedAttackInput();
    mCoyoteTimeRemaining = 0.0f;
    ChangeState(PlayerActionState::AirSlamAttacking);
    return true;
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

bool PlayerStateMachine::TryStartAirSlamAttack(
    Player& player,
    PlayerInput& input,
    PlayerMovement& movement,
    PlayerCombat& combat,
    float deltaTime)
{
    (void)deltaTime;

    const bool hasNormalAttackRequest =
        input.GetBufferedAttackInput() == PlayerAttackInputKind::Normal;
    const bool canStartAirSlam =
        !player.GetOnGround() &&
        hasNormalAttackRequest &&
        !input.GetSpecialAttackPressed() &&
        combat.GetAttackCooldownRemaining() <= 0.0f &&
        !combat.GetIsStrongAttacked();

    if (!canStartAirSlam) {
        return false;
    }

    mAttackDirectionTarget = nullptr;
    movement.ClearStrongAttackDirectionOverride();
    movement.StartAirSlamMovement(player);
    combat.StartAirSlamAttack();
    input.ConsumeBufferedAttackInput();
    mCoyoteTimeRemaining = 0.0f;
    ChangeState(
        PlayerActionState::AirSlamAttacking);
    return true;
}


bool PlayerStateMachine::ApplyIdleGravity(
    Player& player,
    PlayerInput& input,
    PlayerMovement& movement,
    PlayerCombat& combat,
    float deltaTime)
{
    if (movement.UpdateAirborneActionHover(
            player,
            deltaTime)) {
        return false;
    }

    const bool canApplyInputMovement =
        !player.GetOnGround() &&
        combat.CanMoveDuringAttack() &&
        !combat.IsSpecialCharging() &&
        !combat.GetCanSpecialAttack();
    if (canApplyInputMovement) {
        movement.ApplyJumpGravityAndInputMovement(
            player,
            input,
            deltaTime);
        return true;
    }

    movement.ApplyJumpGravity(player, deltaTime);
    return false;
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
                                            PlayerCombat& combat, PlayerStatus& status,
                                            bool wasInputMovementApplied, float deltaTime)
{
    if (player.IsAttachedToPlatform()) {
        return;
    }

    const bool isMoving = std::abs(input.GetMoveForward()) > 0.01f || std::abs(input.GetMoveLeft()) > 0.01f;
    if (isMoving && !status.IsTired()) {
        movement.UpdateFacingDirectionFromInput(player, input);
    }

    const bool canApplyInputMovement =
        combat.CanMoveDuringAttack() &&
        !combat.IsSpecialCharging() &&
        !combat.GetCanSpecialAttack();
    if (canApplyInputMovement &&
        !wasInputMovementApplied) {
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

    const bool startsInAir = !player.GetOnGround();
    movement.StartDodgeMovement(player, input);
    if (startsInAir) {
        combat.StartAirDodgeAttack();
    } else {
        combat.EndAirDodgeAttack();
    }
    status.StartDodgeInvincibility(
        movement.GetDodgeDuration());

    player.GetGame()->GetAudioSystem()->PlaySE("dodge_se");

    return true;
}

bool PlayerStateMachine::TryStartAssistAirDodgeAttack(
    Player& player,
    PlayerInput& input,
    PlayerMovement& movement,
    PlayerCombat& combat,
    PlayerStatus& status)
{
    const bool hasWideAttackRequest =
        input.GetBufferedAttackInput() ==
        PlayerAttackInputKind::Wide;
    const bool canStartAutoDodge =
        player.GetGame()->IsAssistControlStyle() &&
        !player.GetOnGround() &&
        hasWideAttackRequest &&
        !combat.CanStartAirAttack() &&
        combat.GetAttackCooldownRemaining() <= 0.0f &&
        movement.CanDodge(combat) &&
        combat.CanDodgeDuringAttack() &&
        !combat.IsSpecialCharging() &&
        !combat.GetCanSpecialAttack();
    if (!canStartAutoDodge) {
        return false;
    }

    constexpr float targetContactMargin = 1.0f;
    const float maximumTargetDistance =
        movement.GetDodgeDistance() +
        targetContactMargin;
    Enemy* target =
        PlayerTargetingAssist::FindNearestAirborneTarget(
            player,
            maximumTargetDistance);
    if (!target ||
        !movement.StartDodgeMovementTowards(
            player,
            target->GetPos())) {
        return false;
    }

    PlayerTargetingAssist::FaceTarget(
        player,
        movement,
        *target);
    input.ConsumeBufferedAttackInput();
    mAttackDirectionTarget = nullptr;

    ChangeState(PlayerActionState::Dodging);
    combat.StartAirDodgeAttack();
    status.StartDodgeInvincibility(
        movement.GetDodgeDuration());
    player.GetGame()
        ->GetAudioSystem()
        ->PlaySE("dodge_se");
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

    if (!player.GetOnGround() &&
        attackInput == PlayerAttackInputKind::Wide &&
        !combat.CanStartAirAttack()) {
        input.ConsumeBufferedAttackInput();
        return false;
    }

    const bool isNormalAttack = attackInput == PlayerAttackInputKind::Normal;
    const float attackRange =
        isNormalAttack ? combat.GetNormalAttackRange() : combat.GetWideAttackRange();
    const float attackAngle =
        isNormalAttack ? combat.GetNormalAttackAngle() : combat.GetWideAttackAngle();
    if (player.GetGame()->IsAssistControlStyle()) {
        const bool requireAirborneTarget = !player.GetOnGround();

        // 敵への自動方向転換はアシスト操作時だけ行う。
        mAttackDirectionTarget = PlayerTargetingAssist::FindAttackTarget(
            player,
            attackRange,
            attackAngle,
            requireAirborneTarget);

        if (mAttackDirectionTarget) {
            PlayerTargetingAssist::FaceTarget(
                player,
                movement,
                *mAttackDirectionTarget);
        }
    } else {
        mAttackDirectionTarget = nullptr;
    }

    movement.ClearStrongAttackDirectionOverride();
    ChangeState(PlayerActionState::Attacking);
    combat.StartAttacking(player, attackInput, movement, status, deltaTime);
    input.ConsumeBufferedAttackInput();
    return true;
}
