#include "actor/player/PlayerCombat.h"

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
#include <stdexcept>
#include <utility>

PlayerCombat::PlayerCombat(const PlayerCombatDependencies& dependencies)
    : mDependencies(dependencies),
      mHitDetector(dependencies.physics),
      mAttackResolver(dependencies.audio, dependencies.particles, dependencies.onAttackHit, dependencies.requestHitStop)
{
    if (!mDependencies.onAttackHit || !mDependencies.vibrate) {
        throw std::invalid_argument("PlayerCombat requires feedback callbacks");
    }
}

bool PlayerCombat::IsAttacking() const
{
    return mAttackMotionTimer >= 0.0f || mStrongAttackTimer >= 0.0f || mContinuousAttackingTimer >= 0.0f ||
           mSpecialChargingTimer >= 0.0f || mHasPendingAttackHit;
}

void PlayerCombat::StartAttacking(Player& player, PlayerAttackInputKind attackInput, PlayerMovement& movement,
                                  PlayerStatus& status, float deltaTime)
{
    (void)status;
    (void)deltaTime;

    ResetComboOnGroundStateChange(player.GetOnGround());
    if (attackInput == PlayerAttackInputKind::None ||
        (attackInput == PlayerAttackInputKind::Normal && !CanStartComboFinisher())) {
        return;
    }

    mAirAttackMovementUnlockedByDodge = false;
    mIsComboFinisher = false;
    mIsAirAttacking = !player.GetOnGround();
    mComboStartedInAir = mIsAirAttacking;
    if (mIsAirAttacking) {
        mHasAirWeakHitForNextDodge = false;
        player.RestartAirborneGravityFallbackDelay();
        movement.CancelJumpApexHover();
        movement.CancelAirborneActionHover();
        movement.StopAirborneVerticalMovement(player);
    }

    mIsComboAttack = true;
    const int currentComboStage = std::clamp(mAttackComboIndex, 0, 4);
    mCurrentComboStageIndex = currentComboStage;
    mIsComboFinisher = attackInput == PlayerAttackInputKind::Normal || (!mIsAirAttacking && currentComboStage == 4);
    // 空中5段目は4段目の弱攻撃設定を共用し、自動で強攻撃へ派生させない。
    const int weakAttackStage = std::min(currentComboStage, 3);
    mCurrentComboAttack = mIsComboFinisher ? mComboSettings.stages.back()
                                           : mComboSettings.stages[static_cast<std::size_t>(weakAttackStage)];
    if (mIsAirAttacking && !mIsComboFinisher && currentComboStage == 4) {
        mCurrentComboAttack.cooldownSeconds = mComboSettings.stages.back().cooldownSeconds;
    }
    mAttackComboIndex = mIsComboFinisher || currentComboStage == 4 ? 0 : currentComboStage + 1;
    mComboHitsRemaining = mCurrentComboAttack.hitCount;
    mAttackKind = mIsComboFinisher ? PlayerAttackKind::Normal : PlayerAttackKind::Wide;
    mAttackRange = mIsComboFinisher ? mNormalAttackRange : mWideAttackRange;
    mAttackAngle = mIsComboFinisher ? mNormalAttackAngle : mWideAttackAngle;
    mAttack = (mIsComboFinisher ? mNormalAttack : mWideAttack) * mCurrentComboAttack.damageMultiplier;
    mAttackCooldownRemaining = 0.0f;
    mAttackDodgeLockRemaining = mCurrentComboAttack.dodgeCancelSeconds;
    mAttackMotionTimer = -1.0f;
    mAttackHitDelayRemaining = mCurrentComboAttack.hitDelaySeconds;
    mHasPendingAttackHit = true;
    mComboKeepTimer = mCurrentComboAttack.hitDelaySeconds +
                      (mCurrentComboAttack.hitCount - 1) * mCurrentComboAttack.hitIntervalSeconds +
                      mCurrentComboAttack.motionSeconds + mComboSettings.continuationWindowSeconds;
}

void PlayerCombat::NotifyComboHit(int playerNumber, bool didHitEnemy) const
{
    if (didHitEnemy && mIsComboFinisher && mDependencies.onComboFinisherHit) {
        mDependencies.onComboFinisherHit();
    }
    mDependencies.vibrate(playerNumber, mIsComboFinisher ? 16000 : 0, mIsComboFinisher ? 30000 : 10000,
                          mIsComboFinisher ? 180 : 60);
}

void PlayerCombat::ConfigureCombo(PlayerComboSettings settings)
{
    settings.Sanitize();
    mComboSettings = std::move(settings);
}

