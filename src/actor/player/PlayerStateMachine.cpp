#include "actor/player/PlayerStateMachine.h"

#include "actor/Player.h"
#include "actor/player/PlayerBoatRide.h"
#include "actor/player/PlayerCombat.h"
#include "actor/player/PlayerGrounding.h"
#include "actor/player/PlayerInput.h"
#include "actor/player/PlayerJewelGauge.h"
#include "actor/player/PlayerMovement.h"
#include "actor/player/PlayerRespawn.h"
#include "actor/player/PlayerStatus.h"
#include "app/Game.h"
#include "system/SceneSystem.h"

#include <algorithm>
#include <glm/glm.hpp>

void PlayerStateMachine::Update(Player& player, PlayerInput& input, PlayerMovement& movement,
                                PlayerGrounding& grounding, PlayerBoatRide& boatRide, PlayerCombat& combat,
                                PlayerJewelGauge& jewelGauge, PlayerStatus& status, PlayerRespawn& respawn,
                                float deltaTime)
{
    SceneSystem* sceneSystem = player.GetGame()->GetSceneSystem();
    if (!sceneSystem || (!sceneSystem->IsPlaying() && !sceneSystem->IsWaitingForTutorialPlayerJump())) {
        input.SuppressCombatInputUntilReleased();
        return;
    }

    const glm::vec3 prevPos = player.GetPos();

    if (status.IsAlive()) {
        UpdateAlive(player, input, movement, grounding, boatRide, combat, jewelGauge, status, respawn, deltaTime);
        respawn.CheckFallRespawn(player, *this, combat, status, prevPos);
    } else {
        status.Die(*player.GetGame());
    }
}

void PlayerStateMachine::UpdateAlive(Player& player, PlayerInput& input, PlayerMovement& movement,
                                     PlayerGrounding& grounding, PlayerBoatRide& boatRide, PlayerCombat& combat,
                                     PlayerJewelGauge& jewelGauge, PlayerStatus& status, PlayerRespawn& respawn,
                                     float deltaTime)
{
    input.UpdateCombatInput(deltaTime, player.GetGame()->IsAssistControlStyle(),
                            combat.IsSpecialCharging() || combat.GetCanSpecialAttack());
    const bool allowsAirDodgeBuffering = mActionState == PlayerActionState::Idle ||
                                         mActionState == PlayerActionState::Attacking ||
                                         mActionState == PlayerActionState::StrongAttacking;
    input.CaptureAirDodgeInput(!player.GetOnGround(), allowsAirDodgeBuffering);
    movement.UpdateCameraRelativeMovementDirections(player, input);
    boatRide.Update(player, input, movement, respawn, deltaTime);
    UpdateCoyoteTime(player, deltaTime);

    if (jewelGauge.ShouldStartRecoverTimer()) {
        StartJewelTimer(jewelGauge);
    }

    // 弱攻撃の命中判定を終えてから長押しへ移行し、予約中の攻撃や移動ロックを持ち越さない。
    const bool canTransitionFromAttack =
        (mActionState == PlayerActionState::Idle || mActionState == PlayerActionState::Attacking) &&
        !player.IsAttachedToPlatform() && !combat.HasPendingAttackHit();
    if (canTransitionFromAttack && (TryStartSpecialAttack(input, combat, jewelGauge) ||
                                    TryStartContinuousAttack(player, input, combat, jewelGauge))) {
        mAttackDirectionTarget = nullptr;
        movement.ClearStrongAttackDirectionOverride();
        movement.CancelAirborneActionHover();
        ChangeState(PlayerActionState::Idle);
        // 長押しが成立したフレームの経過時間を、溜め・連続攻撃にも重ねて加算しない。
        UpdateTimer(input, movement, grounding, combat, jewelGauge, status, deltaTime);
        input.EndFrame();
        return;
    }

    switch (mActionState) {
    case PlayerActionState::Idle:
        UpdateIdle(player, input, movement, combat, jewelGauge, status, deltaTime);
        break;
    case PlayerActionState::Dodging:
        UpdateDodging(player, input, movement, grounding, combat, deltaTime);
        break;
    case PlayerActionState::Attacking:
        UpdateAttacking(player, input, movement, combat, status, deltaTime);
        break;
    case PlayerActionState::StrongAttacking:
        UpdateStrongAttacking(player, input, movement, combat, status, deltaTime);
        break;
    case PlayerActionState::AirSlamAttacking:
        UpdateAirSlamAttacking(player, movement, combat, deltaTime);
        break;
    case PlayerActionState::KnockedBack:
        UpdateKnockedBack(player, movement, combat, status, deltaTime);
        break;
    }

    UpdateTimer(input, movement, grounding, combat, jewelGauge, status, deltaTime);

    // 各状態の押下開始判定が終わるまで、前フレームの入力を保持する。
    input.EndFrame();
}

void PlayerStateMachine::UpdateTimer(PlayerInput& input, PlayerMovement& movement, PlayerGrounding& grounding,
                                     PlayerCombat& combat, PlayerJewelGauge& jewelGauge, PlayerStatus& status,
                                     float deltaTime)
{
    movement.UpdateDodgeCooldown(deltaTime);

    if (jewelGauge.GetRecoverTimer() >= 0.0f) {
        jewelGauge.UpdateRecoverTimer(deltaTime);
    }

    combat.UpdateAttackCooldown(deltaTime);
    combat.UpdateAttackMoveLock(status, deltaTime);
    combat.UpdateAttackDodgeLock(deltaTime);
    status.UpdateInvincibleTimer(deltaTime);
    grounding.UpdateRayCastTimer(deltaTime);
    input.UpdateInputAvailableTimer(deltaTime);
    input.UpdateAttackBuffer(deltaTime);
    input.UpdateAirDodgeBuffer(deltaTime);

    if (combat.GetComboKeepTimer() > 0.0f) {
        combat.UpdateComboKeepTimer(deltaTime);
    }
}

void PlayerStateMachine::UpdateCoyoteTime(const Player& player, float deltaTime)
{
    if (player.GetOnGround()) {
        mCoyoteTimeRemaining = mCoyoteTimeDuration;
        return;
    }

    mCoyoteTimeRemaining = std::max(0.0f, mCoyoteTimeRemaining - deltaTime);
}

bool PlayerStateMachine::IsAttackingState() const
{
    return mActionState == PlayerActionState::Attacking || mActionState == PlayerActionState::StrongAttacking ||
           mActionState == PlayerActionState::AirSlamAttacking;
}

void PlayerStateMachine::StartIdle()
{
    ChangeState(PlayerActionState::Idle);
}

void PlayerStateMachine::StartJewelTimer(PlayerJewelGauge& jewelGauge)
{
    jewelGauge.StartRecoverTimer(30.0f);
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
