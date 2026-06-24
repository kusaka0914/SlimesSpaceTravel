#pragma once

#include "CharacterActor.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

class Game;
class Player;

class Enemy : public CharacterActor {
public:
    enum class LifeState { Alive, Dying, Dead };

    enum class ActionState {
        Idle,
        Tracking,
        PreparingAttack,
        Attacking,
        KnockedBack,
    };

    Enemy(Game* game);
    void UpdateActor(float deltaTime) override;

    void ApplyDamage(float damage, Player* player);
    void ApplyBreak(float deltaTim, bool isAllBrea = false);

    void SetIsBoss(bool isBoss) { mIsBoss = isBoss; }
    void SetIsStrongAttacked(bool isStrongAttacked) { mIsStrongAttacked = isStrongAttacked; }

    void SetBreakCount(int breakCount) { mBreakCount = breakCount; }
    void SetBreakCountMax(int breakCountMax) { mBreakCountMax = breakCountMax; }

    void SetHp(float hp) { mHp = hp; }
    void SetMaxHp(float maxHp) { mMaxHp = maxHp; }
    void SetDefaultLaunchedTimer(float defaultLaunchedTimer) { mDefaultLaunchedTimer = defaultLaunchedTimer; }
    void SetMoveSpeed(float moveSpeed) { mMoveSpeed = moveSpeed; }
    void SetAttack(float attack) { mAttack = attack; }
    void SetDefaultAttackMotionTimer(float defaultAttackMotionTimer)
    {
        mDefaultAttackMotionTimer = defaultAttackMotionTimer;
    }
    void SetDefaultStandByAttackTimer(float defaultStandByAttackTimer)
    {
        mDefaultStandByAttackTimer = defaultStandByAttackTimer;
    }
    void SetDetectionRange(float detectionRange) { mDetectionRange = detectionRange; }
    void SetKnockBackSpeed(float knockBackSpeed) { mKnockBackSpeed = knockBackSpeed; }
    void SetAttackSpeed(float attackSpeed) { mAttackSpeed = attackSpeed; }
    void FlipCanCountered() { mCanCountered = !mCanCountered; }

    bool GetIsDead() const { return mLifeState == LifeState::Dead; }
    bool GetIsBoss() const { return mIsBoss; }
    bool GetCanCountered() const { return mCanCountered; }

    int GetBreakCount() const { return mBreakCount; }

    float GetHp() const { return mHp; }
    float GetMaxHp() const { return mMaxHp; }
    float GetAttack() const { return mAttack; }
    float GetAttackRange() const { return mAttackSpeed * (mDefaultAttackMotionTimer / 2); }
    float GetStandByAttackTimer() const { return mStandByAttackTimer; }

    int GetBreakCountMax() const { return mBreakCountMax; }

    float GetDetectionRange() const { return mDetectionRange; }
    float GetMoveSpeed() const { return mMoveSpeed; }
    float GetKnockBackSpeed() const { return mKnockBackSpeed; }
    float GetAttackSpeed() const { return mAttackSpeed; }

    float GetDefaultStandByAttackTimer() const { return mDefaultStandByAttackTimer; }
    float GetDefaultLaunchedTimer() const { return mDefaultLaunchedTimer; }
    float GetDefaultAttackMotionTimer() const { return mDefaultAttackMotionTimer; }

private:
    void UpdateAlive(float deltaTime);
    void UpdateDying(float deltaTime);

    void UpdateBehavior(float deltaTime);
    void UpdateFacingVec(float deltaTime);
    void UpdateIdle();
    void UpdateTracking(float deltaTime);
    void UpdatePreparingAttack(float deltaTime);
    void UpdateAttacking(float deltaTime);
    void UpdateKnockedBack(float deltaTime);

    void StartIdle();
    void StartTracking();
    void TryStartPreparingAttack();
    void StartPreparingAttack();
    void TryApplyAttack(float deltaTime);
    void StartAttacking();
    void StartKnockedBack(float knockBackTimer);
    void StartDying();

    void FinishLaunched();
    void FinishDying();

    bool IsPlayerInRange(Player* player, float range) const;
    bool IsJustBeforeAttack() const;
    bool IsProgressing() const { return mAttackMotionTimer >= mDefaultAttackMotionTimer / 2; }
    bool IsHp0() const { return mHp <= 0.0f; }
    bool IsAlive() const { return mLifeState == LifeState::Alive; }

    void MoveToPlayer(float deltaTime);
    void MoveDuringAttacking(float deltaTime);
    void MoveDuringKnockBack(float deltaTime);
    void LaunchIntoAir(float deltaTime);
    void ApplyCounter(Player* player);
    void UpdateInAir(float deltaTime);
    glm::vec3 ClampMoveToGround(const glm::vec3& desiredPos) const;
    bool HasGroundBelow(const glm::vec3& checkPos) const;
    glm::vec3 CalculateCollisionAdjustedPos(const glm::vec3& moveDelta);

private:
    LifeState mLifeState;
    ActionState mActionState;

    bool mIsCountered;
    bool mIsBoss;
    bool mIsHit;
    bool mIsStrongAttacked;
    bool mIsJustBeforeAttack;
    bool mCanCountered;

    int mBreakCount;
    int mBreakCountMax;

    float mAttack;
    float mHp;
    float mMaxHp;
    float mDetectionRange;
    float mMoveSpeed;
    float mKnockBackSpeed;
    float mAttackSpeed;

    float mStandByAttackTimer;
    float mDefaultStandByAttackTimer;
    float mLaunchedTimer;
    float mDefaultLaunchedTimer;
    float mAttackMotionTimer;
    float mDefaultAttackMotionTimer;
    float mDyingTimer;
    float mKnockBackTimer;
    float mCanCounteredTimer;

    glm::vec3 mKnockBackFrom;

    Player* mNearestPlayer;
    std::unordered_set<Player*> mHitPlayers;
};