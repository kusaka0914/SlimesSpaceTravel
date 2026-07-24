#include "actor/enemy/EnemyCombat.h"

#include "Game.h"
#include "actor/Enemy.h"
#include "actor/Player.h"
#include "actor/enemy/EnemyMovement.h"
#include "actor/enemy/EnemyStateMachine.h"
#include "actor/enemy/EnemyStatus.h"
#include "system/AudioSystem.h"

#include <cmath>
#include <glm/glm.hpp>

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

void EnemyCombat::TryApplyFanAttack(Enemy& enemy, EnemyStatus& status, float range,
                                    float angleRadians, float deltaTime)
{
    const float halfAngleThreshold = std::cos(angleRadians * 0.5f);
    const glm::vec3 forward = glm::normalize(enemy.GetFacingForwardVec());

    for (Player* player : enemy.GetGame()->GetPlayers()) {
        if (!player || !player->IsAlive() || status.HasHitPlayer(player)) {
            continue;
        }

        const glm::vec3 toPlayer = player->GetPos() - enemy.GetPos();
        const float distance = glm::length(toPlayer);
        const float effectiveRange = range + player->GetRadius();

        if (distance > effectiveRange) {
            continue;
        }

        const float facingDot =
            distance > 1e-6f ? glm::dot(forward, toPlayer / distance) : 1.0f;
        if (facingDot < halfAngleThreshold) {
            continue;
        }

        player->ApplyDamage(&enemy, deltaTime);
        status.AddHitPlayer(player);
    }
}

void EnemyCombat::TryApplyGroundRadialAttack(
    Enemy& enemy, EnemyStatus& status, float range, float deltaTime)
{
    for (Player* player : enemy.GetGame()->GetPlayers()) {
        if (!player || !player->IsAlive() || !player->GetOnGround() ||
            status.HasHitPlayer(player)) {
            continue;
        }

        const float effectiveRange = range + player->GetRadius();
        if (!IsPlayerInRange(enemy, player, effectiveRange)) {
            continue;
        }

        player->ApplyDamage(&enemy, deltaTime);
        status.AddHitPlayer(player);
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
