#include "system/InputSystem.h"

#include <GLFW/glfw3.h>
#include <SDL.h>
#include <cmath>
#include <cstdlib>

namespace {
constexpr int controllerMovementDeadZone = static_cast<int>(0.25f * 32767.0f);

bool IsControllerMovementPressed(const InputSystem& inputSystem, int playerNum)
{
    if (!inputSystem.HasControllerInput(playerNum)) {
        return false;
    }

    const int horizontalAxis = inputSystem.GetControllerAxis(playerNum, SDL_CONTROLLER_AXIS_LEFTX);
    const int verticalAxis = inputSystem.GetControllerAxis(playerNum, SDL_CONTROLLER_AXIS_LEFTY);

    return std::abs(horizontalAxis) >= controllerMovementDeadZone ||
           std::abs(verticalAxis) >= controllerMovementDeadZone;
}

bool IsKeyboardMovementPressed(const InputSystem& inputSystem)
{
    return inputSystem.IsKeyPressed(GLFW_KEY_W) || inputSystem.IsKeyPressed(GLFW_KEY_A) ||
           inputSystem.IsKeyPressed(GLFW_KEY_S) || inputSystem.IsKeyPressed(GLFW_KEY_D);
}

bool IsSecondPlayerDebugKeyboardMovementPressed(const InputSystem& inputSystem)
{
    return inputSystem.IsKeyPressed(GLFW_KEY_UP) || inputSystem.IsKeyPressed(GLFW_KEY_DOWN) ||
           inputSystem.IsKeyPressed(GLFW_KEY_LEFT) || inputSystem.IsKeyPressed(GLFW_KEY_RIGHT);
}

} // namespace

void InputSystem::CaptureFrameInput(GLFWwindow& window, const std::array<SDL_GameController*, 2>& controllers)
{
    mSnapshot = InputSnapshot{};
    for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key) {
        mSnapshot.keys[static_cast<std::size_t>(key)] = glfwGetKey(&window, key) == GLFW_PRESS;
    }
    for (int button = GLFW_MOUSE_BUTTON_1; button <= GLFW_MOUSE_BUTTON_LAST; ++button) {
        mSnapshot.mouseButtons[static_cast<std::size_t>(button)] = glfwGetMouseButton(&window, button) == GLFW_PRESS;
    }
    glfwGetCursorPos(&window, &mSnapshot.cursorX, &mSnapshot.cursorY);

    for (int playerIndex = 0; playerIndex < 2; ++playerIndex) {
        SDL_GameController* controller = controllers[static_cast<std::size_t>(playerIndex)];
        if (!controller) {
            continue;
        }
        mSnapshot.hasController[static_cast<std::size_t>(playerIndex)] = true;
        for (int axis = 0; axis < SDL_CONTROLLER_AXIS_MAX; ++axis) {
            mSnapshot.controllerAxes[static_cast<std::size_t>(playerIndex)][static_cast<std::size_t>(axis)] =
                static_cast<int>(SDL_GameControllerGetAxis(controller, static_cast<SDL_GameControllerAxis>(axis)));
        }
        for (int button = 0; button < SDL_CONTROLLER_BUTTON_MAX; ++button) {
            mSnapshot.controllerButtons[static_cast<std::size_t>(playerIndex)][static_cast<std::size_t>(button)] =
                SDL_GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(button)) != 0;
        }
    }
}

bool InputSystem::IsKeyPressed(int key) const
{
    return key >= 0 && key < static_cast<int>(mSnapshot.keys.size()) && mSnapshot.keys[static_cast<std::size_t>(key)];
}

bool InputSystem::IsMouseButtonPressed(int button) const
{
    return button >= 0 && button < static_cast<int>(mSnapshot.mouseButtons.size()) &&
           mSnapshot.mouseButtons[static_cast<std::size_t>(button)];
}

int InputSystem::GetControllerAxis(int playerNum, int axis) const
{
    const int playerIndex = playerNum - 1;
    if (playerIndex < 0 || playerIndex >= 2 || axis < 0 || axis >= 8) {
        return 0;
    }
    return mSnapshot.controllerAxes[static_cast<std::size_t>(playerIndex)][static_cast<std::size_t>(axis)];
}

bool InputSystem::IsControllerButtonPressed(int playerNum, int button) const
{
    const int playerIndex = playerNum - 1;
    if (playerIndex < 0 || playerIndex >= 2 || button < 0 || button >= 32) {
        return false;
    }
    return mSnapshot.controllerButtons[static_cast<std::size_t>(playerIndex)][static_cast<std::size_t>(button)];
}

bool InputSystem::IsAnyControllerButtonPressed(int button) const
{
    return IsControllerButtonPressed(1, button) || IsControllerButtonPressed(2, button);
}

bool InputSystem::HasControllerInput(int playerNum) const
{
    const int playerIndex = playerNum - 1;
    return playerIndex >= 0 && playerIndex < 2 && mSnapshot.hasController[static_cast<std::size_t>(playerIndex)];
}

bool InputSystem::IsMovementInputPressed(PlayerInputDevice inputDevice) const
{
    const int controllerPlayerNum = ResolveControllerPlayerNum(inputDevice);
    if (controllerPlayerNum != 0) {
        return IsControllerMovementPressed(*this, controllerPlayerNum);
    }
    if (inputDevice == PlayerInputDevice::PrimaryKeyboard) {
        return IsKeyboardMovementPressed(*this);
    }
    if (inputDevice == PlayerInputDevice::SecondaryKeyboard) {
        return IsSecondPlayerDebugKeyboardMovementPressed(*this);
    }
    return false;
}
