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
           mContinuousAttackingTimer >= 0.0f || mSpecialChargingTimer >= 0.0f || mAirAttackFloatingTimer >= 0.0f;
}

void PlayerCombat::StartAttacking(Player& player, const PlayerInput& input, PlayerMovement& movement,
                                  PlayerStatus& status, float deltaTime)
{
    if (!player.GetOnGround() && input.GetWideAttackPressed()) {
        mAttackKind = PlayerAttackKind::Wide;
        mAttackRange = mWideAttackRange;
        mAttackAngle = mWideAttackAngle;
        mAttackCooldownRemaining = mAttackCooldown;
        mAttack = mWideAttack / 2.0f;
        mAirAttackFloatingTimer = 0.5f;
        mIsAirAttacking = true;

        Attack(player, movement, status, deltaTime);
        return;
    }

    if (!player.GetOnGround()) {
        return;
    }

    if (input.GetAttackPressed()) {
        mAttackKind = PlayerAttackKind::Normal;
        mAttackRange = mNormalAttackRange;
        mAttackAngle = mNormalAttackAngle;
        mAttackCooldownRemaining = mLastAttackCooldown;
        mAttack = mNormalAttack;
    } else if (input.GetWideAttackPressed()) {
        mAttackKind = PlayerAttackKind::Wide;
        mAttackRange = mWideAttackRange;
        mAttackAngle = mWideAttackAngle;
        mAttackCooldownRemaining = mAttackCooldown;
        mAttack = mWideAttack;
    }

    Attack(player, movement, status, deltaTime);
}

void PlayerCombat::StartCharging(Player& player)
{
    mAttackPressTimer = mDefaultAttackPressTimer;

    player.GetGame()->GetAudioSystem()->PlaySE("air_charging_se");
}

void PlayerCombat::StartStrongAttacking(Player& player, float deltaTime)
{
    mAttackKind = PlayerAttackKind::Strong;
    mAttackRange = mStrongAttackRange;
    mAttackAngle = mNormalAttackAngle;
    mAttackCooldownRemaining = mLastAttackCooldown;
    mAttack = mStrongAttack;

    float pressTime = 1.0f;
    if (mDefaultAttackPressTimer > 0.0f) {
        pressTime = std::min(1.0f, (mDefaultAttackPressTimer - mAttackPressTimer) / mDefaultAttackPressTimer);
    }
    mStrongAttackTimer = mDefaultStrongAttackTimer * pressTime;
    mAttackPressTimer = -1.0f;

    mIsStrongAttacked = true;
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
    mAttackMoveLockRemaining = 1.0f;
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

void PlayerCombat::CancelSpecialAttack()
{
    mCanSpecialAttack = false;
    mSpecialChargingTimer = -1.0f;
    mContinuousAttackingTimer = -1.0f;
}

void PlayerCombat::OnLanded()
{
    mIsStrongAttacked = false;
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