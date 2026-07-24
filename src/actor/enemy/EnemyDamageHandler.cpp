#include "actor/enemy/EnemyDamageHandler.h"

#include "actor/Enemy.h"
#include "actor/Player.h"
#include "actor/enemy/EnemyStateMachine.h"
#include "actor/enemy/EnemyStatus.h"

#include <glm/glm.hpp>

void EnemyDamageHandler::ApplyDamage(Enemy& enemy, EnemyStatus& status, EnemyStateMachine& stateMachine, float damage,
                                     Player* player)
{
    (void)player;

    if (!stateMachine.IsAlive(enemy)) {
        return;
    }

    status.AddDamage(damage);

    if (status.IsHp0()) {
        stateMachine.StartDying(enemy, status);
        return;
    }

    if (status.GetIsStrongAttacked()) {
        constexpr float knockBackTimer = 0.5f;
        stateMachine.StartKnockedBack(enemy, status, knockBackTimer);

        status.ClearStrongAttacked();
        stateMachine.FinishLaunched(enemy, status);
        return;
    }

    if (!status.GetIsBoss() && enemy.IsOnGround()) {
        constexpr float knockBackTimer = 0.04f;
        stateMachine.StartKnockedBack(enemy, status, knockBackTimer);
    }
}

void EnemyDamageHandler::ApplyCounter(Enemy& enemy, EnemyStatus& status, EnemyStateMachine& stateMachine,
                                      Player* player)
{
    if (!player) {
        return;
    }

    constexpr float knockBackTimer = 0.6f;
    stateMachine.StartKnockedBack(enemy, status, knockBackTimer);

    status.ClearIsCountered();
    status.AddDamage(player->GetAttack() * 2.0f);
    status.SetStandByAttackTimer(-1.0f);

    if (status.IsHp0()) {
        stateMachine.StartDying(enemy, status);
    }
}
