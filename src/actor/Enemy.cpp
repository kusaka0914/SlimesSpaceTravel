#include "Enemy.h"
#include "CharacterActor.h"
#include "Game.h"
#include "Stage.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "actor/Star.h"
#include "state/UIState.h"
#include "system/AudioSystem.h"
#include "system/PhysicsSystem.h"
#include "system/SceneSystem.h"
#include "utils/MathUtils.h"
#include <btBulletDynamicsCommon.h>

Enemy::Enemy(Game* game)
    : CharacterActor(game),
      mLifeState(LifeState::Alive),
      mActionState(ActionState::Idle),
      mIsCountered(false),
      mIsBoss(false),
      mIsHit(false),
      mIsStrongAttacked(false),
      mIsJustBeforeAttack(false),
      mBreakCount(0),
      mBreakCountMax(0),
      mAttack(20.0f),
      mHp(10.0f),
      mMaxHp(10.0f),
      mDetectionRange(6.0f),
      mMoveSpeed(2.0f),
      mKnockBackSpeed(5.0f),
      mAttackSpeed(1.5f),
      mStandByAttackTimer(-1.0f),
      mDefaultStandByAttackTimer(-1.0f),
      mLaunchedTimer(-1.0f),
      mDefaultLaunchedTimer(-1.0f),
      mAttackMotionTimer(-1.0f),
      mDefaultAttackMotionTimer(-1.0f),
      mDyingTimer(-1.0f),
      mKnockBackTimer(-1.0f),
      mKnockBackFrom(0.0f),
      mNearestPlayer(nullptr),
      mCanCounteredTimer(-1.0f),
      mCanCountered(false)
{
}

void Enemy::UpdateActor(float deltaTime)
{
    CharacterActor::UpdateActor(deltaTime);

    if (!mGame->GetSceneSystem()->IsPlaying()) {
        return;
    }

    mNearestPlayer = mGame->FindNearestPlayer(this);

    switch (mLifeState) {
    case LifeState::Alive:
        UpdateAlive(deltaTime);
        break;

    case LifeState::Dying:
        UpdateDying(deltaTime);
        break;

    case LifeState::Dead:
        break;
    }
}

void Enemy::UpdateAlive(float deltaTime)
{
    if (mActionState == ActionState::KnockedBack) {
        UpdateKnockedBack(deltaTime);

        if (!mOnGround) {
            UpdateInAir(deltaTime);
        }

        return;
    }

    if (mOnGround) {
        UpdateBehavior(deltaTime);
        return;
    }

    UpdateInAir(deltaTime);
}

void Enemy::UpdateDying(float deltaTime)
{
    MoveDuringKnockBack(deltaTime);

    if (!mOnGround) {
        UpdateInAir(deltaTime);
    }

    mDyingTimer -= deltaTime;
    if (mDyingTimer <= 0.0f) {
        FinishDying();
    }
}

void Enemy::UpdateBehavior(float deltaTime)
{
    switch (mActionState) {
    case ActionState::Idle:
        UpdateIdle();
        break;

    case ActionState::Tracking:
        UpdateTracking(deltaTime);
        break;

    case ActionState::PreparingAttack:
        UpdatePreparingAttack(deltaTime);
        break;

    case ActionState::Attacking:
        UpdateAttacking(deltaTime);
        break;

    case ActionState::KnockedBack:
        UpdateKnockedBack(deltaTime);
        break;
    }
}

void Enemy::UpdateFacingVec()
{
    const glm::vec3 toPlayer = glm::normalize(mNearestPlayer->GetPos() - mPos);
    mFacingForwardVec = toPlayer;
    mFacingYaw = mGame->GetMathUtils()->GetYawFromDirection(mUpVec, toPlayer) + 3.14159265f;
}

void Enemy::UpdateIdle()
{
    if (IsPlayerInRange(mDetectionRange)) {
        StartTracking();
    }
}