void PlayerCombat::ConfigureStrongAttack()
{
    mIsComboAttack = false;
    mAttackKind = PlayerAttackKind::Strong;
    mAttackRange = mStrongAttackRange;
    mAttackAngle = mNormalAttackAngle;
    mAttackCooldownRemaining = mLastAttackCooldown;
    mAttack = mStrongAttack;
}

void PlayerCombat::StartAirSlamAttack(const Player& player)
{
    mAirSlamStartHeight = player.CalculateAirSlamHeight();
    // 通常コンボからの派生でも、残った命中予約や硬直を叩きつけへ持ち越さない。
    CancelCurrentAttack();
    ResetAttackCombo();
    mHasAirWeakHitForNextDodge = false;
    mDidAirSlamContactEnemy = false;
    mAirSlamHitEnemies.clear();
    EndContinuousAttacking();
    ConfigureStrongAttack();
    mIsAssistStrongAttack = false;
    mIsStrongAttacked = true;
    mIsCharged = true;
    ClearPendingAttackHit();
}

bool PlayerCombat::UpdateAirSlamContact(Player& player, const PlayerMovement& movement)
{
    const std::vector<Enemy*> airborneEnemies = mHitDetector.FindAirborneEnemiesOnCurrentPlanet(player);

    std::vector<Enemy*> newlyHitEnemies;
    for (Enemy* enemy : airborneEnemies) {
        if (!enemy || enemy->IsOnGround() ||
            std::find(mAirSlamHitEnemies.begin(), mAirSlamHitEnemies.end(), enemy) != mAirSlamHitEnemies.end()) {
            continue;
        }

        mAirSlamHitEnemies.emplace_back(enemy);
        newlyHitEnemies.emplace_back(enemy);
    }

    const bool didStartGravitySlam = mAttackResolver.ResolveAirSlamContact(
        player, movement, newlyHitEnemies, mAirSlamEnemyDownwardSpeed, mStrongAttack, mAirSlamFullDamageHeight,
        mAirSlamMinimumDamageRatio, mAirSlamStartHeight, !mDidAirSlamContactEnemy);
    mDidAirSlamContactEnemy |= didStartGravitySlam;
    return didStartGravitySlam;
}

bool PlayerCombat::ResolveAirSlamImpact(Player& player, PlayerMovement& movement, float deltaTime)
{
    ConfigureStrongAttack();
    ++mResolvedAttackSequence;
    const std::vector<Enemy*> hitEnemies = mHitDetector.FindEnemiesInRadius(player, mStrongAttackRange);

    bool didHitGroundedEnemy = false;
    if (!mDidAirSlamContactEnemy) {
        didHitGroundedEnemy = mAttackResolver.ResolveAirSlamAttack(player, movement, *this, hitEnemies, deltaTime);
    }

    const bool didHitEnemy = mDidAirSlamContactEnemy || didHitGroundedEnemy;
    if (!didHitEnemy) {
        mDependencies.audio.PlaySE("attack_miss_se");
    }

    mIsStrongAttackHit = didHitEnemy;
    mIsStrongAttacked = false;
    mIsCharged = false;
    mDidAirSlamContactEnemy = false;
    mAirSlamHitEnemies.clear();
    return didHitGroundedEnemy;
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
    mDependencies.audio.PlaySE("air_charged_se");
}

void PlayerCombat::FinishSpecialAttackCharging()
{
    mSpecialAttackJewelsConsumed = 0;
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

    if (mIsComboAttack) {
        if (mComboHitsRemaining < mCurrentComboAttack.hitCount) {
            // 追加ヒットにも独立した振りを付け、1入力の2連撃を見た目でも区別する。
            player.RequestComboAttackAnimation();
        }
        Attack(player, movement, status, deltaTime);
        --mComboHitsRemaining;
        if (mComboHitsRemaining > 0) {
            // 同じ入力の2ヒット目を予約し、コンボ段数と攻撃後の硬直は最後のヒットでだけ進める。
            mAttackHitDelayRemaining += mCurrentComboAttack.hitIntervalSeconds;
        } else {
            ClearPendingAttackHit();
            StartAfterAttackReaction(player, movement, status);
        }
        return true;
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
        for (HazardActor* hazardActor : currentPlanet->GetHazardActors()) {
            if (hazardActor && hazardActor->TryReactToAttack(player)) {
                didHitHazardActor = true;
            }
        }
    }

    mAttackResolver.ResolveAttack(player, movement, status, *this, hitEnemies, didHitHazardActor, deltaTime);
}

