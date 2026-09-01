#pragma once

#include <glm/vec3.hpp>

class Enemy;
class Player;
class PlayerCombat;
class PlayerInput;
class PlayerJewelGauge;
class PlayerMovement;
class PlayerStateMachine;
class PlayerStatus;

class PlayerDamageHandler {
public:
    static void Apply(Player& player, PlayerInput& input, PlayerMovement& movement, PlayerStateMachine& stateMachine,
                      PlayerCombat& combat, PlayerJewelGauge& jewelGauge, PlayerStatus& status, Enemy* enemy,
                      float deltaTime);

    static void ApplyFromActor(
        Player& player,
        PlayerInput& input,
        PlayerMovement& movement,
        PlayerStateMachine& stateMachine,
        PlayerCombat& combat,
        PlayerStatus& status,
        const glm::vec3& damageSourcePosition,
        float damage);
};
