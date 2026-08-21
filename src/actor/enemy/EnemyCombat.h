#pragma once

#include <glm/glm.hpp>

class Enemy;
class EnemyMovement;
class EnemyStateMachine;
class EnemyStatus;
class Player;

class EnemyCombat {
public:
    void ApplyBreak(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, EnemyStateMachine& stateMachine,
                    float deltaTime, bool isAllBreak = false);

    void TryApplyAttack(Enemy& enemy, EnemyStatus& status, const EnemyStateMachine& stateMachine,
                        float deltaTime);
    void TryApplyFanAttack(Enemy& enemy, EnemyStatus& status, const EnemyStateMachine& stateMachine,
                           float range, float angleRadians,
                           float deltaTime);
    void TryApplyGroundRadialAttack(Enemy& enemy, EnemyStatus& status, const EnemyStateMachine& stateMachine,
                                    float range,
                                    float deltaTime);

    bool IsPlayerInRange(const Enemy& enemy, Player* player, float range) const;

private:
    bool CanHitPlayer(const Enemy& enemy, const Player* player) const;
};
