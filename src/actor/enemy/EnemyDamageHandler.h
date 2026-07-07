#pragma once

class Enemy;
class EnemyStateMachine;
class EnemyStatus;
class Player;

class EnemyDamageHandler {
public:
    void ApplyDamage(Enemy& enemy, EnemyStatus& status, EnemyStateMachine& stateMachine, float damage, Player* player);
    void ApplyCounter(Enemy& enemy, EnemyStatus& status, EnemyStateMachine& stateMachine, Player* player);
};
