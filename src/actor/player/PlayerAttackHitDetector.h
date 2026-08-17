#pragma once

#include <glm/glm.hpp>

#include <vector>

class Enemy;
class Player;
class PlayerCombat;

class PlayerAttackHitDetector {
public:
    std::vector<Enemy*> FindHitEnemies(Player& player, const PlayerCombat& combat) const;
    std::vector<Enemy*> FindEnemiesInRadius(
        Player& player,
        float range) const;
    std::vector<Enemy*> FindEnemiesTouchingAirDodgeMovement(
        Player& player,
        const glm::vec3& movementStart,
        const glm::vec3& movementEnd) const;

private:
    bool IsGroundedEnemyWithinAirAttackHeight(
        const Player& player,
        const Enemy& enemy) const;
    bool IsEnemyModelHitByAttack(
        const Player& player,
        const Enemy& enemy,
        float attackRange,
        float attackAngle) const;
    bool IsEnemyModelWithinRadius(
        const Player& player,
        const Enemy& enemy,
        float range) const;
    bool DoesAirDodgePathTouchEnemyModel(
        const Player& player,
        const Enemy& enemy,
        const glm::vec3& movementStart,
        const glm::vec3& movementEnd) const;
    bool IsEnemyHitByAttack(float dist, float dot, float effectiveRange, float attackAngle) const;
};
