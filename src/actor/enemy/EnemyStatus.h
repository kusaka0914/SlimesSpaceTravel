#pragma once

#include "actor/enemy/EnemyBreakGauge.h"
#include "actor/enemy/EnemyHealth.h"

#include <glm/glm.hpp>
#include <unordered_set>

class Player;

class EnemyStatus {
public:
    EnemyStatus();

    void SetIsBoss(bool isBoss) { mIsBoss = isBoss; }
    void SetIsStrongAttacked(bool isStrongAttacked) { mIsStrongAttacked = isStrongAttacked; }
    void ClearStrongAttacked() { mIsStrongAttacked = false; }

    void SetBreakCount(int breakCount) { mBreakGauge.SetCount(breakCount); }
    void SetBreakCountMax(int breakCountMax) { mBreakGauge.SetMax(breakCountMax); }
    void ResetBreakCount() { mBreakGauge.Reset(); }
    void DecrementBreakCount() { mBreakGauge.Decrease(); }
    void BreakAll() { mBreakGauge.BreakAll(); }

    void SetHp(float hp) { mHealth.SetHp(hp); }
    void SetMaxHp(float maxHp) { mHealth.SetMaxHp(maxHp); }
    void AddDamage(float damage) { mHealth.AddDamage(damage); }
    void SetHpZero() { mHealth.SetHpZero(); }

    void SetDefaultLaunchedTimer(float defaultLaunchedTimer) { mDefaultLaunchedTimer = defaultLaunchedTimer; }
    void SetLaunchHeight(float launchHeight) { mLaunchHeight = launchHeight; }
    void SetMoveSpeed(float moveSpeed) { mMoveSpeed = moveSpeed; }
    void SetAttack(float attack) { mAttack = attack; }
    void SetDefaultAttackMotionTimer(float defaultAttackMotionTimer) { mDefaultAttackMotionTimer = defaultAttackMotionTimer; }
    void SetDefaultStandByAttackTimer(float defaultStandByAttackTimer)
    {
        mDefaultStandByAttackTimer = defaultStandByAttackTimer;
    }
    void SetDetectionRange(float detectionRange) { mDetectionRange = detectionRange; }
    void SetAttackPreparationRange(float attackPreparationRange)
    {
        mAttackPreparationRange = attackPreparationRange;
    }
    void SetKnockBackSpeed(float knockBackSpeed) { mKnockBackSpeed = knockBackSpeed; }
    void SetAttackSpeed(float attackSpeed) { mAttackSpeed = attackSpeed; }

    void FlipCanCountered() { mCanCountered = !mCanCountered; }
    void SetCanCountered(bool canCountered) { mCanCountered = canCountered; }
    void SetCanCounteredTimer(float timer) { mCanCounteredTimer = timer; }
    void DecreaseCanCounteredTimer(float deltaTime) { mCanCounteredTimer -= deltaTime; }

    void SetStandByAttackTimer(float timer) { mStandByAttackTimer = timer; }
    void DecreaseStandByAttackTimer(float deltaTime) { mStandByAttackTimer -= deltaTime; }
    void ResetStandByAttackTimer() { mStandByAttackTimer = mDefaultStandByAttackTimer; }

    void SetAttackMotionTimer(float timer) { mAttackMotionTimer = timer; }
    void DecreaseAttackMotionTimer(float deltaTime) { mAttackMotionTimer -= deltaTime; }
    void ResetAttackMotionTimer() { mAttackMotionTimer = mDefaultAttackMotionTimer; }

    void SetDyingTimer(float dyingTimer) { mDyingTimer = dyingTimer; }
    void DecreaseDyingTimer(float deltaTime) { mDyingTimer -= deltaTime; }

    void SetKnockBackTimer(float knockBackTimer) { mKnockBackTimer = knockBackTimer; }
    void DecreaseKnockBackTimer(float deltaTime) { mKnockBackTimer -= deltaTime; }

    void SetLaunchedTimer(float launchedTimer) { mLaunchedTimer = launchedTimer; }
    void DecreaseLaunchedTimer(float deltaTime) { mLaunchedTimer -= deltaTime; }
    void ClearLaunchedTimer() { mLaunchedTimer = -1.0f; }

