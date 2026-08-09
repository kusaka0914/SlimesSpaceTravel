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

namespace {
constexpr float attackDodgeCancelDelaySeconds = 0.5f;
}

bool PlayerCombat::IsAttacking() const
{
    return mAttackMotionTimer >= 0.0f || mStrongAttackTimer >= 0.0f ||
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
        if (!CanStartAirAttack()) {
            return;
        }

        ++mAirAttackCount;
        mAttackKind = PlayerAttackKind::Wide;
        mAttackRange = mWideAttackRange;
        mAttackAngle = mWideAttackAngle;
        mAttackCooldownRemaining = mAttackCooldown;
        mAttack = mWideAttack;
        mAirAttackFloatingTimer = 0.5f;
        mIsAirAttacking = true;
        mAttackDodgeLockRemaining =
            attackDodgeCancelDelaySeconds;

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

    mAttackDodgeLockRemaining =
        attackDodgeCancelDelaySeconds;
    StartAttackHitDelay();
}

void PlayerCombat::ConfigureStrongAttack()
{
    mAttackKind = PlayerAttackKind::Strong;
    mAttackRange = mStrongAttackRange;
    mAttackAngle = mNormalAttackAngle;
    mAttackCooldownRemaining = mLastAttackCooldown;
    mAttack = mStrongAttack;
}

void PlayerCombat::StartAirSlamAttack()
{
    ConfigureStrongAttack();
    mIsAssistStrongAttack = false;
    mIsStrongAttacked = true;
    mIsCharged = true;
    ClearPendingAttackHit();
}

bool PlayerCombat::ResolveAirSlamImpact(
    Player& player,
    PlayerMovement& movement,
    PlayerStatus& status,
    float deltaTime)
{
    ConfigureStrongAttack();
    const std::vector<Enemy*> hitEnemies =
        mHitDetector.FindEnemiesInRadius(
            player,
            mStrongAttackRange);

    StartTiredLock(
        status,
        movement,
        5.0f);
    const bool didHitEnemy =
        mAttackResolver.ResolveAirSlamAttack(
            player,
            movement,
            *this,
            hitEnemies,
            deltaTime);

    mIsStrongAttackHit = didHitEnemy;
    mIsStrongAttacked = false;
    mIsCharged = false;
    return didHitEnemy;
}

void PlayerCombat::StartAssistStrongAttacking(Player& player, float deltaTime)
{
    (void)deltaTime;

    ConfigureStrongAttack();
    mIsAssistStrongAttack = true;
    mStrongAttackTimer = mDefaultStrongAttackTimer;
    mIsStrongAttacked = true;
    mIsCharged = true;

    StartAttackHitDelay();
    player.GetGame()->GetAudioSystem()->PlaySE("air_charged_se");
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

bool PlayerCombat::UpdateContinuousAttacking(Player& player, PlayerMovement& movement, PlayerStatus& status,
                                             float deltaTime)
{
    mAttackKind = PlayerAttackKind::Wide;
    mAttack = mWideAttack / 2.0f;
    mAttackRange = mWideAttackRange;
    mAttackAngle = mWideAttackAngle;

    mContinuousAttackingTimer -= deltaTime;
    mContinuousAttackingCooldown -= deltaTime;

    if (mContinuousAttackingCooldown > 0.0f) {
        return false;
    }

    mContinuousAttackingCooldown = 0.25f;
    Attack(player, movement, status, deltaTime);
    mAttackMoveLockRemaining = 0.0f;
    return true;
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

void PlayerCombat::CancelCurrentAttack()
{
    ClearPendingAttackHit();
    mAttackMotionTimer = -1.0f;
    mAirAttackFloatingTimer = -1.0f;
    mAttackMoveLockRemaining = 0.0f;
    mAttackDodgeLockRemaining = 0.0f;
    mIsAirAttacking = false;
}

void PlayerCombat::OnLanded()
{
    mIsStrongAttacked = false;
    mIsAssistStrongAttack = false;
    mIsCharged = false;
    mIsAirAttacking = false;
    mAirAttackCount = 0;
    EndAirDodgeAttack();
}

void PlayerCombat::StartAirDodgeAttack()
{
    mIsAirDodgeAttackActive = true;
    mAirDodgeHitEnemies.clear();
}

void PlayerCombat::UpdateAirDodgeAttack(
    Player& player,
    PlayerMovement& movement,
    const glm::vec3& movementStart,
    const glm::vec3& movementEnd)
{
    if (!mIsAirDodgeAttackActive) {
        return;
    }

    const std::vector<Enemy*> touchingEnemies =
        mHitDetector.FindEnemiesTouchingAirDodgeMovement(
            player,
            movementStart,
            movementEnd);
    std::vector<Enemy*> newlyHitEnemies;
    for (Enemy* enemy : touchingEnemies) {
        const bool wasAlreadyHit =
            std::find(
                mAirDodgeHitEnemies.begin(),
                mAirDodgeHitEnemies.end(),
                enemy) != mAirDodgeHitEnemies.end();
        if (wasAlreadyHit) {
            continue;
        }

        mAirDodgeHitEnemies.emplace_back(enemy);
        newlyHitEnemies.emplace_back(enemy);
    }

    constexpr float airDodgeDamageMultiplier = 2.0f;
    const float airDodgeDamage =
        mWideAttack * airDodgeDamageMultiplier;
    const bool didHitEnemy =
        mAttackResolver.ResolveAirDodgeAttack(
            player,
            movement,
            newlyHitEnemies,
            airDodgeDamage);
    if (didHitEnemy) {
        mAirAttackCount = 0;
    }
}

void PlayerCombat::EndAirDodgeAttack()
{
    mIsAirDodgeAttackActive = false;
    mAirDodgeHitEnemies.clear();
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
