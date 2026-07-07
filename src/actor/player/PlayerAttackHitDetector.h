#pragma once

#include <vector>

class Enemy;
class Player;
class PlayerCombat;

class PlayerAttackHitDetector {
public:
    std::vector<Enemy*> FindHitEnemies(Player& player, const PlayerCombat& combat) const;

private:
    bool IsEnemyHitByAttack(float dist, float dot, float effectiveRange, float attackAngle) const;
};
