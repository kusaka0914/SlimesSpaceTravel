#pragma once

#include <random>

class Enemy;
class EnemyBehaviorController;
class EnemyCombat;
class EnemyMovement;
class EnemyStatus;
struct EnemyBossManeuverConfig;

class EnemyStateMachine {
public:
    enum class LifeState { Alive, Dying, Dead };

    enum class ActionState {
        Idle,
        Tracking,
        PreparingAttack,
        PreAttackApproach,
        Attacking,
        PostAttackRetreatDelay,
        PostAttackRetreat,
        PostRetreatRecovery,
        KnockedBack,
    };

    EnemyStateMachine();
    void ConfigureBossManeuver(const EnemyBossManeuverConfig& config);

    void UpdateAlive(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, EnemyCombat& combat,
                     EnemyBehaviorController& behaviorController, float deltaTime);
    void UpdateDying(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, float deltaTime);

    void UpdateIdle(Enemy& enemy, EnemyStatus& status, EnemyCombat& combat);
    void UpdateTracking(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, EnemyCombat& combat, float deltaTime);
    void UpdatePreparingAttack(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, float deltaTime);
    void UpdatePreAttackApproach(
        Enemy& enemy,
        EnemyStatus& status,
        EnemyMovement& movement,
        float deltaTime);
    void UpdateAttacking(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, EnemyCombat& combat, float deltaTime);
    void UpdateKnockedBack(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, float deltaTime);
    void UpdatePostAttackRetreatDelay(float deltaTime);
    void UpdatePostAttackRetreat(
        Enemy& enemy,
        EnemyStatus& status,
        EnemyMovement& movement,
        float deltaTime);
    void UpdatePostRetreatRecovery(
        Enemy& enemy,
        EnemyStatus& status,
        float deltaTime);

    void StartIdle(Enemy& enemy);
    void StartTracking(Enemy& enemy);
    void TryStartPreparingAttack(Enemy& enemy, EnemyStatus& status, EnemyCombat& combat);
    void StartPreparingAttack(Enemy& enemy, EnemyStatus& status);
    void StartAttacking(Enemy& enemy, EnemyStatus& status);
    bool TryStartPostAttackRetreat(Enemy& enemy, const EnemyStatus& status);
    void StartKnockedBack(Enemy& enemy, EnemyStatus& status, float knockBackTimer);
    void StartDying(Enemy& enemy, EnemyStatus& status);

    void FinishLaunched(Enemy& enemy, EnemyStatus& status);
    void FinishDying(Enemy& enemy, const EnemyStatus& status);

    LifeState GetLifeState() const { return mLifeState; }
    void SetLifeState(LifeState lifeState) { mLifeState = lifeState; }

    ActionState GetActionState() const { return mActionState; }
    void SetActionState(ActionState actionState) { mActionState = actionState; }

    bool IsJustBeforeAttack(const EnemyStatus& status) const;
    bool ShouldPreservePreparationTimer() const
    {
        return mShouldPreservePreparationTimer;
    }
    bool IsProgressing(const EnemyStatus& status) const;
    bool IsAlive() const { return mLifeState == LifeState::Alive; }
    bool IsDead() const { return mLifeState == LifeState::Dead; }
    bool IsAlive(const Enemy& enemy) const;

private:
    bool ShouldTriggerProbability(float probabilityPercent);

    LifeState mLifeState;
    ActionState mActionState;
    float mPreAttackApproachProbabilityPercent = 0.0f;
    float mPreAttackApproachSpeed = 12.0f;
    float mPreAttackApproachStopDistance = 2.5f;
    float mPostAttackRetreatProbabilityPercent = 0.0f;
    float mPostAttackRetreatDelaySeconds = 1.0f;
    float mPostAttackRetreatDelayRemainingSeconds = 0.0f;
    float mPostAttackRetreatSpeed = 12.0f;
    float mPostAttackRetreatDistance = 4.0f;
    float mPostAttackRetreatRemainingDistance = 0.0f;
    float mPostRetreatRecoverySeconds = 2.0f;
    float mPostRetreatRecoveryRemainingSeconds = 0.0f;
    float mPostRetreatFollowupApproachProbabilityPercent = 0.0f;
    bool mHasEvaluatedPreAttackApproach = false;
    bool mShouldPreservePreparationTimer = false;
    std::mt19937 mManeuverRandomEngine;
};