void PlayerCombat::WideAttack(Player& player, PlayerMovement& movement, PlayerStatus& status, float deltaTime)
{
    mIsComboAttack = false;
    mAttackKind = PlayerAttackKind::Wide;
    mAttack = mWideAttack;
    mAttackRange = mWideAttackRange;
    mAttackAngle = mWideAttackAngle;

    Attack(player, movement, status, deltaTime);
}

void PlayerCombat::StrongAttack(Player& player, PlayerMovement& movement, PlayerStatus& status, float deltaTime)
{
    mIsComboAttack = false;
    mAttackKind = PlayerAttackKind::Strong;
    mAttack = mStrongAttack;
    mAttackRange = mStrongAttackRange;
    mAttackAngle = mNormalAttackAngle;

    Attack(player, movement, status, deltaTime);
}

void PlayerCombat::SpecialAttack(Player& player, const PlayerMovement& movement, PlayerJewelGauge& jewelGauge,
                                 float deltaTime)
{
    if (mSpecialAttackJewelsConsumed < 1) {
        return;
    }
    const float damageMultiplier = mSpecialAttackJewelsConsumed >= 2 ? 1.0f : 0.5f;
    mAttack = mChargedAttackDamage * damageMultiplier;
    mAttackKind = PlayerAttackKind::Charged;
    mAttackRange = mChargedAttackRange;
    mAttackAngle = mChargedAttackAngle;
    ++mResolvedAttackSequence;

    const std::vector<Enemy*> enemies = mHitDetector.FindHitEnemies(player, *this);
    mAttackResolver.ResolveSpecialAttack(player, jewelGauge, enemies, mAttack, deltaTime);

    mDependencies.vibrate(movement.GetPlayerNum(), 0, 40000, 1000);

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

    mContinuousAttackingCooldown = std::max(0.0f, mContinuousAttackIntervalSeconds);
    Attack(player, movement, status, deltaTime);
    mAttackMoveLockRemaining = 0.0f;
    return true;
}

void PlayerCombat::AdvanceContinuousAttackDuration(float deltaTime)
{
    if (!IsContinuousAttacking()) {
        return;
    }

    mContinuousAttackingTimer = std::max(-1.0f, mContinuousAttackingTimer - deltaTime);
}

void PlayerCombat::StartAfterAttackReaction(const Player& player, PlayerMovement& movement, PlayerStatus& status)
{
    if (mIsComboAttack) {
        mAttackMoveLockRemaining = mCurrentComboAttack.moveLockSeconds;
        mAttackMotionTimer = mCurrentComboAttack.motionSeconds;
        mAttackCooldownRemaining = mCurrentComboAttack.cooldownSeconds;
        if (mIsAirAttacking) {
            // クールタイム終了後も入力の猶予を残し、次の空中攻撃へつなげられるようにする。
            mAirComboHoverRemainingSeconds =
                mCurrentComboAttack.cooldownSeconds + mComboSettings.airAttackHoverExtraSeconds;
        }
        mComboKeepTimer = mCurrentComboAttack.motionSeconds + mCurrentComboAttack.cooldownSeconds +
                          mComboSettings.continuationWindowSeconds;
        if (mIsComboFinisher || mAttackComboIndex == 0) {
            ResetAttackCombo();
        }
        return;
    }
    mAttackMoveLockRemaining = 0.6f;

    if (!player.GetOnGround()) {
        ResetAttackCombo();
        return;
    }

    mComboKeepTimer = mAttackMoveLockRemaining + 1.0f;

    mAttackMotionTimer = mDefaultAttackMotionTimer;

    if (mAttackKind == PlayerAttackKind::Normal) {
        ResetAttackCombo();
        StartGroundFinisherCooldown();
    } else if (mAttackKind == PlayerAttackKind::Strong) {
        StartTiredLock(status, movement, 2.5f);
    }
}

void PlayerCombat::StartGroundFinisherCooldown()
{
    mAttackCooldownRemaining = mLastAttackCooldown;
    mAttackMoveLockRemaining = 0.8f;
}

void PlayerCombat::StartSpecialAttackCharging()
{
    mIsComboAttack = false;
    ResetAttackCombo();
    mSpecialAttackJewelsConsumed = 0;
    mCanSpecialAttack = false;
    mAttackKind = PlayerAttackKind::Charged;
    mAttackRange = mChargedAttackRange;
    mAttackAngle = mChargedAttackAngle;
    mAttack = mChargedAttackDamage;
    mSpecialChargingTimer = std::max(0.0f, mChargedAttackChargeDurationSeconds);
}

bool PlayerCombat::ConsumeSpecialAttackJewel(PlayerJewelGauge& jewelGauge)
{
    if (!IsSpecialCharging() || mSpecialAttackJewelsConsumed >= 2 || !jewelGauge.Consume(1)) {
        return false;
    }
    ++mSpecialAttackJewelsConsumed;
    return true;
}