    void SetKnockBackFrom(const glm::vec3& knockBackFrom) { mKnockBackFrom = knockBackFrom; }

    void SetNearestPlayer(Player* player) { mNearestPlayer = player; }

    void SetIsJustBeforeAttack(bool isJustBeforeAttack) { mIsJustBeforeAttack = isJustBeforeAttack; }
    void ClearIsHit() { mIsHit = false; }
    void ClearIsCountered() { mIsCountered = false; }

    void ClearHitPlayers() { mHitPlayers.clear(); }
    bool HasHitAnyPlayer() const { return !mHitPlayers.empty(); }
    bool HasHitPlayer(Player* player) const { return mHitPlayers.contains(player); }
    void AddHitPlayer(Player* player) { mHitPlayers.insert(player); }

    bool GetIsBoss() const { return mIsBoss; }
    bool GetCanCountered() const { return mCanCountered; }
    bool GetIsStrongAttacked() const { return mIsStrongAttacked; }
    bool GetIsJustBeforeAttack() const { return mIsJustBeforeAttack; }

    int GetBreakCount() const { return mBreakGauge.GetCount(); }
    int GetBreakCountMax() const { return mBreakGauge.GetMax(); }

    float GetHp() const { return mHealth.GetHp(); }
    float GetMaxHp() const { return mHealth.GetMaxHp(); }
    float GetAttack() const { return mAttack; }
    float GetAttackRange() const { return mAttackSpeed * (mDefaultAttackMotionTimer / 2.0f); }
    float GetStandByAttackTimer() const { return mStandByAttackTimer; }
    float GetDetectionRange() const { return mDetectionRange; }
    float GetAttackPreparationRange() const { return mAttackPreparationRange; }
    float GetMoveSpeed() const { return mMoveSpeed; }
    float GetKnockBackSpeed() const { return mKnockBackSpeed; }
    float GetAttackSpeed() const { return mAttackSpeed; }
    float GetDefaultStandByAttackTimer() const { return mDefaultStandByAttackTimer; }
    float GetDefaultLaunchedTimer() const { return mDefaultLaunchedTimer; }
    float GetLaunchHeight() const { return mLaunchHeight; }
    float GetDefaultAttackMotionTimer() const { return mDefaultAttackMotionTimer; }
    float GetAttackMotionTimer() const { return mAttackMotionTimer; }
    float GetDyingTimer() const { return mDyingTimer; }
    float GetKnockBackTimer() const { return mKnockBackTimer; }
    float GetLaunchedTimer() const { return mLaunchedTimer; }
    float GetCanCounteredTimer() const { return mCanCounteredTimer; }

    const glm::vec3& GetKnockBackFrom() const { return mKnockBackFrom; }
    Player* GetNearestPlayer() const { return mNearestPlayer; }

    EnemyHealth& GetHealth() { return mHealth; }
    const EnemyHealth& GetHealth() const { return mHealth; }
    EnemyBreakGauge& GetBreakGauge() { return mBreakGauge; }
    const EnemyBreakGauge& GetBreakGauge() const { return mBreakGauge; }

    bool IsHp0() const { return mHealth.IsDead(); }
    bool IsBreakCountEmpty() const { return mBreakGauge.IsEmpty(); }

private:
    EnemyHealth mHealth;
    EnemyBreakGauge mBreakGauge;

    bool mIsCountered;
    bool mIsBoss;
    bool mIsHit;
    bool mIsStrongAttacked;
    bool mIsJustBeforeAttack;
    bool mCanCountered;

    float mAttack;
    float mDetectionRange;
    float mAttackPreparationRange;
    float mMoveSpeed;
    float mKnockBackSpeed;
    float mAttackSpeed;

    float mStandByAttackTimer;
    float mDefaultStandByAttackTimer;
    float mLaunchedTimer;
    float mDefaultLaunchedTimer;
    float mLaunchHeight;
    float mAttackMotionTimer;
    float mDefaultAttackMotionTimer;
    float mDyingTimer;
    float mKnockBackTimer;
    float mCanCounteredTimer;

    glm::vec3 mKnockBackFrom;

    Player* mNearestPlayer;
    std::unordered_set<Player*> mHitPlayers;
};
