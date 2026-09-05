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
                       const std::vector<Enemy*>& hitEnemies,
                       bool didHitHazardActor,
                       float deltaTime) const;

    void ResolveSpecialAttack(Player& player, PlayerJewelGauge& jewelGauge,
                              const std::vector<Enemy*>& hitEnemies,
                              float chargedAttackDamage,
                              float deltaTime) const;
    bool ResolveAirSlamAttack(
        Player& player,
        const PlayerMovement& movement,
        PlayerCombat& combat,
        const std::vector<Enemy*>& hitEnemies,
        float deltaTime) const;
    bool ResolveAirSlamContact(
        Player& player,
        const PlayerMovement& movement,
        const std::vector<Enemy*>& hitEnemies,
        float enemyDownwardSpeed,
        float maximumDamage,
        float fullDamageHeight,
        float minimumDamageRatio,
        bool shouldAssignImpactFeedback) const;
    bool ResolveAirDodgeAttack(
        Player& player,
        const PlayerMovement& movement,
        const std::vector<Enemy*>& hitEnemies,
        float damage,
        float enemyPushSpeed,
        float enemyPushDampingPerSecond,
        float enemyLiftHeight) const;
};
