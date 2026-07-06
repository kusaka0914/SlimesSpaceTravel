#include "actor/enemy/EnemyCombat.h"
#include "actor/Enemy.h"
#include "actor/Player.h"
#include "actor/enemy/EnemyMovement.h"
#include "actor/enemy/EnemyStateMachine.h"
#include "actor/enemy/EnemyStatus.h"
#include "system/AudioSystem.h"
#include "Game.h"
#include <glm/glm.hpp>

void EnemyCombat::ApplyDamage(Enemy& enemy, EnemyStatus& status, EnemyStateMachine& stateMachine, float damage,
                              Player* player)
{
    (void)player;

    if (!stateMachine.IsAlive(enemy)) {
        return;
    }

    status.AddDamage(damage);

    if (status.IsHp0()) {
        stateMachine.StartDying(enemy, status);
    }

    if (status.GetIsStrongAttacked()) {
        constexpr float knockBackTimer = 0.5f;
        stateMachine.StartKnockedBack(enemy, status, knockBackTimer);

        status.ClearStrongAttacked();
        stateMachine.FinishLaunched(enemy, status);
    } else if (!status.GetIsBoss() && enemy.IsOnGround()) {
        constexpr float knockBackTimer = 0.04f;
        stateMachine.StartKnockedBack(enemy, status, knockBackTimer);
    }
}

void EnemyCombat::ApplyBreak(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, EnemyStateMachine& stateMachine,
                             float deltaTime, bool isAllBreak)
{
    if (!stateMachine.IsAlive(enemy)) {
        return;
    }

    if (isAllBreak) {
        status.BreakAll();
    } else {
        status.DecrementBreakCount();
    }

    enemy.GetGame()->GetAudioSystem()->PlaySE("destroy_se");

    if (status.IsBreakCountEmpty()) {
        movement.LaunchIntoAir(enemy, status, stateMachine, deltaTime);
        return;
    }
}

void EnemyCombat::TryApplyAttack(Enemy& enemy, EnemyStatus& status, const EnemyStateMachine& stateMachine,
                                 float deltaTime)
{
    if (!stateMachine.IsProgressing(status)) {
        return;
    }

    constexpr float hitRangeMargin = 0.2f;
    const float hitRange = enemy.GetRadius() + hitRangeMargin;

    for (Player* player : enemy.GetGame()->GetPlayers()) {
        if (!player) {
            continue;
        }

        if (!player->IsAlive()) {
            continue;
        }

        if (status.HasHitPlayer(player)) {
            continue;
        }

        if (!IsPlayerInRange(enemy, player, hitRange)) {
            continue;
        }

        player->ApplyDamage(&enemy, deltaTime);
        status.AddHitPlayer(player);
    }
}

void EnemyCombat::ApplyCounter(Enemy& enemy, EnemyStatus& status, EnemyStateMachine& stateMachine, Player* player)
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

bool EnemyCombat::IsPlayerInRange(const Enemy& enemy, Player* player, float range) const
{
    if (!player) {
        return false;
    }

    const float distToPlayer = glm::length(player->GetPos() - enemy.GetPos());
    return distToPlayer <= range;
}
