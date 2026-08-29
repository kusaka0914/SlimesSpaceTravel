#include "actor/player/PlayerCombat.h"

#include "Game.h"

#include "actor/Enemy.h"
#include "actor/HazardActor.h"
#include "actor/Planet.h"
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
constexpr float airWeakAttackDamageMultiplier = 2.0f;
}

bool PlayerCombat::IsAttacking() const
{
    return mAttackMotionTimer >= 0.0f || mStrongAttackTimer >= 0.0f ||
           mContinuousAttackingTimer >= 0.0f || mSpecialChargingTimer >= 0.0f ||
           mHasPendingAttackHit;
}

void PlayerCombat::StartAttacking(Player& player, PlayerAttackInputKind attackInput, PlayerMovement& movement,
                                   PlayerStatus& status, float deltaTime)
{
    (void)status;
    (void)deltaTime;

    // 回避キャンセルで解除した空中弱攻撃の移動ロックは、次の攻撃開始時に
    // だけ通常どおり有効へ戻す。
    mAirAttackMovementUnlockedByDodge = false;

    if (!player.GetOnGround() && attackInput == PlayerAttackInputKind::Wide) {
        if (!CanStartAirAttack()) {
            return;
        }

        player.RestartAirborneGravityFallbackDelay();
        ResetGroundAttackCombo();
        ++mAirAttackCount;
        mAttackKind = PlayerAttackKind::Wide;
        mAttackRange = mWideAttackRange;
        mAttackAngle = mWideAttackAngle;
        mAttackCooldownRemaining =
            mAirWeakAttackCooldownSeconds;
        mAttack =
            mWideAttack *
            airWeakAttackDamageMultiplier;
        mIsAirAttacking = true;
        movement.CancelJumpApexHover();
        movement.CancelAirborneActionHover();
        movement.StopAirborneVerticalMovement(player);
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
        mAttackCooldownRemaining =
            mGroundWeakAttackCooldownSeconds;
        mAttack = mWideAttack;
    }

    const bool isGroundWeakAttack =
        attackInput == PlayerAttackInputKind::Wide &&
        !shouldFinishAssistCombo;
    mAttackDodgeLockRemaining =
        isGroundWeakAttack ? 0.0f : attackDodgeCancelDelaySeconds;
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
    EndContinuousAttacking();
    ConfigureStrongAttack();
    mIsAssistStrongAttack = false;
    mIsStrongAttacked = true;
    mIsCharged = true;
    ClearPendingAttackHit();
}

bool PlayerCombat::ResolveAirSlamImpact(
    Player& player,
    PlayerMovement& movement,
    float deltaTime)
{
    ConfigureStrongAttack();
    ++mResolvedAttackSequence;
    const std::vector<Enemy*> hitEnemies =
        mHitDetector.FindEnemiesInRadius(
            player,
            mStrongAttackRange);

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



    Attack(player, movement, status, deltaTime);
    return true;
}

void PlayerCombat::Attack(Player& player, PlayerMovement& movement, PlayerStatus& status, float deltaTime)
{
    ++mResolvedAttackSequence;
    const std::vector<Enemy*> hitEnemies = mHitDetector.FindHitEnemies(player, *this);
    bool didHitHazardActor = false;
    Planet* currentPlanet = player.GetCurrentPlanet();
    if (currentPlanet) {
        for (HazardActor* hazardActor :
             currentPlanet->GetHazardActors()) {
            if (hazardActor &&
                hazardActor->TryReactToAttack(player)) {
                didHitHazardActor = true;
            }
        }
    }

    mAttackResolver.ResolveAttack(
        player,
        movement,
        status,
        *this,
        hitEnemies,
        didHitHazardActor,
        deltaTime);
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
    mAttackKind = PlayerAttackKind::Charged;
    mAttackRange = mChargedAttackRange;
    mAttackAngle = mChargedAttackAngle;
    ++mResolvedAttackSequence;

    const std::vector<Enemy*> enemies = mHitDetector.FindHitEnemies(player, *this);
    mAttackResolver.ResolveSpecialAttack(
        player,
        jewelGauge,
        enemies,
        mChargedAttackDamage,
        deltaTime);

    player.GetGame()->VibrateControllerForPlayer(movement.GetPlayerNum(), 0, 40000, 1000);

    mCanSpecialAttack = false;
    mAttackCooldownRemaining = 1.0f;
}

bool PlayerCombat::UpdateContinuousAttacking(Player& player, PlayerMovement& movement, PlayerStatus& status,
                                             float deltaTime)
{
    mAttackKind = PlayerAttackKind::Wide;
    mAttack = mContinuousAttackDamage;
    mAttackRange = mContinuousAttackRange;
    mAttackAngle = mContinuousAttackAngle;

    AdvanceContinuousAttackDuration(deltaTime);
    mContinuousAttackingCooldown -= deltaTime;

    if (mContinuousAttackingCooldown > 0.0f) {
        return false;
    }

    mContinuousAttackingCooldown =
        std::max(0.0f, mContinuousAttackIntervalSeconds);
    Attack(player, movement, status, deltaTime);
    mAttackMoveLockRemaining = 0.0f;
    return true;
}