void Enemy::UpdateTracking(float deltaTime)
{
    UpdateFacingVec();
    MoveToPlayer(deltaTime);
    TryStartPreparingAttack();
}

void Enemy::TryStartPreparingAttack()
{
    constexpr float attackStartRangeMargin = 1.5f;
    const float attackStartRange = mRadius + attackStartRangeMargin;

    if (IsPlayerInRange(attackStartRange)) {
        StartPreparingAttack();
    }
}

void Enemy::UpdatePreparingAttack(float deltaTime)
{
    if (!mIsJustBeforeAttack) {
        UpdateFacingVec();
    }

    mStandByAttackTimer -= deltaTime;

    if (IsJustBeforeAttack()) {
        mIsJustBeforeAttack = true;
        mGame->GetAudioSystem()->PlaySE("attack_pre_se");
    }

    if (mStandByAttackTimer <= 0.0f) {
        StartAttacking();
    }
}

void Enemy::UpdateAttacking(float deltaTime)
{
    MoveDuringAttacking(deltaTime);
    TryApplyAttack(deltaTime);

    mCanCounteredTimer -= deltaTime;
    if (mCanCounteredTimer <= 0.0f) {
        mCanCountered = false;
    }

    mAttackMotionTimer -= deltaTime;
    if (mAttackMotionTimer <= 0.0f) {
        StartIdle();
    }
}

void Enemy::TryApplyAttack(float deltaTime)
{
    constexpr float hitRangeMargin = 0.2f;
    const float hitRange = mRadius + hitRangeMargin;

    const bool canApplyDamage = IsPlayerInRange(hitRange) && !mIsHit && IsProgressing();
    if (canApplyDamage) {
        mNearestPlayer->ApplyDamage(this, deltaTime);
        mIsHit = true;
    }
}

void Enemy::UpdateKnockedBack(float deltaTime)
{
    MoveDuringKnockBack(deltaTime);

    mKnockBackTimer -= deltaTime;
    if (mKnockBackTimer <= 0.0f) {
        StartIdle();
    }
}

void Enemy::StartIdle()
{
    mActionState = ActionState::Idle;
}

void Enemy::StartTracking()
{
    mActionState = ActionState::Tracking;
}

void Enemy::StartPreparingAttack()
{
    mActionState = ActionState::PreparingAttack;
    mStandByAttackTimer = mDefaultStandByAttackTimer;
}

void Enemy::StartAttacking()
{
    mActionState = ActionState::Attacking;
    mAttackMotionTimer = mDefaultAttackMotionTimer;
    mIsHit = false;
    mIsJustBeforeAttack = false;
    mCanCounteredTimer = 0.1f;
    mCanCountered = true;
}

void Enemy::StartKnockedBack(float knockBackTimer)
{
    mActionState = ActionState::KnockedBack;
    mKnockBackTimer = knockBackTimer;
    mKnockBackFrom = glm::normalize(mPos - mNearestPlayer->GetPos());
    mLaunchedTimer = -1.0f;
}

void Enemy::StartDying()
{
    mLifeState = LifeState::Dying;
    mDyingTimer = 1.0f;
    mHp = 0;
    constexpr float dyingKnockBackTimer = 0.5f;
    StartKnockedBack(dyingKnockBackTimer);
    mGame->GetAudioSystem()->PlaySE("defeat_se");
}

void Enemy::FinishDying()
{
    mLifeState = LifeState::Dead;
    mIsActive = false;
    mCurrentPlanet->OnEnemyDead();

    if (mIsBoss) {
        Star* star = GetCurrentPlanet()->GetStar();
        if (!star) {
            return;
        }

        star->SetIsActive(true);
    }
}

void Enemy::FinishLaunched()
{
    mBreakCount = mBreakCountMax;
    mShouldJudgeLanding = true;
    mLaunchedTimer = -1.0f;
}

