#pragma once

#include "actor/enemy/EnemyGuardGauge.h"

#include <glm/glm.hpp>

class Enemy;
class EnemyMovement;
class EnemyStateMachine;
class EnemyStatus;
class Player;

class EnemyCombat {
public:
    EnemyGuardDamageResult ApplyGuardDamage(
        Enemy& enemy,
        EnemyStatus& status,
        EnemyMovement& movement,
        EnemyStateMachine& stateMachine,
        float guardDamage,
        float deltaTime);
    EnemyGuardDamageResult ApplyGuardDamageEnsuringCurrentSegmentBreak(
        Enemy& enemy,
        EnemyStatus& status,
        EnemyMovement& movement,
        EnemyStateMachine& stateMachine,
        float minimumGuardDamage,
        float deltaTime);
    EnemyGuardDamageResult BreakGuard(
        Enemy& enemy,
        EnemyStatus& status,
        EnemyMovement& movement,
        EnemyStateMachine& stateMachine,
        float deltaTime);

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
    void LaunchWhenGuardBreaks(
        Enemy& enemy,
        EnemyStatus& status,
        EnemyMovement& movement,
        EnemyStateMachine& stateMachine,
        const EnemyGuardDamageResult& damageResult,
        float deltaTime);
    bool CanHitPlayer(const Enemy& enemy, const Player* player) const;
};
