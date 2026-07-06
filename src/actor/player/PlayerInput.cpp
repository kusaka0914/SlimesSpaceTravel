#include "actor/player/PlayerInput.h"

#include "actor/Player.h"
#include "actor/player/PlayerModuleContext.h"
#include "actor/player/PlayerMovement.h"

#include <GLFW/glfw3.h>
#include <SDL.h>
#include <cmath>
#include <glm/glm.hpp>

void PlayerInput::ProcessActor(PlayerModuleContext& context)
{
    if (inputAvailableTimer >= 0.0f) {
        return;
    }

    ProcessGameController(context);
    ProcessKeyboard(context);
}

void PlayerInput::ProcessGameController(PlayerModuleContext& context)
{
    Player& player = context.player;
    PlayerMovement& movement = context.movement;

    if (!player.GetGame()->IsGameControllerConnected() || movement.playerNum != 1) {
        return;
    }

    SDL_GameController* sdlController = player.GetGame()->GetSdlController();

    constexpr float deadZone = 0.25f;
    constexpr float scale = 1.0f / 32767.0f;

    moveForward = SDL_GameControllerGetAxis(sdlController, SDL_CONTROLLER_AXIS_LEFTY) * scale;
    moveLeft = SDL_GameControllerGetAxis(sdlController, SDL_CONTROLLER_AXIS_LEFTX) * scale;

    if (std::abs(moveForward) < deadZone) {
        moveForward = 0.0f;
    }

    if (std::abs(moveLeft) < deadZone) {
        moveLeft = 0.0f;
    }

    jumpPressed = SDL_GameControllerGetButton(sdlController, SDL_CONTROLLER_BUTTON_A);
    attackPressed = SDL_GameControllerGetButton(sdlController, SDL_CONTROLLER_BUTTON_X);
    wideAttackPressed = SDL_GameControllerGetButton(sdlController, SDL_CONTROLLER_BUTTON_Y);
    dodgePressed = SDL_GameControllerGetButton(sdlController, SDL_CONTROLLER_BUTTON_B);
    specialAttackPressed = SDL_GameControllerGetButton(sdlController, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
    recoverPressed = SDL_GameControllerGetButton(sdlController, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
}

void PlayerInput::ProcessKeyboard(PlayerModuleContext& context)
{
    Player& player = context.player;
    PlayerMovement& movement = context.movement;

    const bool isControllerConnected = player.GetGame()->IsGameControllerConnected();

    if (!isControllerConnected && movement.playerNum != 1) {
        return;
    }

    if (isControllerConnected && movement.playerNum != 2) {
        return;
    }

    GLFWwindow* window = player.GetGame()->GetWindow();

    moveForward = 0.0f;
    moveLeft = 0.0f;
    cameraYaw = 0.0f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        moveForward -= 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        moveForward += 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        moveLeft -= 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        moveLeft += 1.0f;
    }

    constexpr float cameraKeySpeed = 0.02f;

    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        cameraYaw += cameraKeySpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        cameraYaw -= cameraKeySpeed;
    }

    glm::vec2 moveInput(moveLeft, moveForward);
    if (glm::length(moveInput) > 1.0f) {
        moveInput = glm::normalize(moveInput);
    }

    moveLeft = moveInput.x;
    moveForward = moveInput.y;

    jumpPressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    attackPressed = glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS;
    wideAttackPressed = glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS;
    dodgePressed = glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS;
    specialAttackPressed = glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS;
    recoverPressed = glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS;
}