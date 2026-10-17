#pragma once

#include <array>

class Game;
class Player;

// フレーム単位の入力を保持し、シーン・編集モードに応じてゲーム操作へ振り分ける。
class InputSystem {
public:
    explicit InputSystem(Game* game);

    // フレーム開始時に入力機器の状態を取得する。以降の問い合わせは次回取得まで同じ値を参照する。
    void CaptureFrameInput();

    // CaptureFrameInput後に呼び、ゲーム操作と押下履歴を更新する。モードによってStage再読込なども要求する。
    void ProcessGameInput();

    // 単発操作の押下履歴を押下済みにして、入力を離すまで再発火を抑える。
    void SuppressOneShotInputUntilReleased();
    bool IsMovementInputPressedForPlayer(const Player* player) const;
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
    void SuppressUGCPlayShortcutUntilReleased();
    void UpdateLastUsedInputDevice();
    bool ProcessTgsEndShortcutInput();
    void ProcessPauseToggleInput();
    void ProcessPauseMenuInput();
    void SuppressAllPlayerJumpInputUntilReleased();
    void ProcessDebugReloadInput();
    void ProcessPlayerJoinInput();
    void ProcessPlayerSplitInput();
    void ProcessPlayerSwitchInput();
    void ProcessBattleStyleSelectionInput();
    void ProcessTitleMenuInput();
    void ProcessUGCModeInput();
    void ProcessUGCClearResultInput();
    void ProcessUGCEditorCursorInput();
    void ProcessUGCEditorCommandInput();
    bool ProcessSceneConfirmInput(bool allowsSceneAction);
    void ProcessDebugEditorToggleInput();
    void ProcessFreeCameraToggleInput();
    void ProcessStartInput(bool allowsStartAction);

    struct InputSnapshot {
        std::array<bool, 512> keys{};
        std::array<bool, 16> mouseButtons{};
        std::array<std::array<int, 8>, 2> controllerAxes{};
        std::array<std::array<bool, 32>, 2> controllerButtons{};
        std::array<bool, 2> hasController{};
        double cursorX = 0.0;
        double cursorY = 0.0;
    };

    Game* mGame = nullptr;
    InputSnapshot mSnapshot;

    bool mWasReloadKeyPressed = false;
    bool mWasUIReloadKeyPressed = false;
    bool mWasDebugEditorTogglePressed = false;
    bool mWasFreeCameraTogglePressed = false;
    bool mWasPlayerJoinPressed = false;
    bool mWasTgsEndShortcutPressed = false;
    bool mWasTgsNoticeShortcutPressed = false;
    bool mWasPlayerSplitPressed = false;
    bool mWasPlayerSwitchPressed = false;
    bool mWasBattleStyleDirectionPressed = false;
    bool mWasTitleMenuDirectionPressed = false;
    bool mWasTitleMenuConfirmPressed = false;
    bool mWasUGCModePressed = false;
    bool mWasUGCWorkBrowserPressed = false;
    bool mWasUGCClearResultDirectionPressed = false;
    bool mWasUGCClearResultConfirmPressed = false;
    bool mWasStartPressed = false;
    bool mWasPauseMenuKeyPressed = false;
    bool mWasPauseMenuUpPressed = false;
    bool mWasPauseMenuDownPressed = false;
    bool mWasPauseMenuConfirmPressed = false;
    std::array<bool, 2> mWasControllerConfirmPressed{};
    bool mWasUGCEditorCursorActive = false;
    bool mWasUGCEditorControllerClickPressed = false;
    bool mWasUGCEditorUndoPressed = false;
    bool mWasUGCEditorRedoPressed = false;
    bool mWasUGCEditorEraserPressed = false;
    bool mWasUGCEditorZoomInPressed = false;
    bool mWasUGCEditorZoomOutPressed = false;
    bool mWasUGCEditorLayerDownPressed = false;
    bool mWasUGCEditorLayerUpPressed = false;
    bool mWasUGCEditorPlayPressed = false;
    bool mWasUGCEditorSelectionPressed = false;
    bool mWasUGCEditorDpadLeftPressed = false;
    bool mWasUGCEditorDpadRightPressed = false;
    bool mWasUGCEditorDpadUpPressed = false;
    bool mWasUGCEditorDpadDownPressed = false;
    bool mWasKeyboardConfirmPressed = false;
    bool mShouldIgnoreNextSyntheticCursorMotion = false;
    bool mHasPreviousCursorPosition = false;
    double mPreviousCursorX = 0.0;
    double mPreviousCursorY = 0.0;
};
