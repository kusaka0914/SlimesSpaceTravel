#pragma once

class Player;
class PlayerInput;
class PlayerMovement;
class PlayerCombat;
class PlayerStatus;
class PlayerRespawn;

class PlayerStateMachine {
public:
    void Update(Player& player, PlayerInput& input, PlayerMovement& movement, PlayerCombat& combat,
                PlayerStatus& status, PlayerRespawn& respawn, float deltaTime);

    void UpdateAlive(Player& player, PlayerInput& input, PlayerMovement& movement, PlayerCombat& combat,
                     PlayerStatus& status, PlayerRespawn& respawn, float deltaTime);
    void UpdateIdle(Player& player, PlayerInput& input, PlayerMovement& movement, PlayerCombat& combat,
                    PlayerStatus& status, float deltaTime);
    void UpdateDodging(Player& player, PlayerMovement& movement, PlayerCombat& combat, float deltaTime);
    void UpdateAttacking(Player& player, PlayerInput& input, PlayerMovement& movement, PlayerCombat& combat,
                         PlayerStatus& status, float deltaTime);
    void UpdateCharging(Player& player, PlayerInput& input, PlayerMovement& movement, PlayerCombat& combat,
                        float deltaTime);
    void UpdateStrongAttacking(Player& player, PlayerMovement& movement, PlayerCombat& combat, PlayerStatus& status,
                               float deltaTime);
    void UpdateKnockedBack(Player& player, PlayerMovement& movement, PlayerCombat& combat, PlayerStatus& status,
                           float deltaTime);
    void UpdateSpecialAttackCharging(Player& player, PlayerInput& input, PlayerMovement& movement,
                                     PlayerCombat& combat, float deltaTime);
    void UpdateContinuousAttacking(Player& player, PlayerMovement& movement, PlayerCombat& combat,
                                   PlayerStatus& status, float deltaTime);
    void UpdateTimer(PlayerInput& input, PlayerMovement& movement, PlayerCombat& combat, PlayerStatus& status,
                     float deltaTime);
    void StartIdle(PlayerCombat& combat);
    void StartJewelTimer(PlayerCombat& combat);
    void StartTired(PlayerStatus& status, PlayerMovement& movement, PlayerCombat& combat, float lockTime);
    void ReduceTired(PlayerStatus& status, PlayerMovement& movement, PlayerCombat& combat);
};