void PlayerCombat::CancelSpecialAttackAndRefund(PlayerJewelGauge& jewelGauge)
{
    jewelGauge.AddFromItem(mSpecialAttackJewelsConsumed);
    CancelSpecialAttack();
}

void PlayerCombat::StartContinuousAttacking()
{
    mIsComboAttack = false;
    ResetAttackCombo();
    mContinuousAttackingTimer = std::max(0.0f, mContinuousAttackDurationSeconds);
    mContinuousAttackingCooldown = 0.0f;
}

void PlayerCombat::EndContinuousAttacking()
{
    if (mContinuousAttackingTimer >= 0.0f) {
        ResetAttackCombo();
    }
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
    mControlTransferRecoveryRemaining = std::max(0.0f, mControlTransferRecoveryRemaining - reduceTime);

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
    mControlTransferRecoveryRemaining = 0.0f;
}

void PlayerCombat::CancelSpecialAttack()
{
    mSpecialAttackJewelsConsumed = 0;
    mIsAssistStrongAttack = false;
    mCanSpecialAttack = false;
    mSpecialChargingTimer = -1.0f;
    EndContinuousAttacking();
}

void PlayerCombat::CancelCurrentAttack()
{
    mAirComboHoverRemainingSeconds = 0.0f;
    mIsComboAttack = false;
    mComboHitsRemaining = 0;
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
    ResetComboOnGroundStateChange(true);
    mAirComboHoverRemainingSeconds = 0.0f;
    mIsStrongAttacked = false;
    mIsAssistStrongAttack = false;
    mIsCharged = false;
    mIsAirAttacking = false;
    mAirAttackMovementUnlockedByDodge = false;
    mHasAirWeakHitForNextDodge = false;
    EndAirDodgeAttack();
}

void PlayerCombat::PrepareAssistAirCombo()
{
    mHasAirWeakHitForNextDodge = false;
    mIsAirAttacking = false;
}

void PlayerCombat::StartAirDodgeAttack()
{
    mIsAirDodgeAttackActive = true;
    mIsEnhancedAirDodgeAttackActive = mHasAirWeakHitForNextDodge;
    mHasAirWeakHitForNextDodge = false;
    mAirDodgeHitEnemies.clear();
}

void PlayerCombat::UpdateAirDodgeAttack(Player& player, PlayerMovement& movement, const glm::vec3& movementStart,
                                        const glm::vec3& movementEnd, float deltaTime)
{
    if (!mIsAirDodgeAttackActive) {
        return;
    }

    const std::vector<Enemy*> touchingEnemies = mHitDetector.FindEnemiesTouchingAirDodgeMovement(
        player, movementStart, movementEnd, mAirDodgeHorizontalHitboxScale, mAirDodgeVerticalHitboxScale);
    std::vector<Enemy*> newlyHitEnemies;
    for (Enemy* enemy : touchingEnemies) {
        const bool wasAlreadyHit =
            std::find(mAirDodgeHitEnemies.begin(), mAirDodgeHitEnemies.end(), enemy) != mAirDodgeHitEnemies.end();
        if (wasAlreadyHit) {
            continue;
        }

        mAirDodgeHitEnemies.emplace_back(enemy);
        newlyHitEnemies.emplace_back(enemy);
    }

    const bool didHitEnemy = mAttackResolver.ResolveAirDodgeAttack(
        player, movement, newlyHitEnemies, mAirDodgeAttackDamage, mAirDodgeGuardDamage, mAirDodgeEnemyPushSpeed,
        mAirDodgeEnemyPushDampingPerSecond, mIsEnhancedAirDodgeAttackActive ? mAirComboDodgeEnemyLiftHeight : 0.0f,
        deltaTime);
    if (didHitEnemy) {
        // 空中ダッシュ攻撃を当てた場合だけ、次の空中ダッシュを許可する。
        // 外した場合は現在のダッシュを最後にして、着地まで再使用できない。
        movement.RestoreAirDodge();
    }
}

float PlayerCombat::CalculateCurrentGuardDamage(bool isPlayerGrounded) const
{
    if (mIsComboAttack) {
        return mCurrentComboAttack.guardDamage;
    }
    if (IsContinuousAttacking()) {
        return mContinuousAttackGuardDamage;
    }

    if (!isPlayerGrounded) {
        return mAirWeakGuardDamage;
    }

    // 地上強攻撃は、単発でも弱攻撃コンボの最終段と同じ扱いにする。
    if (mAttackKind == PlayerAttackKind::Normal) {
        return mGroundCombo3GuardDamage;
    }

    if (mAttackKind != PlayerAttackKind::Wide) {
        return mGroundWideGuardDamage;
    }

    return mGroundWideGuardDamage;
}

