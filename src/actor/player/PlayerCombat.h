#pragma once

#include "actor/player/PlayerAttackHitDetector.h"
#include "actor/player/PlayerAttackResolver.h"
#include "actor/player/PlayerTypes.h"

#include <vector>

class Player;
class PlayerInput;
class PlayerJewelGauge;
class PlayerMovement;
class PlayerStatus;

class PlayerCombat {
public:
    bool IsAttacking() const;
    bool HasPendingAttackHit() const { return mHasPendingAttackHit; }
    bool CanMoveDuringAttack() const { return mAttackMoveLockRemaining <= 0.0f && !mIsAirAttacking; }
    bool CanDodgeDuringAttack() const { return mAttackDodgeLockRemaining <= 0.0f; }
    bool IsSpecialCharging() const { return mSpecialChargingTimer >= 0.0f; }
    bool IsContinuousAttacking() const { return mContinuousAttackingTimer >= 0.0f; }
    bool IsAirAttackFloating() const { return mAirAttackFloatingTimer > 0.0f; }

    void StartAttacking(Player& player, const PlayerInput& input, PlayerMovement& movement, PlayerStatus& status,
                        float deltaTime);
    void StartCharging(Player& player);
    void StartStrongAttacking(Player& player, float deltaTime);
    void FinishCharging(Player& player, const PlayerMovement& movement);
    void FinishSpecialAttackCharging();

    void Attack(Player& player, PlayerMovement& movement, PlayerStatus& status, float deltaTime);
    void WideAttack(Player& player, PlayerMovement& movement, PlayerStatus& status, float deltaTime);
    void StrongAttack(Player& player, PlayerMovement& movement, PlayerStatus& status, float deltaTime);
    void SpecialAttack(Player& player, const PlayerMovement& movement, PlayerJewelGauge& jewelGauge, float deltaTime);
    void UpdateContinuousAttacking(Player& player, PlayerMovement& movement, PlayerStatus& status, float deltaTime);
    bool UpdatePendingAttackHit(Player& player, PlayerMovement& movement, PlayerStatus& status, float deltaTime);
    void StartAfterAttackReaction(const Player& player, PlayerMovement& movement, PlayerStatus& status);

    void StartSpecialAttackCharging();
    void StartContinuousAttacking();
    void StartTiredLock(PlayerStatus& status, PlayerMovement& movement, float lockTime);
    void ReduceTiredLock(PlayerStatus& status, PlayerMovement& movement, float reduceTime);
    void CancelSpecialAttack();
    void OnLanded();

    void UpdateAirAttackFloatingTimer(float deltaTime);
    void UpdateAttackCooldown(float deltaTime);
    void UpdateAttackMoveLock(PlayerStatus& status, float deltaTime);
    void UpdateAttackDodgeLock(float deltaTime);
    void UpdateComboKeepTimer(float deltaTime);

    void SetAttack(float attack) { mAttack = attack; }
    void SetAttackSpeed(float attackSpeed) { mAttackSpeed = attackSpeed; }
    void SetAttackCooldown(float attackCooldown) { mAttackCooldown = attackCooldown; }
    void SetLastAttackCooldown(float lastAttackCooldown) { mLastAttackCooldown = lastAttackCooldown; }
    void SetDefaultAttackPressTimer(float defaultAttackPressTimer)
    {
        mDefaultAttackPressTimer = defaultAttackPressTimer;
    }
    void SetSpecialAttackCooldown(float specialAttackCooldown) { mSpecialAttackCooldown = specialAttackCooldown; }
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
    void SetAttackHitDelay(float attackHitDelay) { mAttackHitDelay = attackHitDelay; }
    void SetAttackCooldownRemaining(float value) { mAttackCooldownRemaining = value; }
    void SetCanSpecialAttack(bool value) { mCanSpecialAttack = value; }
    void SetStrongAttackHit(bool value) { mIsStrongAttackHit = value; }
    void ResetAttackComboIndex() { mAttackComboIndex = 0; }

