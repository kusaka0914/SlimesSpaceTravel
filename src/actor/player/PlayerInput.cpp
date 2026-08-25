#include "actor/player/PlayerInput.h"

#include "actor/Player.h"
#include "actor/player/PlayerMovement.h"
#include "system/SceneSystem.h"

#include <GLFW/glfw3.h>
#include <SDL.h>
#include <cmath>
#include <glm/glm.hpp>

void PlayerInput::ProcessActor(Player& player, const PlayerMovement& movement)
{
    const bool isNonControlledSoloPlayer =
        !player.GetGame()->GetIsPlayer2Joined() &&
        player.GetGame()->GetPlayers().size() >= 2 &&
        player.GetGame()->GetControlledPlayer() != &player;
    if (isNonControlledSoloPlayer) {
        ClearNonControlledPlayerInput();
        return;
    }

    if (mInputAvailableTimer >= 0.0f) {
        SceneSystem* sceneSystem =
            player.GetGame()->GetSceneSystem();
        if (sceneSystem &&
            sceneSystem->IsWaitingForTutorialPlayerJump()) {
            mJumpPressed = false;
            ApplyTutorialInputRestriction(player);
        }
        return;
    }

    ProcessGameController(player, movement);
    ProcessKeyboard(player, movement);
    ApplyTutorialInputRestriction(player);
    CaptureAttackInput();
}

void PlayerInput::ClearNonControlledPlayerInput()
{
    mMoveForward = 0.0f;
    mMoveLeft = 0.0f;
    mCameraYaw = 0.0f;
    mCameraStickX = 0.0f;
    mCameraStickY = 0.0f;

    mDodgePressed = false;
    mJumpPressed = false;
    mAttackPressed = false;
    mWideAttackPressed = false;
    mSpecialAttackPressed = false;
    mRecoverPressed = false;

    ClearAttackBuffer();
}

void PlayerInput::ApplyTutorialInputRestriction(Player& player)
{
    SceneSystem* sceneSystem =
        player.GetGame()->GetSceneSystem();
    if (!sceneSystem ||
        !sceneSystem->IsWaitingForTutorialPlayerJump()) {
        return;
    }

    mMoveForward = 0.0f;
    mMoveLeft = 0.0f;
    mCameraYaw = 0.0f;

    mDodgePressed = false;
    mAttackPressed = false;
    mWideAttackPressed = false;
    mSpecialAttackPressed = false;
    mRecoverPressed = false;

    ClearAttackBuffer();
}

