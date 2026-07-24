#pragma once

#include "CharacterActor.h"
#include "actor/enemy/EnemyStateMachine.h"
#include "actor/enemy/EnemyStatus.h"

#include <glm/glm.hpp>
#include <memory>
#include <string>

class EnemyCombat;
class EnemyBehaviorController;
class EnemyDamageHandler;
class EnemyMovement;
class Game;
class Player;
struct EnemyAttackPreview;
struct EnemyConfig;

class Enemy : public CharacterActor {
public:
    using LifeState = EnemyStateMachine::LifeState;
    using ActionState = EnemyStateMachine::ActionState;

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

    bool GetIsDead() const { return mStateMachine->IsDead(); }
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

    LifeState GetLifeState() const { return mStateMachine->GetLifeState(); }
    void SetLifeState(LifeState lifeState) { mStateMachine->SetLifeState(lifeState); }

    ActionState GetActionState() const { return mStateMachine->GetActionState(); }
    void SetActionState(ActionState actionState) { mStateMachine->SetActionState(actionState); }

    bool IsAlive() const { return mStateMachine->IsAlive(); }
    bool IsOnGround() const { return mOnGround; }
    void SetOnGroundForEnemy(bool onGround) { mOnGround = onGround; }
    void SetShouldJudgeLandingForEnemy(bool shouldJudgeLanding) { mShouldJudgeLanding = shouldJudgeLanding; }

    const glm::vec3& GetVelocity() const { return mVelocity; }
    void SetVelocity(const glm::vec3& velocity) { mVelocity = velocity; }
    void AddVelocity(const glm::vec3& velocity) { mVelocity += velocity; }

    void AddPos(const glm::vec3& delta) { mPos += delta; }
    void SetFacingForwardForEnemy(const glm::vec3& facingForward) { mFacingForwardVec = facingForward; }
    void SetFacingYawForEnemy(float facingYaw) { mFacingYaw = facingYaw; }

    const char* GetCurrentBehaviorActionType() const;
    const std::string& GetBehaviorProfileName() const;
    bool GetBehaviorAttackPreview(EnemyAttackPreview& preview) const;

    void ApplyGravityForEnemy(float deltaTime) { ApplyGravity(deltaTime); }
    bool IsSteepGroundForEnemy(const glm::vec3& hitNormal, const glm::vec3& up) const
    {
        return CheckDotAngleSteep(hitNormal, up);
    }

private:
    void ApplyEnemyConfig(const EnemyConfig& config);

private:
    EnemyStatus mStatus;

    std::unique_ptr<EnemyStateMachine> mStateMachine;
    std::unique_ptr<EnemyMovement> mMovement;
    std::unique_ptr<EnemyCombat> mCombat;
    std::unique_ptr<EnemyDamageHandler> mDamageHandler;
    std::unique_ptr<EnemyBehaviorController> mBehaviorController;
};
