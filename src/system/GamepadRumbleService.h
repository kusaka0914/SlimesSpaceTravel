#pragma once

#include <SDL.h>

#include <vector>

// 接続中のSDLコントローラーを保持し、Player番号に応じた振動を送る。
class GamepadRumbleService {
public:
    ~GamepadRumbleService();

    void Initialize();
    void Shutdown();

    // 接続台数が変わった場合は、保持中のコントローラーハンドルを開き直す。
    void UpdateConnection();

    // 1始まりのPlayer番号へ振動を送る。未接続なら何もしない。強度には0〜65535を指定する。
    void VibrateForPlayer(int playerNum, int lowFrequency, int highFrequency, int durationMilliseconds);

    SDL_GameController* GetController() const { return GetControllerForPlayer(1); }

    // 1始まりの番号に対応する借用ハンドルを返す。未接続ならnullptr。再接続・終了で無効になる。
    SDL_GameController* GetControllerForPlayer(int playerNum) const;
    bool IsConnected() const { return !mControllers.empty(); }
    bool HasControllerForPlayer(int playerNum) const;

private:
    std::vector<SDL_GameController*> mControllers;
};
