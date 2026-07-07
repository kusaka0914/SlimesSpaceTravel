#pragma once

class Enemy;
class EnemyMovement;
class EnemyStateMachine;
class EnemyStatus;
class Player;

class EnemyCombat {
public:
    void ApplyBreak(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, EnemyStateMachine& stateMachine,
                    float deltaTime, bool isAllBreak = false);

    void TryApplyAttack(Enemy& enemy, EnemyStatus& status, const EnemyStateMachine& stateMachine, float deltaTime);

    bool IsPlayerInRange(const Enemy& enemy, Player* player, float range) const;
};
