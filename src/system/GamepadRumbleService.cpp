#include "system/GamepadRumbleService.h"

GamepadRumbleService::~GamepadRumbleService()
{
    Shutdown();
}

void GamepadRumbleService::Initialize()
{
    if (SDL_Init(SDL_INIT_GAMECONTROLLER) == 0) {
        UpdateConnection();
    }
}

void GamepadRumbleService::Shutdown()
{
    if (mController) {
        SDL_GameControllerClose(mController);
        mController = nullptr;
    }
}

void GamepadRumbleService::UpdateConnection()
{
    if (mController) {
        return;
    }

    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            mController = SDL_GameControllerOpen(i);
            break;
        }
    }
}

void GamepadRumbleService::VibrateForPlayer(int playerNum, int lowFrequency, int highFrequency, int duration)
{
    if (playerNum != 1) {
        return;
    }

    if (!mController) {
        return;
    }

    SDL_GameControllerRumble(mController, lowFrequency, highFrequency, duration);
}