void PlayerCombat::AdvanceContinuousAttackDuration(float deltaTime)
{
    if (!IsContinuousAttacking()) {
        return;
    }

    mContinuousAttackingTimer =
        std::max(-1.0f, mContinuousAttackingTimer - deltaTime);
}

void PlayerCombat::StartAfterAttackReaction(const Player& player, PlayerMovement& movement, PlayerStatus& status)
{
    mAttackMoveLockRemaining = 0.6f;

    if (!player.GetOnGround()) {
        ResetGroundAttackCombo();
        return;
    }

    mComboKeepTimer = mAttackMoveLockRemaining + 1.0f;

    mAttackMotionTimer = mDefaultAttackMotionTimer;

    mAttackComboIndex++;




    if (mAttackKind == PlayerAttackKind::Normal) {
        if (mAttackComboIndex != 3) {
            ResetGroundAttackCombo();
        }
        StartGroundFinisherCooldown();
        return;
    }

    if (mAttackKind == PlayerAttackKind::Strong) {
        StartTiredLock(status, movement, 2.5f);
        return;
    }

    if (mAttackComboIndex != 3) {
        return;
    }

    if (mAttackKind == PlayerAttackKind::Wide && player.GetOnGround()) {
        StartGroundFinisherCooldown();
    }
}

void PlayerCombat::StartGroundFinisherCooldown()
{
    mAttackCooldownRemaining = mLastAttackCooldown;
    mAttackMoveLockRemaining = 0.8f;
}

void PlayerCombat::StartSpecialAttackCharging()
{
    mAttackKind = PlayerAttackKind::Charged;
    mAttackRange = mChargedAttackRange;
    mAttackAngle = mChargedAttackAngle;
    mAttack = mChargedAttackDamage;
    mSpecialChargingTimer =
        std::max(0.0f, mChargedAttackChargeDurationSeconds);
}

void PlayerCombat::StartContinuousAttacking()
{
    mContinuousAttackingTimer =
        std::max(0.0f, mContinuousAttackDurationSeconds);
    mContinuousAttackingCooldown = 0.0f;
}

void PlayerCombat::EndContinuousAttacking()
{
    mContinuousAttackingTimer = -1.0f;
    mContinuousAttackingCooldown = -1.0f;
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
    EndContinuousAttacking();
}

void PlayerCombat::CancelCurrentAttack()
{
    ClearPendingAttackHit();
    mAttackMotionTimer = -1.0f;
    mAttackMoveLockRemaining = 0.0f;
    mAttackDodgeLockRemaining = 0.0f;
    mIsAirAttacking = false;
}

void PlayerCombat::CancelAirAttackForDodge()
{
    CancelCurrentAttack();
    mAirAttackMovementUnlockedByDodge = true;
}

void PlayerCombat::OnLanded()
{
    mIsStrongAttacked = false;
    mIsAssistStrongAttack = false;
    mIsCharged = false;
    mIsAirAttacking = false;
    mAirAttackMovementUnlockedByDodge = false;
    mAirAttackCount = 0;
    ResetAirWeakAttackHitCount();
    EndAirDodgeAttack();
}

void PlayerCombat::PrepareAssistAirCombo()
{
    mAirAttackCount = 0;
    ResetAirWeakAttackHitCount();
    mIsAirAttacking = false;
}

bool PlayerCombat::RegisterAirWeakAttackHit()
{
    ++mAirWeakAttackHitCount;
    if (mAirWeakAttackHitCount < airWeakAttackHitsForBreak) {
        return false;
    }

    ResetAirWeakAttackHitCount();
    return true;
}

void PlayerCombat::ResetAirWeakAttackHitCount()
{
    mAirWeakAttackHitCount = 0;
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
            movementEnd,
            mAirDodgeHorizontalHitboxScale,
            mAirDodgeVerticalHitboxScale);
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

    const bool didHitEnemy =
        mAttackResolver.ResolveAirDodgeAttack(
            player,
            movement,
            newlyHitEnemies,
            mAirDodgeAttackDamage,
            mAirDodgeEnemyPushSpeed,
            mAirDodgeEnemyPushDampingPerSecond);
    if (didHitEnemy) {
        mAirAttackCount = 0;
        ResetAirWeakAttackHitCount();
        // 空中回避攻撃を当てた場合だけ、次の空中回避を許可する。
        // 外した場合は現在の回避を最後にして、着地まで再使用できない。
        movement.RestoreAirDodge();
    }
}

void PlayerCombat::EndAirDodgeAttack()
{
    mIsAirDodgeAttackActive = false;
    mAirDodgeHitEnemies.clear();
}

void PlayerCombat::StartGroundWeakAttackCooldown()
{
    mAttackCooldownRemaining =
        std::max(
            mAttackCooldownRemaining,
            mGroundWeakAttackCooldownSeconds);
}

void PlayerCombat::StartAirWeakAttackCooldown()
{
    mAttackCooldownRemaining =
        std::max(
            mAttackCooldownRemaining,
            mAirWeakAttackCooldownSeconds);
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

    ResetGroundAttackCombo();
}

bool PlayerCombat::CanAcceptMovementInput() const
{
    return CanMoveDuringAttack() && !IsSpecialCharging() && !GetCanSpecialAttack();
}
