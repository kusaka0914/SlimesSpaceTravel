#include "actor/enemy/EnemyDamageHandler.h"

#include "actor/Enemy.h"
#include "actor/Player.h"
#include "actor/enemy/EnemyMovement.h"
#include "actor/enemy/EnemyStateMachine.h"
#include "actor/enemy/EnemyStatus.h"

#include <glm/glm.hpp>

void EnemyDamageHandler::ApplyDamage(Enemy& enemy, EnemyStatus& status, EnemyStateMachine& stateMachine,
                                     EnemyMovement& movement, float damage, Player* player)
{
    (void)player;
    (void)movement;

    if (!stateMachine.IsAlive(enemy)) {
        return;
    }

    status.AddDamage(damage);

    if (status.IsHp0()) {
        stateMachine.StartDying(enemy, status);
        return;
    }

    if (status.GetIsBoss()) {
        enemy.StartBossHitReaction();
    } else {
        enemy.StartNormalHitReaction();
    }

    if (status.GetIsStrongAttacked() && (!status.GetIsBoss() || enemy.IsLaunched())) {
        // 強攻撃では通常被弾用の接線方向への補正を行わず、3Dの押し戻し方向を維持する。
        constexpr float knockBackTimer = 1.0f;
        stateMachine.StartKnockedBack(enemy, status, knockBackTimer);

        status.ClearStrongAttacked();
        stateMachine.FinishLaunched(enemy, status);
        return;
    }

    if (status.GetIsBoss()) {
        status.ClearStrongAttacked();
        return;
    }

    // 通常被弾の見た目だけを更新し、準備中・発動中の行動と予兆タイマーは継続する。
    status.ClearStrongAttacked();
}

void EnemyDamageHandler::ApplyCounter(Enemy& enemy, EnemyStatus& status, EnemyStateMachine& stateMachine,
                                      Player* player)
{
    if (!player) {
        return;
    }

    if (status.GetIsBoss()) {
        stateMachine.StartIdle(enemy);
        enemy.StartBossHitReaction();
    } else {
        constexpr float knockBackTimer = 0.6f;
        stateMachine.StartKnockedBack(enemy, status, knockBackTimer);
    }

    status.ClearIsCountered();
    status.AddDamage(player->CalculateOutgoingAttackDamage(player->GetAttack() * 2.0f));
    status.SetStandByAttackTimer(-1.0f);

    if (status.IsHp0()) {
        stateMachine.StartDying(enemy, status);
    }
}
