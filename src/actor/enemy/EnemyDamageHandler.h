#pragma once

class Enemy;
class EnemyMovement;
class EnemyStateMachine;
class EnemyStatus;
class Player;

class EnemyDamageHandler {
public:
    void ApplyDamage(
        Enemy& enemy,
        EnemyStatus& status,
        EnemyStateMachine& stateMachine,
        EnemyMovement& movement,
        float damage,
        Player* player);
    void ApplyCounter(Enemy& enemy, EnemyStatus& status, EnemyStateMachine& stateMachine, Player* player);
};
