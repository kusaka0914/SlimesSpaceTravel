#pragma once

#include "system/PlayerInputDeviceRouting.h"
#include <SDL_gamecontroller.h>
#include <array>

struct GLFWwindow;

// OS入力をフレーム単位で保持する。ウィンドウ・機器のハンドルは取得中だけ借用し、保持しない。
class InputSystem {
public:
    // controllersは1P・2Pの順。未接続はnullptrとし、再接続後は最新のハンドルを渡す。
    // フレーム開始時に取得し、次回取得まで問い合わせ結果は変化しない。
    void CaptureFrameInput(GLFWwindow& window, const std::array<SDL_GameController*, 2>& controllers);

    // 割当済みの機器に移動入力があるかを返す。Playerの参加状態・操作権は呼出元で解決する。
    bool IsMovementInputPressed(PlayerInputDevice inputDevice) const;
    bool IsKeyPressed(int key) const;
    bool IsMouseButtonPressed(int button) const;

    // 1始まりのコントローラー番号とSDL軸番号で取得する。範囲外は0。値はSDLの生の軸値。
    int GetControllerAxis(int playerNum, int axis) const;

    // 1始まりのコントローラー番号とSDLボタン番号で取得する。範囲外はfalse。
    bool IsControllerButtonPressed(int playerNum, int button) const;
    bool IsAnyControllerButtonPressed(int button) const;
    bool HasControllerInput(int playerNum) const;
    double GetCursorX() const { return mSnapshot.cursorX; }
    double GetCursorY() const { return mSnapshot.cursorY; }

private:
    struct InputSnapshot {
        std::array<bool, 512> keys{};
        std::array<bool, 16> mouseButtons{};
        std::array<std::array<int, 8>, 2> controllerAxes{};
        std::array<std::array<bool, 32>, 2> controllerButtons{};
        std::array<bool, 2> hasController{};
        double cursorX = 0.0;
        double cursorY = 0.0;
    };

    InputSnapshot mSnapshot;
};