void PlayerInput::ProcessGameController(Player& player, const PlayerMovement& movement)
{
    Game* game = player.GetGame();

    if (!game->GetIsPlayer2Joined() &&
        game->GetControlledPlayer() != &player) {
        return;
    }




    SDL_GameController* sdlController =
        game->GetIsPlayer2Joined()
            ? game->GetSdlControllerForPlayer(movement.GetPlayerNum())
            : game->GetSdlController();
    if (!sdlController) {
        return;
    }

    constexpr float deadZone = 0.25f;
    constexpr float scale = 1.0f / 32767.0f;

    mMoveForward = SDL_GameControllerGetAxis(sdlController, SDL_CONTROLLER_AXIS_LEFTY) * scale;
    mMoveLeft = SDL_GameControllerGetAxis(sdlController, SDL_CONTROLLER_AXIS_LEFTX) * scale;

    if (std::abs(mMoveForward) < deadZone) {
        mMoveForward = 0.0f;
    }

    if (std::abs(mMoveLeft) < deadZone) {
        mMoveLeft = 0.0f;
    }

    mJumpPressed = SDL_GameControllerGetButton(sdlController, SDL_CONTROLLER_BUTTON_A);
    mAttackPressed = SDL_GameControllerGetButton(sdlController, SDL_CONTROLLER_BUTTON_X);
    mWideAttackPressed = SDL_GameControllerGetButton(sdlController, SDL_CONTROLLER_BUTTON_Y);
    mDodgePressed = SDL_GameControllerGetButton(sdlController, SDL_CONTROLLER_BUTTON_B);
    mSpecialAttackPressed = SDL_GameControllerGetButton(sdlController, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
    mRecoverPressed = mSpecialAttackPressed && mJumpPressed;
}

void PlayerInput::ProcessKeyboard(Player& player, const PlayerMovement& movement)
{
    Game* game = player.GetGame();

    if (game->GetIsPlayer2Joined()) {
        if (game->HasGameControllerForPlayer(movement.GetPlayerNum())) {
            return;
        }
    } else {
        if (game->IsGameControllerConnected() ||
            game->GetControlledPlayer() != &player) {
            return;
        }
    }

    mMoveForward = 0.0f;
    mMoveLeft = 0.0f;
    mCameraYaw = 0.0f;
    mJumpPressed = false;
    mAttackPressed = false;
    mWideAttackPressed = false;
    mDodgePressed = false;
    mSpecialAttackPressed = false;
    mRecoverPressed = false;

    if (game->IsEditorKeyboardInputCaptured()) {
        return;
    }

    GLFWwindow* window = game->GetWindow();

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        mMoveForward -= 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        mMoveForward += 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        mMoveLeft -= 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        mMoveLeft += 1.0f;
    }

    constexpr float cameraKeySpeed = 0.02f;

    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        mCameraYaw += cameraKeySpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        mCameraYaw -= cameraKeySpeed;
    }

    glm::vec2 moveInput(mMoveLeft, mMoveForward);
    if (glm::length(moveInput) > 1.0f) {
        moveInput = glm::normalize(moveInput);
    }

    mMoveLeft = moveInput.x;
    mMoveForward = moveInput.y;

    mJumpPressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    mAttackPressed = glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS;
    mWideAttackPressed = glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS;
    mDodgePressed = glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS;
    mSpecialAttackPressed = glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS;
    mRecoverPressed = mSpecialAttackPressed && mJumpPressed;
}

void PlayerInput::CaptureAttackInput()
{
    // L/Nとの同時押しはスペシャル攻撃用なので、通常攻撃として予約しない。
    if (mSpecialAttackPressed) {
        return;
    }

    const bool normalAttackStarted = mAttackPressed && !mAttackPressedPrev;
    const bool wideAttackStarted = mWideAttackPressed && !mWideAttackPressedPrev;

    if (normalAttackStarted) {
        mBufferedAttackInput = PlayerAttackInputKind::Normal;
        mAttackBufferRemaining = mAttackBufferDuration;
    } else if (wideAttackStarted) {
        mBufferedAttackInput = PlayerAttackInputKind::Wide;
        mAttackBufferRemaining = mAttackBufferDuration;
    }
}

void PlayerInput::EndFrame()
{
    mDodgePressedPrev = mDodgePressed;
    mJumpPressedPrev = mJumpPressed;
    mAttackPressedPrev = mAttackPressed;
    mWideAttackPressedPrev = mWideAttackPressed;
    mSpecialAttackPressedPrev = mSpecialAttackPressed;
    mRecoverPressedPrev = mRecoverPressed;
}

void PlayerInput::UpdateInputAvailableTimer(float deltaTime)
{
    if (mInputAvailableTimer >= 0.0f) {
        mInputAvailableTimer -= deltaTime;
    }
}

void PlayerInput::UpdateAttackBuffer(float deltaTime)
{
    if (mBufferedAttackInput == PlayerAttackInputKind::None) {
        return;
    }

    mAttackBufferRemaining -= deltaTime;
    if (mAttackBufferRemaining <= 0.0f) {
        ClearAttackBuffer();
    }
}

void PlayerInput::ConsumeBufferedAttackInput()
{
    ClearAttackBuffer();
}

void PlayerInput::ClearAttackBuffer()
{
    mBufferedAttackInput = PlayerAttackInputKind::None;
    mAttackBufferRemaining = 0.0f;
}

void PlayerInput::SyncAttackButtonPrev()
{
    mAttackPressedPrev = mAttackPressed;
    mWideAttackPressedPrev = mWideAttackPressed;
    mSpecialAttackPressedPrev = mSpecialAttackPressed;
}
