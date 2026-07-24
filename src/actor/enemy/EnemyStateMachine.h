#pragma once

class Enemy;
class EnemyBehaviorController;
class EnemyCombat;
class EnemyMovement;
class EnemyStatus;

class EnemyStateMachine {
public:
    enum class LifeState { Alive, Dying, Dead };

    enum class ActionState {
        Idle,
        Tracking,
        PreparingAttack,
        Attacking,
        KnockedBack,
    };

    EnemyStateMachine();

    void UpdateAlive(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, EnemyCombat& combat,
                     EnemyBehaviorController& behaviorController, float deltaTime);
    void UpdateDying(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, float deltaTime);

    void UpdateIdle(Enemy& enemy, EnemyStatus& status, EnemyCombat& combat);
    void UpdateTracking(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, EnemyCombat& combat, float deltaTime);
    void UpdatePreparingAttack(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, float deltaTime);
    void UpdateAttacking(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, EnemyCombat& combat, float deltaTime);
    void UpdateKnockedBack(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, float deltaTime);

    void StartIdle(Enemy& enemy);
    void StartTracking(Enemy& enemy);
    void TryStartPreparingAttack(Enemy& enemy, EnemyStatus& status, EnemyCombat& combat);
    void StartPreparingAttack(Enemy& enemy, EnemyStatus& status);
    void StartAttacking(Enemy& enemy, EnemyStatus& status);
    void StartKnockedBack(Enemy& enemy, EnemyStatus& status, float knockBackTimer);
    void StartDying(Enemy& enemy, EnemyStatus& status);

    void FinishLaunched(Enemy& enemy, EnemyStatus& status);
    void FinishDying(Enemy& enemy, const EnemyStatus& status);

    LifeState GetLifeState() const { return mLifeState; }
    void SetLifeState(LifeState lifeState) { mLifeState = lifeState; }

    ActionState GetActionState() const { return mActionState; }
    void SetActionState(ActionState actionState) { mActionState = actionState; }

    bool IsJustBeforeAttack(const EnemyStatus& status) const;
    bool IsProgressing(const EnemyStatus& status) const;
    bool IsAlive() const { return mLifeState == LifeState::Alive; }
    bool IsDead() const { return mLifeState == LifeState::Dead; }
    bool IsAlive(const Enemy& enemy) const;

private:
    LifeState mLifeState;
    ActionState mActionState;
};
