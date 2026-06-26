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
#include <yaml-cpp/yaml.h>

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

namespace {
float ReadFloat(const YAML::Node& node, const char* key, float defaultValue)
{
    return node[key] ? node[key].as<float>() : defaultValue;
}

int ReadInt(const YAML::Node& node, const char* key, int defaultValue)
{
    return node[key] ? node[key].as<int>() : defaultValue;
}

std::string ReadString(const YAML::Node& node, const char* key, const std::string& defaultValue)
{
    return node[key] ? node[key].as<std::string>() : defaultValue;
}
} // namespace

void Enemy::ApplyConfig(const std::string& type)
{
    SetIsBoss(type == "boss");

    YAML::Node enemyRoot = YAML::LoadFile("../assets/data/actor/enemies.yaml");

    if (!enemyRoot["enemies"] || !enemyRoot["enemies"].IsSequence()) {
        return;
    }

    for (const YAML::Node& enemyNode : enemyRoot["enemies"]) {
        const std::string enemyType = ReadString(enemyNode, "type", "");

        if (enemyType == "common") {
            SetKnockBackSpeed(ReadFloat(enemyNode, "knockBackSpeed", 0.0f));
            SetDefaultLaunchedTimer(ReadFloat(enemyNode, "defaultLaunchedTimer", 0.0f));
            SetDetectionRange(ReadFloat(enemyNode, "detectionRange", 0.0f));
            continue;
        }

        if (type != enemyType) {
            continue;
        }

        const float hp = ReadFloat(enemyNode, "hp", 80.0f);
        SetHp(hp);
        SetMaxHp(hp);

        const float scale = ReadFloat(enemyNode, "scale", 0.25f);
        SetScale(glm::vec3(scale));

        SetMoveSpeed(ReadFloat(enemyNode, "speed", 1.0f));
        SetAttack(ReadFloat(enemyNode, "attack", 5.0f));
        SetRadius(ReadFloat(enemyNode, "radius", 0.75f));

        const int breakCountMax = ReadInt(enemyNode, "breakCountMax", 1);
        SetBreakCountMax(breakCountMax);
        SetBreakCount(breakCountMax);

        SetModelPath(ReadString(enemyNode, "modelPath", ""));

        SetDefaultStandByAttackTimer(ReadFloat(enemyNode, "defaultStandByAttackTimer", 0.0f));
        SetDefaultAttackMotionTimer(ReadFloat(enemyNode, "defaultAttackMotionTimer", 0.0f));
        SetAttackSpeed(ReadFloat(enemyNode, "attackSpeed", 0.0f));
    }
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

void Enemy::UpdateFacingVec(float deltaTime)
{
    const glm::vec3 toPlayer = glm::normalize(mNearestPlayer->GetPos() - mPos);
    constexpr float turnSpeed = 5.0f;
    const float t = 1.0f - std::exp(-turnSpeed * deltaTime);

    mFacingForwardVec = glm::normalize(glm::mix(mFacingForwardVec, toPlayer, t));
    mFacingYaw = mGame->GetMathUtils()->GetYawFromDirection(mUpVec, mFacingForwardVec) + 3.14159265f;
}

void Enemy::UpdateIdle()
{
    if (IsPlayerInRange(mNearestPlayer, mDetectionRange)) {
        StartTracking();
    }
}

void Enemy::UpdateTracking(float deltaTime)
{
    UpdateFacingVec(deltaTime);
    MoveToPlayer(deltaTime);
    TryStartPreparingAttack();
}

void Enemy::TryStartPreparingAttack()
{
    constexpr float attackStartRangeMargin = 1.5f;
    const float attackStartRange = mRadius + attackStartRangeMargin;

    if (IsPlayerInRange(mNearestPlayer, attackStartRange)) {
        StartPreparingAttack();
    }
}

void Enemy::UpdatePreparingAttack(float deltaTime)
{
    if (!mIsJustBeforeAttack) {
        UpdateFacingVec(deltaTime);
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
    if (!IsProgressing()) {
        return;
    }

    constexpr float hitRangeMargin = 0.2f;
    const float hitRange = mRadius + hitRangeMargin;

    for (Player* player : mGame->GetPlayers()) {
        if (!player) {
            continue;
        }

        if (!player->IsAlive()) {
            continue;
        }

        if (mHitPlayers.contains(player)) {
            continue;
        }

        const bool isInRange = IsPlayerInRange(player, hitRange);

        if (!isInRange) {
            continue;
        }

        player->ApplyDamage(this, deltaTime);
        mHitPlayers.insert(player);
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
    mHitPlayers.clear();
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

bool Enemy::IsPlayerInRange(Player* player, float range) const
{
    const float distToPlayer = glm::length(player->GetPos() - mPos);
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

glm::vec3 Enemy::CalculateCollisionAdjustedPos(const glm::vec3& moveDelta)
{
    glm::vec3 desiredPos = mPos + moveDelta;

    desiredPos = mGame->GetPhysicsSystem()->CheckCollision(this, moveDelta, desiredPos);

    if (IsAlive()) {
        desiredPos = ClampMoveToGround(desiredPos);
    }

    return desiredPos;
}

glm::vec3 Enemy::ClampMoveToGround(const glm::vec3& desiredPos) const
{
    const glm::vec3 move = desiredPos - mPos;
    const float moveLength = glm::length(move);

    if (moveLength < 1e-6f) {
        return desiredPos;
    }

    constexpr float checkStep = 0.25f;
    const int checkCount = std::max(1, static_cast<int>(std::ceil(moveLength / checkStep)));

    glm::vec3 lastSafePos = mPos;

    for (int i = 1; i <= checkCount; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(checkCount);
        const glm::vec3 checkPos = glm::mix(mPos, desiredPos, t);

        if (!HasGroundBelow(checkPos)) {
            return lastSafePos;
        }

        lastSafePos = checkPos;
    }

    return desiredPos;
}

bool Enemy::HasGroundBelow(const glm::vec3& checkPos) const
{
    if (!mGame || !mGame->GetPhysicsSystem()) {
        return true;
    }

    btDiscreteDynamicsWorld* bulletWorld = mGame->GetPhysicsSystem()->GetBulletWorld();
    if (!bulletWorld) {
        return true;
    }

    if (glm::length(mUpVec) < 1e-6f) {
        return true;
    }

    const glm::vec3 up = glm::normalize(mUpVec);

    constexpr float rayStartOffset = 0.3f;
    constexpr float rayLength = 1.2f;

    const glm::vec3 rayFromPos = checkPos + up * rayStartOffset;
    const glm::vec3 rayToPos = checkPos - up * rayLength;

    const btVector3 rayFrom(rayFromPos.x, rayFromPos.y, rayFromPos.z);
    const btVector3 rayTo(rayToPos.x, rayToPos.y, rayToPos.z);

    btCollisionWorld::ClosestRayResultCallback rayCallback(rayFrom, rayTo);
    bulletWorld->rayTest(rayFrom, rayTo, rayCallback);

    if (!rayCallback.hasHit()) {
        return false;
    }

    const btVector3 hitNormalBt = rayCallback.m_hitNormalWorld;
    glm::vec3 hitNormal(hitNormalBt.x(), hitNormalBt.y(), hitNormalBt.z());

    if (glm::length(hitNormal) < 1e-6f) {
        return false;
    }

    hitNormal = glm::normalize(hitNormal);

    if (CheckDotAngleSteep(hitNormal, up)) {
        return false;
    }

    return true;
}