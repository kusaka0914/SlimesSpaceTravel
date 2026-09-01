#include "actor/enemy/EnemyDamageHandler.h"

#include "actor/Enemy.h"
#include "actor/Player.h"
#include "actor/enemy/EnemyMovement.h"
#include "actor/enemy/EnemyStateMachine.h"
#include "actor/enemy/EnemyStatus.h"

#include <glm/glm.hpp>

void EnemyDamageHandler::ApplyDamage(
    Enemy& enemy,
    EnemyStatus& status,
    EnemyStateMachine& stateMachine,
    EnemyMovement& movement,
    float damage,
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

    if (status.GetIsBoss()) {
        enemy.StartBossHitReaction();
    } else {
        enemy.StartNormalHitReaction();
    }

    if (status.GetIsStrongAttacked()) {
        // 強攻撃はプレイヤーから敵への3D方向へ移動するため、時間を2倍にすると
        // 横・縦のノックバック距離が同じ比率で2倍になる。
        constexpr float knockBackTimer = 1.0f;
        stateMachine.StartKnockedBack(enemy, status, knockBackTimer);

        status.ClearStrongAttacked();
        stateMachine.FinishLaunched(enemy, status);
        return;
    }

    if (status.GetIsBoss()) {
        return;
    }

    const bool canRestartNormalHitKnockBack =
        enemy.IsOnGround() ||
        stateMachine.GetActionState() ==
            EnemyStateMachine::ActionState::KnockedBack;
    if (status.IsNormalHitKnockBackEnabled() &&
        canRestartNormalHitKnockBack) {
        constexpr float knockBackTimer = 0.04f;
        stateMachine.StartKnockedBack(enemy, status, knockBackTimer);
        movement.StartNormalHitKnockBack(enemy, status);
    }
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
    status.AddDamage(
        player->CalculateOutgoingAttackDamage(
            player->GetAttack() * 2.0f));
    status.SetStandByAttackTimer(-1.0f);

    if (status.IsHp0()) {
        stateMachine.StartDying(enemy, status);
    }
}