bool PlayerCombat::ShouldEnsureCurrentGuardSegmentBreak(bool isPlayerGrounded) const
{
    if (mIsComboAttack) {
        return false;
    }
    if (!isPlayerGrounded || IsContinuousAttacking()) {
        return false;
    }

    return mAttackKind == PlayerAttackKind::Normal;
}

void PlayerCombat::EndAirDodgeAttack()
{
    mIsAirDodgeAttackActive = false;
    mIsEnhancedAirDodgeAttackActive = false;
    mAirDodgeHitEnemies.clear();
}

void PlayerCombat::StartGroundWeakAttackCooldown()
{
    mAttackCooldownRemaining = std::max(mAttackCooldownRemaining, mGroundWeakAttackCooldownSeconds);
}

void PlayerCombat::StartAirWeakAttackCooldown()
{
    mAttackCooldownRemaining = std::max(mAttackCooldownRemaining, mAirWeakAttackCooldownSeconds);
}

void PlayerCombat::ResetComboOnGroundStateChange(bool isOnGround)
{
    const bool isAirborne = !isOnGround;
    if (mComboStartedInAir != isAirborne) {
        ResetAttackCombo();
    }
}

void PlayerCombat::InheritControlRestrictions(const PlayerCombat& source)
{
    mAttackComboIndex = source.mAttackComboIndex;
    mComboStartedInAir = source.mComboStartedInAir;
    mComboKeepTimer = source.mComboKeepTimer;
    mAirComboHoverRemainingSeconds = source.mAirComboHoverRemainingSeconds;
    const float weakAttackCooldown =
        source.mIsAirAttacking ? source.mAirWeakAttackCooldownSeconds : source.mGroundWeakAttackCooldownSeconds;
    float remainingAttack = std::max(0.0f, source.mAttackMotionTimer);
    if (!source.mIsComboAttack && source.mAttackMotionTimer >= 0.0f) {
        remainingAttack += std::max(0.0f, weakAttackCooldown);
    }
    if (source.mHasPendingAttackHit && !source.mIsComboAttack) {
        // 命中後に始まるモーションと待ち時間も含め、別の体で次の攻撃だけを先に始めないようにする。
        remainingAttack = std::max(remainingAttack, std::max(0.0f, source.mAttackHitDelayRemaining) +
                                                        std::max(0.0f, source.mDefaultAttackMotionTimer) +
                                                        std::max(0.0f, weakAttackCooldown));
    }
    if (source.mIsComboAttack && source.mHasPendingAttackHit) {
        remainingAttack = std::max(
            remainingAttack,
            std::max(0.0f, source.mAttackHitDelayRemaining) +
                std::max(0, source.mComboHitsRemaining - 1) * source.mCurrentComboAttack.hitIntervalSeconds +
                std::max(source.mCurrentComboAttack.motionSeconds, source.mCurrentComboAttack.cooldownSeconds));
    }
    mControlTransferRecoveryRemaining =
        std::max({mControlTransferRecoveryRemaining, source.GetAttackCooldownRemaining(), remainingAttack,
                  source.mStrongAttackTimer, source.mContinuousAttackingTimer});
    mAttackMoveLockRemaining = std::max(mAttackMoveLockRemaining, source.mAttackMoveLockRemaining);
    mAttackDodgeLockRemaining = std::max(mAttackDodgeLockRemaining, source.mAttackDodgeLockRemaining);
}

void PlayerCombat::ResetForPlacementInheritance(const PlayerCombat& source, bool isAirborne)
{
    if (&source == this) {
        return;
    }
    CancelCurrentAttack();
    CancelSpecialAttack();
    OnLanded();
    ResetAttackCombo();
    mStrongAttackTimer = -1.0f;
    mAttackCooldownRemaining = 0.0f;
    mControlTransferRecoveryRemaining = 0.0f;
    (void)isAirborne;
    InheritControlRestrictions(source);
}

void PlayerCombat::UpdateAttackCooldown(float deltaTime)
{
    mAirComboHoverRemainingSeconds = std::max(0.0f, mAirComboHoverRemainingSeconds - std::max(0.0f, deltaTime));
    mControlTransferRecoveryRemaining = std::max(0.0f, mControlTransferRecoveryRemaining - std::max(0.0f, deltaTime));
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

    ResetAttackCombo();
}

bool PlayerCombat::CanAcceptMovementInput() const
{
    return CanMoveDuringAttack();
}
