#include "actor/player/PlayerCombat.h"

#include "actor/Enemy.h"
#include "actor/Player.h"
#include "actor/player/PlayerInput.h"
#include "actor/player/PlayerJewelGauge.h"
#include "actor/player/PlayerMovement.h"
#include "actor/player/PlayerStatus.h"
#include "system/AudioSystem.h"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

bool PlayerCombat::IsAttacking() const
{
    return mAttackMotionTimer >= 0.0f || mStrongAttackTimer >= 0.0f || mAttackPressTimer >= 0.0f ||
           mContinuousAttackingTimer >= 0.0f || mSpecialChargingTimer >= 0.0f || mAirAttackFloatingTimer >= 0.0f ||
           mHasPendingAttackHit;
}

void PlayerCombat::StartAttacking(Player& player, PlayerAttackInputKind attackInput, PlayerMovement& movement,
                                   PlayerStatus& status, float deltaTime)
{
    (void)movement;
    (void)status;
    (void)deltaTime;

    if (!player.GetOnGround() && attackInput == PlayerAttackInputKind::Wide) {
        mAttackKind = PlayerAttackKind::Wide;
        mAttackRange = mWideAttackRange;
        mAttackAngle = mWideAttackAngle;
        mAttackCooldownRemaining = mAttackCooldown;
        mAttack = mWideAttack / 2.0f;
        mAirAttackFloatingTimer = 0.5f;
        mIsAirAttacking = true;

        StartAttackHitDelay();
        return;
    }

    if (!player.GetOnGround()) {
        return;
    }

    const bool shouldFinishAssistCombo =
        player.GetGame()->IsAssistControlStyle() &&
        attackInput == PlayerAttackInputKind::Wide &&
        mAttackComboIndex == 2;

    if (attackInput == PlayerAttackInputKind::Normal || shouldFinishAssistCombo) {
        mAttackKind = PlayerAttackKind::Normal;
        mAttackRange = mNormalAttackRange;
        mAttackAngle = mNormalAttackAngle;
        mAttackCooldownRemaining = mLastAttackCooldown;
        mAttack = mNormalAttack;
    } else if (attackInput == PlayerAttackInputKind::Wide) {
        mAttackKind = PlayerAttackKind::Wide;
        mAttackRange = mWideAttackRange;
        mAttackAngle = mWideAttackAngle;
        mAttackCooldownRemaining = mAttackCooldown;
        mAttack = mWideAttack;
    }

    StartAttackHitDelay();
}

void PlayerCombat::StartCharging(Player& player)
{
    mAttackPressTimer = mDefaultAttackPressTimer;

    player.GetGame()->GetAudioSystem()->PlaySE("air_charging_se");
}

void PlayerCombat::ConfigureStrongAttack()
{
    mAttackKind = PlayerAttackKind::Strong;
    mAttackRange = mStrongAttackRange;
    mAttackAngle = mNormalAttackAngle;
    mAttackCooldownRemaining = mLastAttackCooldown;
    mAttack = mStrongAttack;
}

void PlayerCombat::StartStrongAttacking(Player& player, float deltaTime)
{
    (void)deltaTime;

    ConfigureStrongAttack();
    mIsAssistStrongAttack = false;

    float pressTime = 1.0f;
    if (mDefaultAttackPressTimer > 0.0f) {
        pressTime = std::min(1.0f, (mDefaultAttackPressTimer - mAttackPressTimer) / mDefaultAttackPressTimer);
    }
    mStrongAttackTimer = mDefaultStrongAttackTimer * pressTime;
    mAttackPressTimer = -1.0f;

    mIsStrongAttacked = true;

    if (mIsCharged) {
        StartAttackHitDelay();
    } else {
        ClearPendingAttackHit();
    }
}

void PlayerCombat::StartAssistStrongAttacking(Player& player, float deltaTime)
{
    (void)deltaTime;

    ConfigureStrongAttack();
    mIsAssistStrongAttack = true;
    mStrongAttackTimer = mDefaultStrongAttackTimer;
    mAttackPressTimer = -1.0f;
    mIsStrongAttacked = true;
    mIsCharged = true;

    StartAttackHitDelay();
    player.GetGame()->GetAudioSystem()->PlaySE("air_charged_se");
}

