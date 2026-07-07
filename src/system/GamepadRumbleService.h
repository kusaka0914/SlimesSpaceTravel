#pragma once

#include <SDL.h>

class GamepadRumbleService {
public:
    ~GamepadRumbleService();

    void Initialize();
    void Shutdown();
    void UpdateConnection();

    void VibrateForPlayer(int playerNum, int lowFrequency, int highFrequency, int duration);

    SDL_GameController* GetController() const { return mController; }
    bool IsConnected() const { return mController != nullptr; }

private:
    SDL_GameController* mController = nullptr;
};
