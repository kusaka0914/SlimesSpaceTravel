#pragma once

#include "actor/CharacterActor.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

class Game;
class NPC;
class Boat;
class Enemy;

class Player : public CharacterActor {
public:
    enum class ActionState {
        Idle,
        Dodging,
        Attacking,
        Charging,
        StrongAttacking,
        KnockedBack,
    };

    enum class AttackKind { Normal, Wide, Strong };

    struct RaySegment {
        glm::vec3 from;
        glm::vec3 to;
    };

    Player(Game* game);

    void ApplyConfig();

    void Initialize() override;
    void ProcessActor() override;
    void UpdateActor(float deltaTime) override;

    void ApplyDamage(Enemy* enemy, float deltaTime);
    void OnBoatArrived(Boat* boat);
    void Restart();
    bool IsInvincible() const { return mInvincibleTimer > 0.0f; };
    bool IsAlive() const { return mHp > 0.0f; };
    bool IsAttacking() const
    {
        return mActionState == ActionState::Attacking || mActionState == ActionState::StrongAttacking ||
               mContinuousAttackingTimer >= 0.0f || mSpecialChargingTimer >= 0.0f || mAirAttackFloatingTimer >= 0.0f;
    }

    void SetIsDodged(bool isDodged) { mIsDodged = isDodged; }

    void SetCurrentPlanetNum(int currentPlanetNum) { mCurrentPlanetNum = currentPlanetNum; }
    void SetPlayerNum(int playerNum) { mPlayerNum = playerNum; }

    void SetCameraYaw(float cameraYaw) { mCameraYaw = cameraYaw; }
    void SetAttack(float attack) { mAttack = attack; }
    void SetMoveSpeed(float moveSpeed) { mMoveSpeed = moveSpeed; }
    void SetAttackSpeed(float attackSpeed) { mAttackSpeed = attackSpeed; }
    void SetChargeMoveSpeed(float chargeMoveSpeed) { mChargeMoveSpeed = chargeMoveSpeed; }
    void SetHp(float hp) { mHp = hp; }
    void SetMaxHp(float maxHp) { mMaxHp = maxHp; }
    void SetDefaultDamageTimer(float defaultDamageTimer) { mDefaultDamageTimer = defaultDamageTimer; }
    void SetAttackCooldown(float attackCooldown) { mAttackCooldown = attackCooldown; }
    void SetLastAttackCooldown(float lastAttackCooldown) { mLastAttackCooldown = lastAttackCooldown; }
    void SetDefaultAttackPressTimer(float defaultAttackPressTimer)
    {
        mDefaultAttackPressTimer = defaultAttackPressTimer;
    }
    void SetSpecialAttackCooldown(float specialAttackCooldown) { mSpecialAttackCooldown = specialAttackCooldown; }
    void SetDodgeDuration(float dodgeDuration) { mDodgeDuration = dodgeDuration; }
    void SetDefaultInvincibleTimer(float defaultInvincibleTimer) { mDefaultInvincibleTimer = defaultInvincibleTimer; }
    void SetDodgeCooldownTime(float dodgeCooldownTime) { mDodgeCooldownTime = dodgeCooldownTime; }
    void SetDodgeDistance(float dodgeDistance) { mDodgeDistance = dodgeDistance; }
    void SetNormalAttackRange(float normalAttackRange) { mNormalAttackRange = normalAttackRange; }
    void SetNormalAttackAngle(float normalAttackAngle) { mNormalAttackAngle = normalAttackAngle; }
    void SetNormalAttack(float normalAttack) { mNormalAttack = normalAttack; }
    void SetWideAttackRange(float wideAttackRange) { mWideAttackRange = wideAttackRange; }
    void SetWideAttackAngle(float wideAttackAngle) { mWideAttackAngle = wideAttackAngle; }
    void SetWideAttack(float wideAttack) { mWideAttack = wideAttack; }
    void SetStrongAttackRange(float strongAttackRange) { mStrongAttackRange = strongAttackRange; }
    void SetStrongAttack(float strongAttack) { mStrongAttack = strongAttack; }
    void SetStrongAttackSpeed(float strongAttackSpeed) { mStrongAttackSpeed = strongAttackSpeed; }
    void SetDefaultStrongAttackTimer(float defaultStrongAttackTimer)
    {
        mDefaultStrongAttackTimer = defaultStrongAttackTimer;
    }
    void SetDefaultAttackMotionTimer(float defaultAttackMotionTimer)
    {
        mDefaultAttackMotionTimer = defaultAttackMotionTimer;
    }
    void SetRayCastTimer(float rayCastTimer) { mRayCastTimer = rayCastTimer; }
    void SetInputAvailableTimer(float inputAvailableTimer) { mInputAvailableTimer = inputAvailableTimer; }
    void SetKnockBackSpeed(float knockBackSpeed) { mKnockBackSpeed = knockBackSpeed; }