bool Enemy::IsPlayerInRange(float range) const
{
    const float distToPlayer = glm::length(mNearestPlayer->GetPos() - mPos);
    return distToPlayer <= range;
}

bool Enemy::IsJustBeforeAttack() const
{
    if (mIsJustBeforeAttack) {
        return false;
    }

    constexpr float justBeforeAttackTime = 1.0f;
    return mStandByAttackTimer <= justBeforeAttackTime;
}

void Enemy::MoveToPlayer(float deltaTime)
{
    const glm::vec3 moveDelta = mFacingForwardVec * mMoveSpeed * deltaTime;
    mPos = CalculateCollisionAdjustedPos(moveDelta);
}

void Enemy::MoveDuringAttacking(float deltaTime)
{
    glm::vec3 moveDelta;
    if (IsProgressing()) {
        moveDelta = mFacingForwardVec * mAttackSpeed * deltaTime;
    } else {
        moveDelta = -mFacingForwardVec * mAttackSpeed * deltaTime;
    }

    mPos = CalculateCollisionAdjustedPos(moveDelta);
}

void Enemy::MoveDuringKnockBack(float deltaTime)
{
    const glm::vec3 moveDelta = mKnockBackFrom * mKnockBackSpeed * deltaTime;
    mPos = CalculateCollisionAdjustedPos(moveDelta);
}

void Enemy::UpdateInAir(float deltaTime)
{
    if (mLaunchedTimer >= 0.0f) {
        mLaunchedTimer -= deltaTime;

        if (mLaunchedTimer >= 0.0f) {
            return;
        }

        FinishLaunched();
        return;
    }

    glm::vec3 prevVelocity = mVelocity;
    ApplyGravity(deltaTime);

    const float vPrev = dot(prevVelocity, mUpVec);
    const float vNow = dot(mVelocity, mUpVec);

    // 頂点で固定開始
    const bool isTop = vPrev > 0.0f && vNow <= 0.0f;
    if (isTop) {
        mLaunchedTimer = mDefaultLaunchedTimer;
    }
}

void Enemy::ApplyCounter(Player* player)
{
    constexpr float knockBackTimer = 0.6f;
    StartKnockedBack(knockBackTimer);

    mIsCountered = false;
    mHp -= player->GetAttack() * 2.0f;
    mStandByAttackTimer = -1.0f;

    if (IsHp0()) {
        StartDying();
    }
}

void Enemy::LaunchIntoAir(float deltaTime)
{
    mGame->OnEnemyLaunched();

    constexpr float launchSpeed = 5.0f;
    mVelocity += mUpVec * launchSpeed;
    mPos += mVelocity * deltaTime;

    mOnGround = false;
    mStandByAttackTimer = -1.0f;
    mAttackMotionTimer = -1.0f;
    mShouldJudgeLanding = false;
    mGame->SetHitStopTimer(0.3f);

    StartIdle();
}

void Enemy::ApplyDamage(float damage, Player* player)
{
    if (!IsAlive()) {
        return;
    }

    mHp -= damage;

    if (IsHp0()) {
        StartDying();
    }

    if (mIsStrongAttacked) {
        constexpr float knockBackTimer = 0.5f;
        StartKnockedBack(knockBackTimer);

        mIsStrongAttacked = false;
        FinishLaunched();
    } else if (!mIsBoss && mOnGround) {
        constexpr float knockBackTimer = 0.04f;
        StartKnockedBack(knockBackTimer);
    }
}

void Enemy::ApplyBreak(float deltaTime, bool isAllBreak)
{
    if (!IsAlive()) {
        return;
    }

    if (isAllBreak) {
        while (mBreakCount) {
            mBreakCount--;
        }
    } else {
        mBreakCount--;
    }
    mGame->GetAudioSystem()->PlaySE("destroy_se");

    if (mBreakCount <= 0) {
        LaunchIntoAir(deltaTime);
        return;
    }
}