void PlayerCombat::FinishCharging(Player& player, const PlayerMovement& movement)
{
    player.GetGame()->OnPlayerFinishCharging(movement.GetPlayerNum());
    mIsCharged = true;
}

void PlayerCombat::FinishSpecialAttackCharging()
{
    mSpecialChargingTimer = -1.0f;
    mCanSpecialAttack = false;
}

void PlayerCombat::StartAttackHitDelay()
{
    mAttackHitDelayRemaining = std::max(0.0f, mAttackHitDelay);
    mHasPendingAttackHit = true;
}

void PlayerCombat::ClearPendingAttackHit()
{
    mAttackHitDelayRemaining = -1.0f;
    mHasPendingAttackHit = false;
}

bool PlayerCombat::UpdatePendingAttackHit(Player& player, PlayerMovement& movement, PlayerStatus& status,
                                          float deltaTime)
{
    if (!mHasPendingAttackHit) {
        return false;
    }

    mAttackHitDelayRemaining -= deltaTime;
    if (mAttackHitDelayRemaining > 0.0f) {
        return false;
    }

    ClearPendingAttackHit();

    // FindHitEnemies reads the player's facing vector here, so the hit direction
    // is the direction the player is facing at the end of the wind-up.
    Attack(player, movement, status, deltaTime);
    return true;
}

void PlayerCombat::Attack(Player& player, PlayerMovement& movement, PlayerStatus& status, float deltaTime)
{
    const std::vector<Enemy*> hitEnemies = mHitDetector.FindHitEnemies(player, *this);
    mAttackResolver.ResolveAttack(player, movement, status, *this, hitEnemies, deltaTime);
}

void PlayerCombat::WideAttack(Player& player, PlayerMovement& movement, PlayerStatus& status, float deltaTime)
{
    mAttackKind = PlayerAttackKind::Wide;
    mAttack = mWideAttack;
    mAttackRange = mWideAttackRange;
    mAttackAngle = mWideAttackAngle;

    Attack(player, movement, status, deltaTime);
}

void PlayerCombat::StrongAttack(Player& player, PlayerMovement& movement, PlayerStatus& status, float deltaTime)
{
    mAttackKind = PlayerAttackKind::Strong;
    mAttack = mStrongAttack;
    mAttackRange = mStrongAttackRange;
    mAttackAngle = mNormalAttackAngle;

    Attack(player, movement, status, deltaTime);
}

void PlayerCombat::SpecialAttack(Player& player, const PlayerMovement& movement, PlayerJewelGauge& jewelGauge,
                                 float deltaTime)
{
    const std::vector<Enemy*> enemies = mHitDetector.FindHitEnemies(player, *this);
    mAttackResolver.ResolveSpecialAttack(player, jewelGauge, enemies, deltaTime);

    player.GetGame()->VibrateControllerForPlayer(movement.GetPlayerNum(), 0, 40000, 1000);

    mCanSpecialAttack = false;
    mAttackCooldownRemaining = 1.0f;
}

void PlayerCombat::UpdateContinuousAttacking(Player& player, PlayerMovement& movement, PlayerStatus& status,
                                             float deltaTime)
{
    mAttackKind = PlayerAttackKind::Wide;
    mAttack = mWideAttack / 2.0f;
    mAttackRange = mWideAttackRange;
    mAttackAngle = mWideAttackAngle;

    mContinuousAttackingTimer -= deltaTime;
    mContinuousAttackingCooldown -= deltaTime;

    if (mContinuousAttackingCooldown <= 0.0f) {
        mContinuousAttackingCooldown = 0.25f;
        Attack(player, movement, status, deltaTime);
        mAttackMoveLockRemaining = 0.0f;
    }
}

