#pragma once

class Enemy;
class EnemyMovement;
class EnemyStateMachine;
class EnemyStatus;
class Player;

class EnemyCombat {
public:
    void ApplyDamage(Enemy& enemy, EnemyStatus& status, EnemyStateMachine& stateMachine, float damage, Player* player);
    void ApplyBreak(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, EnemyStateMachine& stateMachine,
                    float deltaTime, bool isAllBreak = false);

    void TryApplyAttack(Enemy& enemy, EnemyStatus& status, const EnemyStateMachine& stateMachine, float deltaTime);
    void ApplyCounter(Enemy& enemy, EnemyStatus& status, EnemyStateMachine& stateMachine, Player* player);

    bool IsPlayerInRange(const Enemy& enemy, Player* player, float range) const;
};
