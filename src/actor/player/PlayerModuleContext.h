#pragma once

class Player;
class PlayerInput;
class PlayerMovement;
class PlayerCombat;
class PlayerStatus;
class PlayerRespawn;
class PlayerInteraction;
class PlayerStateMachine;

struct PlayerModuleContext {
    Player& player;
    PlayerInput& input;
    PlayerMovement& movement;
    PlayerCombat& combat;
    PlayerStatus& status;
    PlayerRespawn& respawn;
    PlayerInteraction& interaction;
    PlayerStateMachine& stateMachine;
};