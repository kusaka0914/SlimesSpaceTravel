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
#include "app/Game.h"
#include "system/AudioSystem.h"

#include <algorithm>

void PlayerStateMachine::UpdateDodging(Player& player, PlayerInput& input, PlayerMovement& movement,
                                       PlayerGrounding& grounding, PlayerCombat& combat, PlayerStatus& status,
                                       float deltaTime)
{
    const glm::vec3 movementStart = player.GetPos();
    movement.ApplyDodgeMovement(player, combat, grounding, deltaTime);
    combat.UpdateAirDodgeAttack(player, movement, movementStart, player.GetPos(), deltaTime);

    movement.ReduceDodgeTimer(deltaTime);
    if (movement.GetDodgeTimer() <= 0.0f) {
        status.StartDodgeCounterGrace();
        const bool didFinishAirDodge = combat.IsAirDodgeAttackActive() && !player.GetOnGround();
        const bool shouldResumeAirMovementImmediately = didFinishAirDodge && mShouldSkipAirDodgePostHover;
        combat.EndAirDodgeAttack();
        if (didFinishAirDodge && !shouldResumeAirMovementImmediately) {
            movement.StopAirborneVerticalMovement(player);
            movement.StartAirborneActionHover(movement.GetAirDodgePostHoverDurationSeconds());
        }
        if (shouldResumeAirMovementImmediately) {
            // 攻撃からダッシュしたケースだけは、ダッシュ終了フレームから
            // 空中入力と落下を再開する。これで着地まで停止しない。
            combat.CancelCurrentAttack();
            movement.CancelAirborneActionHover();
            movement.ApplyJumpGravityAndInputMovement(player, input, deltaTime);
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

    if (player.GetGame()->IsAssistControlStyle() && TryStartAirSlamAttack(player, input, movement, combat)) {
        return;
    }

    const bool wasAirWeakAttacking = !player.GetOnGround();
    if (TryStartDodging(player, input, movement, combat, status)) {
        if (wasAirWeakAttacking) {
            combat.CancelAirAttackForDodge();
        } else {
            combat.CancelCurrentAttack();
        }

        // 空中攻撃をダッシュでキャンセルした場合は、残りの硬直を持ち越さない。
        mShouldSkipAirDodgePostHover = wasAirWeakAttacking;
        mAllowsAirMovementAfterDodge = wasAirWeakAttacking;
        mAttackDirectionTarget = nullptr;
        return;
    }

    if (TryStartAssistAirDodgeAttack(player, input, movement, combat, status)) {
        return;
    }

    if (combat.HasPendingAttackHit()) {
        const bool targetMatchesPlayerAttackHeight =
            player.GetOnGround() || (mAttackDirectionTarget && mAttackDirectionTarget->IsLaunched());
        const bool hasValidDirectionTarget = mAttackDirectionTarget && mAttackDirectionTarget->GetIsActive() &&
                                             mAttackDirectionTarget->IsAlive() &&
                                             !mAttackDirectionTarget->GetIsDead() && targetMatchesPlayerAttackHeight &&
                                             mAttackDirectionTarget->GetCurrentPlanet() == player.GetCurrentPlanet();

        if (hasValidDirectionTarget) {
            // 攻撃開始時に選んだ最寄りの敵へ、判定が出る瞬間まで向きを維持する。
            PlayerTargetingAssist::FaceTarget(player, movement, *mAttackDirectionTarget);
        } else {
            mAttackDirectionTarget = nullptr;
            movement.UpdateFacingDirectionFromInput(player, input);
        }

        const bool didResolveAttack = combat.UpdatePendingAttackHit(player, movement, status, deltaTime);
        if (didResolveAttack && !combat.HasPendingAttackHit()) {
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

void PlayerStateMachine::UpdateStrongAttacking(Player& player, PlayerInput& input, PlayerMovement& movement,
                                               PlayerCombat& combat, PlayerStatus& status, float deltaTime)
{
    const bool hasValidStrongTarget = mAttackDirectionTarget && mAttackDirectionTarget->GetIsActive() &&
                                      mAttackDirectionTarget->IsAlive() && !mAttackDirectionTarget->GetIsDead() &&
                                      mAttackDirectionTarget->IsLaunched() &&
                                      mAttackDirectionTarget->GetCurrentPlanet() == player.GetCurrentPlanet();

    if (hasValidStrongTarget) {
        // アシストStrong中は、対象方向へ向きと突進方向を維持する。
        PlayerTargetingAssist::FaceTarget(player, movement, *mAttackDirectionTarget);

        movement.UpdateStrongAttackDirectionTowards(player, mAttackDirectionTarget->GetPos());
    } else {
        mAttackDirectionTarget = nullptr;

        if (combat.HasPendingAttackHit() && !combat.GetIsAssistStrongAttack()) {
            movement.UpdateFacingDirectionFromInput(player, input);
        }
    }

    if (combat.GetStrongAttackTimer() >= 0.0f) {
        movement.ApplyStrongAttackMovement(player, combat, deltaTime);
        combat.ReduceStrongAttackTimer(deltaTime);
    }

    combat.UpdatePendingAttackHit(player, movement, status, deltaTime);

    if (combat.HasPendingAttackHit() || combat.GetStrongAttackTimer() >= 0.0f) {
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
        player.GetGame()->OnStrongAttacked(movement.GetPlayerNum());
    }
}

void PlayerStateMachine::UpdateAirSlamAttacking(Player& player, PlayerMovement& movement, PlayerCombat& combat,
                                                float deltaTime)
{
    const bool didReachGround = movement.UpdateAirSlamMovement(player, combat, deltaTime);
    if (movement.IsAirSlamFalling()) {
        const bool didStartEnemyGravitySlam = combat.UpdateAirSlamContact(player, movement);
        if (didStartEnemyGravitySlam) {
            movement.StartAirSlamEnemyFallWatch(player);
        }
    }
    if (!didReachGround) {
        return;
    }

    const bool didHitEnemy = combat.ResolveAirSlamImpact(player, movement, deltaTime);
    if (didHitEnemy) {
        player.GetGame()->OnAirSlamAttackHit(movement.GetPlayerNum());
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
    const bool hasReleaseRequest = input.HasChargedAttackReleaseRequest();
    if (!input.IsChargeAttackHeld() && !hasReleaseRequest) {
        // メニューなどによる入力破棄では発射せず、今回消費した分だけ戻す。
        combat.CancelSpecialAttackAndRefund(jewelGauge);
        input.ClearAttackBuffer();
        return;
    }
    if (hasReleaseRequest) {
        if (combat.GetSpecialAttackJewelsConsumed() >= 1) {
            combat.SpecialAttack(player, movement, jewelGauge, deltaTime);
            player.RequestStrongAttackAnimation();
            combat.FinishSpecialAttackCharging();
        } else {
            combat.CancelSpecialAttackAndRefund(jewelGauge);
        }
        input.ClearAttackBuffer();
        return;
    }

    const bool wasReady = combat.GetCanSpecialAttack();
    const float previousTimer = combat.GetSpecialChargingTimer();
    const float chargeDurationSeconds = combat.GetChargedAttackChargeDurationSeconds();
    combat.ReduceSpecialChargingTimer(std::min(std::max(0.0f, deltaTime), std::max(0.0f, previousTimer)));
    const float remainingSeconds = combat.GetSpecialChargingTimer();
    if (combat.GetSpecialAttackJewelsConsumed() == 0 && remainingSeconds <= chargeDurationSeconds * 0.5f) {
        if (!combat.ConsumeSpecialAttackJewel(jewelGauge)) {
            combat.CancelSpecialAttackAndRefund(jewelGauge);
            player.GetGame()->OnPlayerInsufficientJewels(player.GetPlayerNum(), 1);
            return;
        }
        player.GetGame()->VibrateControllerForPlayer(movement.GetPlayerNum(), 10000, 0, 1000);
        player.GetGame()->GetAudioSystem()->PlaySE("charging_se");
    }
    // 2個目が足りなくても1段階目を維持し、離せば半分の威力で発射できる。
    if (combat.GetSpecialAttackJewelsConsumed() == 1 && remainingSeconds <= 0.0f && jewelGauge.CanConsume(1)) {
        combat.ConsumeSpecialAttackJewel(jewelGauge);
    }
    if (!wasReady && combat.GetSpecialAttackJewelsConsumed() == 2) {
        combat.SetCanSpecialAttack(true);
        player.GetGame()->VibrateControllerForPlayer(movement.GetPlayerNum(), 30000, 0, 1000);
        player.GetGame()->GetAudioSystem()->PlaySE("charged_se");
    }
}

void PlayerStateMachine::UpdateContinuousAttacking(Player& player, PlayerMovement& movement, PlayerCombat& combat,
                                                   PlayerStatus& status, float deltaTime)
{
    const bool didAttack = combat.UpdateContinuousAttacking(player, movement, status, deltaTime);
    if (didAttack) {
        player.RequestNextWeakAttackAnimation();
    }
}
