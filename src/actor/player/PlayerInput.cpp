#include "actor/player/PlayerInput.h"

#include "actor/Player.h"
#include "actor/player/PlayerMovement.h"

#include <GLFW/glfw3.h>
#include <SDL.h>
#include <cmath>
#include <glm/glm.hpp>

void PlayerInput::ProcessActor(Player& player, const PlayerMovement& movement)
{
    if (mInputAvailableTimer >= 0.0f) {
        return;
    }

    ProcessGameController(player, movement);
    ProcessKeyboard(player, movement);
}

void PlayerInput::ProcessGameController(Player& player, const PlayerMovement& movement)
{
    if (!player.GetGame()->IsGameControllerConnected() || movement.GetPlayerNum() != 1) {
        return;
    }

    SDL_GameController* sdlController = player.GetGame()->GetSdlController();

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
    mRecoverPressed = SDL_GameControllerGetButton(sdlController, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
}

void PlayerInput::ProcessKeyboard(Player& player, const PlayerMovement& movement)
{
    const bool isControllerConnected = player.GetGame()->IsGameControllerConnected();

    if (!isControllerConnected && movement.GetPlayerNum() != 1) {
        return;
    }

    if (isControllerConnected && movement.GetPlayerNum() != 2) {
        return;
    }

    GLFWwindow* window = player.GetGame()->GetWindow();

    mMoveForward = 0.0f;
    mMoveLeft = 0.0f;
    mCameraYaw = 0.0f;

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
    mRecoverPressed = glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS;
}

void PlayerInput::EndFrame()
{
    mDodgePressedPrev = mDodgePressed;
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

void PlayerInput::SyncAttackButtonPrev()
{
    mAttackPressedPrev = mAttackPressed;
    mWideAttackPressedPrev = mWideAttackPressed;
    mSpecialAttackPressedPrev = mSpecialAttackPressed;
}