    PlayerAttackKind GetAttackKind() const { return mAttackKind; }
    bool GetIsStrongAttacked() const { return mIsStrongAttacked; }
    bool GetIsCharged() const { return mIsCharged; }
    bool GetCanSpecialAttack() const { return mCanSpecialAttack; }
    bool GetIsStrongAttackHit() const { return mIsStrongAttackHit; }
    int GetAttackComboIndex() const { return mAttackComboIndex; }
    float GetAttack() const { return mAttack; }
    float GetAttackSpeed() const { return mAttackSpeed; }
    float GetAttackCooldownRemaining() const { return mAttackCooldownRemaining; }
    float GetAttackCooldown() const { return mAttackCooldown; }
    float GetLastAttackCooldown() const { return mLastAttackCooldown; }
    float GetDefaultAttackPressTimer() const { return mDefaultAttackPressTimer; }
    float GetSpecialAttackCooldown() const { return mSpecialAttackCooldown; }
    float GetAttackMoveLockRemaining() const { return mAttackMoveLockRemaining; }
    float GetAttackDodgeLockRemaining() const { return mAttackDodgeLockRemaining; }
    float GetAttackMotionTimer() const { return mAttackMotionTimer; }
    float GetStrongAttackTimer() const { return mStrongAttackTimer; }
    float GetAttackPressTimer() const { return mAttackPressTimer; }
    float GetSpecialChargingTimer() const { return mSpecialChargingTimer; }
    float GetContinuousAttackingTimer() const { return mContinuousAttackingTimer; }
    float GetComboKeepTimer() const { return mComboKeepTimer; }
    float GetAttackRange() const { return mAttackRange; }
    float GetAttackAngle() const { return mAttackAngle; }
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
    float GetAttackHitDelay() const { return mAttackHitDelay; }
    const std::vector<PlayerRaySegment>& GetRayCasts() const { return mRayCasts; }

    void ReduceAttackMotionTimer(float deltaTime) { mAttackMotionTimer -= deltaTime; }
    void ReduceAttackPressTimer(float deltaTime) { mAttackPressTimer -= deltaTime; }
    void ReduceStrongAttackTimer(float deltaTime) { mStrongAttackTimer -= deltaTime; }
    void ReduceSpecialChargingTimer(float deltaTime) { mSpecialChargingTimer -= deltaTime; }
    void ReduceContinuousAttackingTimer(float deltaTime) { mContinuousAttackingTimer -= deltaTime; }
    void ReduceContinuousAttackingCooldown(float deltaTime) { mContinuousAttackingCooldown -= deltaTime; }
    void SetContinuousAttackingCooldown(float value) { mContinuousAttackingCooldown = value; }
    float GetContinuousAttackingCooldown() const { return mContinuousAttackingCooldown; }
    void ClearStrongAttackHit() { mIsStrongAttackHit = false; }
    bool CanAcceptMovementInput() const;

private:
    void StartAttackHitDelay();
    void ClearPendingAttackHit();

private:
    PlayerAttackKind mAttackKind = PlayerAttackKind::Normal;

    bool mIsStrongAttackHit = false;
    bool mIsStrongAttacked = false;
    bool mIsCharged = false;
    bool mCanSpecialAttack = false;
    bool mIsAirAttacking = false;
    bool mHasPendingAttackHit = false;

    int mAttackComboIndex = 0;

    float mAttackStartHeight = 0.0f;
    float mAttack = 10.0f;
    float mAttackSpeed = 5.0f;
    float mAttackCooldownRemaining = 0.0f;
    float mAttackCooldown = 0.3f;
    float mLastAttackCooldown = 1.0f;
    float mAttackMoveLockRemaining = -1.0f;
    float mAttackDodgeLockRemaining = 0.0f;
    float mAttackMotionTimer = -1.0f;
    float mDefaultAttackMotionTimer = 0.3f;
    float mAttackHitDelay = 0.5f;
    float mAttackHitDelayRemaining = -1.0f;
    float mAirAttackFloatingTimer = -1.0f;
    float mSpecialAttackCooldown = 30.0f;
    float mAttackPressTimer = -1.0f;
    float mDefaultAttackPressTimer = 0.0f;
    float mStrongAttackTimer = -1.0f;
    float mDefaultStrongAttackTimer = 0.06f;
    float mComboKeepTimer = -1.0f;
    float mAttackRange = 2.8f;
    float mAttackAngle = 0.8f;
    float mNormalAttackRange = 2.8f;
    float mNormalAttackAngle = 0.8f;
    float mNormalAttack = 10.0f;
    float mWideAttackRange = 2.8f;
    float mWideAttackAngle = -0.2f;
    float mWideAttack = 5.0f;
    float mStrongAttackRange = 6.0f;
    float mStrongAttack = 50.0f;
    float mStrongAttackSpeed = 100.0f;
    float mSpecialChargingTimer = -1.0f;
    float mContinuousAttackingTimer = -1.0f;
    float mContinuousAttackingCooldown = -1.0f;

    std::vector<PlayerRaySegment> mRayCasts;

    PlayerAttackHitDetector mHitDetector;
    PlayerAttackResolver mAttackResolver;
};
