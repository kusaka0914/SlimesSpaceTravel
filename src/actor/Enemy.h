#pragma once

#include "CharacterActor.h"
#include "actor/enemy/EnemyStatus.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>

class EnemyCombat;
class EnemyMovement;
class EnemyStateMachine;
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
    ~Enemy() override;

    void UpdateActor(float deltaTime) override;

    void ApplyDamage(float damage, Player* player);
    void ApplyBreak(float deltaTime, bool isAllBreak = false);
    void ApplyConfig(const std::string& type);

    void SetIsBoss(bool isBoss) { mStatus.SetIsBoss(isBoss); }
    void SetIsStrongAttacked(bool isStrongAttacked) { mStatus.SetIsStrongAttacked(isStrongAttacked); }

    void SetBreakCount(int breakCount) { mStatus.SetBreakCount(breakCount); }
    void SetBreakCountMax(int breakCountMax) { mStatus.SetBreakCountMax(breakCountMax); }

    void SetHp(float hp) { mStatus.SetHp(hp); }
    void SetMaxHp(float maxHp) { mStatus.SetMaxHp(maxHp); }
    void SetDefaultLaunchedTimer(float defaultLaunchedTimer) { mStatus.SetDefaultLaunchedTimer(defaultLaunchedTimer); }
    void SetMoveSpeed(float moveSpeed) { mStatus.SetMoveSpeed(moveSpeed); }
    void SetAttack(float attack) { mStatus.SetAttack(attack); }
    void SetDefaultAttackMotionTimer(float defaultAttackMotionTimer)
    {
        mStatus.SetDefaultAttackMotionTimer(defaultAttackMotionTimer);
    }
    void SetDefaultStandByAttackTimer(float defaultStandByAttackTimer)
    {
        mStatus.SetDefaultStandByAttackTimer(defaultStandByAttackTimer);
    }
    void SetDetectionRange(float detectionRange) { mStatus.SetDetectionRange(detectionRange); }
    void SetKnockBackSpeed(float knockBackSpeed) { mStatus.SetKnockBackSpeed(knockBackSpeed); }
    void SetAttackSpeed(float attackSpeed) { mStatus.SetAttackSpeed(attackSpeed); }
    void FlipCanCountered() { mStatus.FlipCanCountered(); }

    bool GetIsDead() const { return mLifeState == LifeState::Dead; }
    bool GetIsBoss() const { return mStatus.GetIsBoss(); }
    bool GetCanCountered() const { return mStatus.GetCanCountered(); }

    int GetBreakCount() const { return mStatus.GetBreakCount(); }

    float GetHp() const { return mStatus.GetHp(); }
    float GetMaxHp() const { return mStatus.GetMaxHp(); }
    float GetAttack() const { return mStatus.GetAttack(); }
    float GetAttackRange() const { return mStatus.GetAttackRange(); }
    float GetStandByAttackTimer() const { return mStatus.GetStandByAttackTimer(); }

    int GetBreakCountMax() const { return mStatus.GetBreakCountMax(); }

    float GetDetectionRange() const { return mStatus.GetDetectionRange(); }
    float GetMoveSpeed() const { return mStatus.GetMoveSpeed(); }
    float GetKnockBackSpeed() const { return mStatus.GetKnockBackSpeed(); }
    float GetAttackSpeed() const { return mStatus.GetAttackSpeed(); }

    float GetDefaultStandByAttackTimer() const { return mStatus.GetDefaultStandByAttackTimer(); }
    float GetDefaultLaunchedTimer() const { return mStatus.GetDefaultLaunchedTimer(); }
    float GetDefaultAttackMotionTimer() const { return mStatus.GetDefaultAttackMotionTimer(); }

    LifeState GetLifeState() const { return mLifeState; }
    void SetLifeState(LifeState lifeState) { mLifeState = lifeState; }

    ActionState GetActionState() const { return mActionState; }
    void SetActionState(ActionState actionState) { mActionState = actionState; }

    bool IsAlive() const { return mLifeState == LifeState::Alive; }
    bool IsOnGround() const { return mOnGround; }
    void SetOnGroundForEnemy(bool onGround) { mOnGround = onGround; }
    void SetShouldJudgeLandingForEnemy(bool shouldJudgeLanding) { mShouldJudgeLanding = shouldJudgeLanding; }

    const glm::vec3& GetVelocity() const { return mVelocity; }
    void SetVelocity(const glm::vec3& velocity) { mVelocity = velocity; }
    void AddVelocity(const glm::vec3& velocity) { mVelocity += velocity; }

    void AddPos(const glm::vec3& delta) { mPos += delta; }
    void SetFacingForwardForEnemy(const glm::vec3& facingForward) { mFacingForwardVec = facingForward; }
    void SetFacingYawForEnemy(float facingYaw) { mFacingYaw = facingYaw; }

    void ApplyGravityForEnemy(float deltaTime) { ApplyGravity(deltaTime); }
    bool IsSteepGroundForEnemy(const glm::vec3& hitNormal, const glm::vec3& up) const
    {
        return CheckDotAngleSteep(hitNormal, up);
    }

private:
    EnemyStatus mStatus;
    LifeState mLifeState;
    ActionState mActionState;

    std::unique_ptr<EnemyStateMachine> mStateMachine;
    std::unique_ptr<EnemyMovement> mMovement;
    std::unique_ptr<EnemyCombat> mCombat;
};
