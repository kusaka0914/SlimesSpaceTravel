#include "system/GamepadRumbleService.h"

GamepadRumbleService::~GamepadRumbleService()
{
    Shutdown();
}

void GamepadRumbleService::Initialize()
{
    // SDL can expose each Joy-Con as a separate mini gamepad. Enable its
    // HIDAPI pairing before the controller subsystem starts so a connected
    // left/right pair occupies one local-player controller slot, while a
    // single Joy-Con still remains usable on its own.
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_JOY_CONS, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_COMBINE_JOY_CONS, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_VERTICAL_JOY_CONS, "0");
    if (SDL_Init(SDL_INIT_GAMECONTROLLER) == 0) {
        UpdateConnection();
    }
}

void GamepadRumbleService::Shutdown()
{
    for (SDL_GameController* controller : mControllers) {
        if (controller) {
            SDL_GameControllerClose(controller);
        }
    }
    mControllers.clear();
}

void GamepadRumbleService::UpdateConnection()
{
    int connectedControllerCount = 0;
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            ++connectedControllerCount;
        }
    }

    if (connectedControllerCount == static_cast<int>(mControllers.size())) {
        return;
    }

    Shutdown();
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            if (SDL_GameController* controller = SDL_GameControllerOpen(i)) {
                mControllers.push_back(controller);
            }
        }
    }
}

SDL_GameController* GamepadRumbleService::GetControllerForPlayer(
    int playerNum) const
{
    const int controllerIndex = playerNum - 1;
    if (controllerIndex < 0 ||
        controllerIndex >= static_cast<int>(mControllers.size())) {
        return nullptr;
    }
    return mControllers[static_cast<std::size_t>(controllerIndex)];
}

bool GamepadRumbleService::HasControllerForPlayer(int playerNum) const
{
    return GetControllerForPlayer(playerNum) != nullptr;
}

void GamepadRumbleService::VibrateForPlayer(int playerNum, int lowFrequency, int highFrequency, int duration)
{
    SDL_GameController* controller = GetControllerForPlayer(playerNum);
    if (!controller) {
        return;
    }

    SDL_GameControllerRumble(controller, lowFrequency, highFrequency, duration);
}
