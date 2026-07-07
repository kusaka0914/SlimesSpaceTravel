#include "actor/player/PlayerAttackHitDetector.h"

#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "actor/player/PlayerCombat.h"

#include <cmath>
#include <glm/glm.hpp>

std::vector<Enemy*> PlayerAttackHitDetector::FindHitEnemies(Player& player, const PlayerCombat& combat) const
{
    std::vector<Enemy*> hitEnemies;

    if (!player.GetCurrentPlanet()) {
        return hitEnemies;
    }

    for (Enemy* enemy : player.GetCurrentPlanet()->GetEnemies()) {
        if (enemy->GetIsDead()) {
            continue;
        }

        if (combat.GetAttackKind() == PlayerAttackKind::Strong && enemy->GetOnGround()) {
            continue;
        }

        const glm::vec3 enemyPos = enemy->GetPos();
        const glm::vec3 toEnemy =
            glm::normalize((enemyPos + enemy->GetFacingForwardVec() * (enemy->GetRadius() - 1.0f)) - player.GetPos());

        const float dist = glm::length(enemyPos - player.GetPos());
        const float dot = glm::dot(player.GetFacingForwardVec(), toEnemy);
        const float effectiveRange = combat.GetAttackRange() + enemy->GetRadius();

        if (IsEnemyHitByAttack(dist, dot, effectiveRange, combat.GetAttackAngle())) {
            hitEnemies.push_back(enemy);
        }
    }

    return hitEnemies;
}

bool PlayerAttackHitDetector::IsEnemyHitByAttack(float dist, float dot, float effectiveRange, float attackAngle) const
{
    const float threshold = std::cos(attackAngle * 0.5f);
    return dist <= effectiveRange && dot >= threshold;
}
