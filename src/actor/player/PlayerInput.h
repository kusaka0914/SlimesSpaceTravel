#pragma once

struct PlayerModuleContext;

class PlayerInput {
public:
    bool dodgePressed = false;
    bool dodgePressedPrev = false;
    bool jumpPressed = false;
    bool attackPressed = false;
    bool attackPressedPrev = false;
    bool wideAttackPressed = false;
    bool wideAttackPressedPrev = false;
    bool specialAttackPressed = false;
    bool specialAttackPressedPrev = false;
    bool recoverPressed = false;
    bool recoverPressedPrev = false;

    float cameraYaw = 0.0f;
    float moveForward = 0.0f;
    float moveLeft = 0.0f;
    float cameraStickX = 0.0f;
    float cameraStickY = 0.0f;
    float inputAvailableTimer = -1.0f;

    void ProcessActor(PlayerModuleContext& context);
    void ProcessGameController(PlayerModuleContext& context);
    void ProcessKeyboard(PlayerModuleContext& context);
};