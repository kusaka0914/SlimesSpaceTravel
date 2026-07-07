#pragma once

#include <vector>

class Enemy;
class Player;
class PlayerCombat;
class PlayerJewelGauge;
class PlayerMovement;
class PlayerStatus;

class PlayerAttackResolver {
public:
    void ResolveAttack(Player& player, PlayerMovement& movement, PlayerStatus& status, PlayerCombat& combat,
                       const std::vector<Enemy*>& hitEnemies, float deltaTime) const;

    void ResolveSpecialAttack(Player& player, PlayerJewelGauge& jewelGauge,
                              const std::vector<Enemy*>& hitEnemies, float deltaTime) const;
};