void PlayerCombat::StartAfterAttackReaction(const Player& player, PlayerMovement& movement, PlayerStatus& status)
{
    mAttackMoveLockRemaining = 0.6f;
    mComboKeepTimer = mAttackMoveLockRemaining + 1.0f;

    if (player.GetOnGround()) {
        mAttackMotionTimer = mDefaultAttackMotionTimer;
    }

    mAttackComboIndex++;

    if (mAttackKind == PlayerAttackKind::Normal && mAttackComboIndex != 3) {
        mAttackComboIndex = 0;
        return;
    }

    if (mAttackKind == PlayerAttackKind::Strong) {
        StartTiredLock(status, movement, 5.0f);
        return;
    }

    if (mAttackComboIndex != 3) {
        return;
    }

    if (mAttackKind == PlayerAttackKind::Normal) {
        mAttackMoveLockRemaining = 1.0f;
    }

    if (mAttackKind == PlayerAttackKind::Wide && player.GetOnGround()) {
        mAttackCooldownRemaining = mLastAttackCooldown;
        mAttackMoveLockRemaining = 0.8f;
    }
}

void PlayerCombat::StartSpecialAttackCharging()
{
    mSpecialChargingTimer = 3.0f;
    mAttackRange = mWideAttackRange;
    mAttackAngle = mWideAttackAngle / 2.0f;
}

void PlayerCombat::StartContinuousAttacking()
{
    mContinuousAttackingTimer = 6.0f;
}

void PlayerCombat::StartTiredLock(PlayerStatus& status, PlayerMovement& movement, float lockTime)
{
    status.StartTired();
    mAttackMoveLockRemaining = lockTime;
    movement.StartDodgeLock(lockTime);
    mAttackCooldownRemaining = lockTime;
}

void PlayerCombat::ReduceTiredLock(PlayerStatus& status, PlayerMovement& movement, float reduceTime)
{
    mAttackMoveLockRemaining -= reduceTime;
    movement.SetDodgeCooldown(movement.GetDodgeCooldown() - reduceTime);
    mAttackCooldownRemaining -= reduceTime;

    if (mAttackMoveLockRemaining <= 0.0f) {
        status.EndTired();
    }
}

void PlayerCombat::EndTiredLock(PlayerStatus& status, PlayerMovement& movement)
{
    status.EndTired();
    mAttackMoveLockRemaining = 0.0f;
    movement.SetDodgeCooldown(0.0f);
    mAttackCooldownRemaining = 0.0f;
}

void PlayerCombat::CancelSpecialAttack()
{
    mIsAssistStrongAttack = false;
    mCanSpecialAttack = false;
    mSpecialChargingTimer = -1.0f;
    mContinuousAttackingTimer = -1.0f;
}

void PlayerCombat::OnLanded()
{
    mIsStrongAttacked = false;
    mIsAssistStrongAttack = false;
    mIsCharged = false;
    mIsAirAttacking = false;
}

void PlayerCombat::UpdateAirAttackFloatingTimer(float deltaTime)
{
    if (mAirAttackFloatingTimer > 0.0f) {
        mAirAttackFloatingTimer -= deltaTime;
    }
}

void PlayerCombat::UpdateAttackCooldown(float deltaTime)
{
    if (mAttackCooldownRemaining >= 0.0f) {
        mAttackCooldownRemaining -= deltaTime;
    }
}

void PlayerCombat::UpdateAttackMoveLock(PlayerStatus& status, float deltaTime)
{
    if (mAttackMoveLockRemaining > 0.0f) {
        mAttackMoveLockRemaining -= deltaTime;
        if (status.IsTired() && mAttackMoveLockRemaining <= 0.0f) {
            status.EndTired();
        }
    }
}

void PlayerCombat::UpdateAttackDodgeLock(float deltaTime)
{
    if (mAttackDodgeLockRemaining > 0.0f) {
        mAttackDodgeLockRemaining -= deltaTime;
    }
}

void PlayerCombat::UpdateComboKeepTimer(float deltaTime)
{
    mComboKeepTimer -= deltaTime;
    if (mComboKeepTimer >= 0.0f) {
        return;
    }

    mAttackComboIndex = 0;
}

bool PlayerCombat::CanAcceptMovementInput() const
{
    return CanMoveDuringAttack() && !IsSpecialCharging() && !GetCanSpecialAttack();
}