    void SetVelocity(const glm::vec3& velocity) { mVelocity = velocity; }

    void SetTalkableNPC(NPC* talkableNPC) { mTalkableNPC = talkableNPC; }

    bool GetIsStrongAttacked() const { return mIsStrongAttacked; }
    bool GetIsSpecialAttackPressed() const { return mSpecialAttackPressed; }
    bool GetCanSpecialAttack() const { return mCanSpecialAttack; }
    bool GetIsTired() const { return mIsTired; }

    int GetCurrentPlanetNum() const { return mCurrentPlanetNum; }
    int GetJewelCount() const { return mJewelCount; }
    int GetPlayerNum() const { return mPlayerNum; }

    float GetAttack() const { return mAttack; }
    float GetHp() const { return mHp; }
    float GetAttackMotionTimer() const { return mAttackMotionTimer; }
    float GetStrongAttackTimer() const { return mStrongAttackTimer; }
    float GetInvincibleTimer() const { return mInvincibleTimer; }
    float GetSpecialAttackCooldownRemaining() const { return mJewelTimer; }
    float GetAttackRange() const { return mAttackRange; }
    float GetAttackAngle() const { return mAttackAngle; }
    float GetRayCastTimer() const { return mRayCastTimer; }
    float GetMoveSpeed() const { return mMoveSpeed; }
    float GetCameraYaw() const { return mCameraYaw; }

    float GetAttackSpeed() const { return mAttackSpeed; }
    float GetChargeMoveSpeed() const { return mChargeMoveSpeed; }

    float GetMaxHp() const { return mMaxHp; }

    float GetDefaultDamageTimer() const { return mDefaultDamageTimer; }

    float GetAttackCooldown() const { return mAttackCooldown; }
    float GetLastAttackCooldown() const { return mLastAttackCooldown; }
    float GetDefaultAttackPressTimer() const { return mDefaultAttackPressTimer; }

    float GetSpecialAttackCooldown() const { return mSpecialAttackCooldown; }

    float GetDodgeDuration() const { return mDodgeDuration; }
    float GetDodgeCooldownTime() const { return mDodgeCooldownTime; }
    float GetDodgeDistance() const { return mDodgeDistance; }

    float GetDefaultInvincibleTimer() const { return mDefaultInvincibleTimer; }

    float GetNormalAttackRange() const { return mNormalAttackRange; }
    float GetNormalAttackAngle() const { return mNormalAttackAngle; }
    float GetNormalAttack() const { return mNormalAttack; }

    float GetWideAttackRange() const { return mWideAttackRange; }
    float GetWideAttackAngle() const { return mWideAttackAngle; }
    float GetWideAttack() const { return mWideAttack; }

    float GetStrongAttackRange() const { return mStrongAttackRange; }
    float GetStrongAttack() const { return mStrongAttack; }
    float GetStrongAttackSpeed() const { return mStrongAttackSpeed; }

    float GetDefaultStrongAttackTimer() const { return mDefaultStrongAttackTimer; }
    float GetDefaultAttackMotionTimer() const { return mDefaultAttackMotionTimer; }

    float GetInputAvailableTimer() const { return mInputAvailableTimer; }
    float GetKnockBackSpeed() const { return mKnockBackSpeed; }

    ActionState GetActionState() const { return mActionState; }

    const glm::vec3& GetForwardVec() const { return mForwardVec; }
    const std::vector<RaySegment>& GetRayCasts() const { return mRayCasts; }

    NPC* GetTalkableNPC() const { return mTalkableNPC; }

private:
    void ProcessGameController();
    void ProcessKeyboard();

    void UpdateAlive(float deltaTime);
    void Die();
    void Recover();
    void Respawn();

    void UpdateWorldVec();
    void UpdateIdle(float deltaTime);
    void UpdateDodging(float deltaTime);
    void UpdateAttacking(float deltaTime);
    void UpdateCharging(float deltaTime);
    void UpdateStrongAttacking(float deltaTime);
    void UpdateKnockedBack(float deltaTime);
    void UpdateSpecialAttackCharging(float deltaTime);
    void UpdateContinuousAttacking(float deltaTime);
    void UpdateTimer(float deltaTime);

    void UpdateJewelTimer(float deltaTime);
    void UpdateComboKeepTimer(float deltaTime);
    void UpdateWalk(float deltaTime);
    void UpdateBoatRide();

