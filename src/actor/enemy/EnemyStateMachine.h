#pragma once

class Enemy;
class EnemyCombat;
class EnemyMovement;
class EnemyStatus;

class EnemyStateMachine {
public:
    void UpdateAlive(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, EnemyCombat& combat, float deltaTime);
    void UpdateDying(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, float deltaTime);
    void UpdateBehavior(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, EnemyCombat& combat, float deltaTime);

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

    bool IsJustBeforeAttack(const EnemyStatus& status) const;
    bool IsProgressing(const EnemyStatus& status) const;
    bool IsAlive(const Enemy& enemy) const;
};
