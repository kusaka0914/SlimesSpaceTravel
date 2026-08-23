#pragma once

#include <SDL.h>

#include <vector>

class GamepadRumbleService {
public:
    ~GamepadRumbleService();

    void Initialize();
    void Shutdown();
    void UpdateConnection();

    void VibrateForPlayer(int playerNum, int lowFrequency, int highFrequency, int duration);

    SDL_GameController* GetController() const { return GetControllerForPlayer(1); }
    SDL_GameController* GetControllerForPlayer(int playerNum) const;
    bool IsConnected() const { return !mControllers.empty(); }
    bool HasControllerForPlayer(int playerNum) const;

private:
    std::vector<SDL_GameController*> mControllers;
};