    void StartIdle();
    void StartDodging();
    void StartAttacking(float deltaTime);
    void StartCharging(float deltaTime);
    void StartStrongAttacking(float deltaTime);
    void StartRidingBoat(Boat* boat);
    void StartJumping(float deltaTime);
    void StartJewelTimer();
    void StartTired(float lockTime);

    void FinishCharging();
    void FinishSpecialAttackCharging();

    void ChangeFaceDir();
    void UpdateFacingForwardVec();
    void MoveDuringDodging(float deltaTime);
    void MoveDuringAttacking(float deltaTime);
    void MoveDuringCharging(float deltaTime);
    void MoveDuringStrongAttacking(float deltaTime);
    void Attack(float deltaTime);
    void StartAfterAttackReaction();
    void WideAttack(float deltaTime);
    void StrongAttack(float deltaTime);
    void SpecialAttack(float deltaTime);
    void MoveDuringKnockBack(float deltaTime);
    void StartSpecialAttackCharging();
    void StartContinuousAttacking();
    void FollowMovingBoat(Boat* boat);
    void OnLanded() override;
    void ReduceTired();

    bool IsTouchingBoat(Boat* boat);
    bool IsFallIntoPlanetInside();
    bool IsEnemyHitByAttack(float dist, float dot, float effectiveRange);
    bool CanWalk() const { return mAttackMoveLockRemaining <= 0.0f && !mIsAirAttacking; }
    std::vector<Enemy*> FindHitEnemies();
    void OnUpVecUpdateFailed() override;
    void OnCastSucceeded() override;
    void SnapToGround(float upOffset, float downLength);

private:
    ActionState mActionState;
    AttackKind mAttackKind;

    bool mDodgePressed;
    bool mDodgePressedPrev;
    bool mJumpPressed;
    bool mAttackPressed;
    bool mAttackPressedPrev;
    bool mWideAttackPressed;
    bool mWideAttackPressedPrev;
    bool mSpecialAttackPressed;
    bool mSpecialAttackPressedPrev;
    bool mRecoverPressed;
    bool mRecoverPressedPrev;
    bool mIsDodged;
    bool mIsStrongAttackHit;
    bool mIsStrongAttacked;
    bool mIsCharged;
    bool mCanSpecialAttack;
    bool mIsAirAttacking;
    bool mIsTired;

    int mCurrentPlanetNum;
    int mAttackComboIndex;
    int mRestartPlanetIndex;
    int mPlayerNum;
    int mJewelCount;

    float mCameraYaw;
    float mMoveForward;
    float mMoveLeft;
    float mAttackStartHeight;
    float mDodgeTimer;
    float mDodgeDuration;
    float mDodgeCooldown;
    float mDodgeCooldownTime;
    float mDodgeDistance;
    float mDodgeStartHeight;
    float mMoveSpeed;
    float mChargeMoveSpeed;
    float mCameraStickX;
    float mCameraStickY;
    float mAttack;
    float mAttackSpeed;
    float mHp;
    float mMaxHp;
    float mDamageTimer;
    float mDefaultDamageTimer;
    float mAttackCooldownRemaining;
    float mAttackCooldown;
    float mLastAttackCooldown;
    float mAttackMoveLockRemaining;
    float mAttackDodgeLockRemaining;
    float mAttackMotionTimer;
    float mDefaultAttackMotionTimer;
    float mAirAttackFloatingTimer;
    float mJewelTimer;
    float mSpecialAttackCooldown;
    float mAttackPressTimer;
    float mDefaultAttackPressTimer;
    float mStrongAttackTimer;
    float mDefaultStrongAttackTimer;
    float mComboKeepTimer;
    float mInvincibleTimer;
    float mDefaultInvincibleTimer;
    float mAttackRange;
    float mAttackAngle;
    float mNormalAttackRange;
    float mNormalAttackAngle;
    float mNormalAttack;
    float mWideAttackRange;
    float mWideAttackAngle;
    float mWideAttack;
    float mStrongAttackRange;
    float mStrongAttack;
    float mStrongAttackSpeed;
    float mRayCastTimer;
    float mInputAvailableTimer;
    float mKnockBackSpeed;
    float mSpecialChargingTimer;
    float mContinuousAttackingTimer;
    float mContinuousAttackingCooldown;

    glm::vec3 mForwardVec;
    glm::vec3 mLeftVec;
    glm::vec3 mKnockBackFrom;
    glm::vec3 mRestartPos;
    glm::vec3 mDodgeDir;
    std::vector<RaySegment> mRayCasts;

    NPC* mTalkableNPC;
};