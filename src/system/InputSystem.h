#pragma once

#include <array>

class Game;
class Player;

class InputSystem {
public:
    explicit InputSystem(Game* game);

    void CaptureFrameInput();
    void ProcessGameInput();
    bool IsMovementInputPressedForPlayer(const Player* player) const;
    bool IsKeyPressed(int key) const;
    bool IsMouseButtonPressed(int button) const;
    int GetControllerAxis(int playerNum, int axis) const;
    bool IsControllerButtonPressed(int playerNum, int button) const;
    bool HasControllerInput(int playerNum) const;
    double GetCursorX() const { return mSnapshot.cursorX; }
    double GetCursorY() const { return mSnapshot.cursorY; }

private:
    void SuppressOneShotInputUntilReleased();
    void SuppressUGCPlayShortcutUntilReleased();
    void UpdateLastUsedInputDevice();
    void ProcessPauseToggleInput();
    void ProcessPauseMenuInput();
    void ProcessDebugReloadInput();
    void ProcessPlayerJoinInput();
    void ProcessPlayerSplitInput();
    void ProcessPlayerSwitchInput();
    void ProcessBattleStyleSelectionInput();
    void ProcessTitleMenuInput();
    void ProcessUGCModeInput();
    void ProcessUGCEditorCursorInput();
    void ProcessUGCEditorCommandInput();
    void ProcessSceneConfirmInput(bool allowsSceneAction);
    void ProcessDebugEditorToggleInput();
    void ProcessFreeCameraToggleInput();
    void ProcessStartInput();

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

    Game* mGame = nullptr;
    InputSnapshot mSnapshot;

    bool mReloadKeyPressedPrev = false;
    bool mUIReloadKeyPressedPrev = false;
    bool mPPressedPrev = false;
    bool mLPressedPrev = false;
    bool mQPressedPrev = false;
    bool mPlayerSplitPressedPrev = false;
    bool mPlayerSwitchPressedPrev = false;
    bool mBattleStyleDirectionPressedPrev = false;
    bool mTitleMenuDirectionPressedPrev = false;
    bool mTitleMenuConfirmPressedPrev = false;
    bool mUGCModePressedPrev = false;
    bool mUGCWorkBrowserPressedPrev = false;
    bool mStartPressedPrev = false;
    bool mPauseMenuKeyPressedPrev = false;
    bool mPauseMenuUpPressedPrev = false;
    bool mPauseMenuDownPressedPrev = false;
    bool mPauseMenuConfirmPressedPrev = false;
    bool mControllerConfirmPressedPrev = false;
    bool mUGCEditorControllerClickPressedPrev = false;
    bool mUGCEditorUndoPressedPrev = false;
    bool mUGCEditorRedoPressedPrev = false;
    bool mUGCEditorEraserPressedPrev = false;
    bool mUGCEditorZoomInPressedPrev = false;
    bool mUGCEditorZoomOutPressedPrev = false;
    bool mUGCEditorLayerDownPressedPrev = false;
    bool mUGCEditorLayerUpPressedPrev = false;
    bool mUGCEditorPlayPressedPrev = false;
    bool mUGCEditorSelectionPressedPrev = false;
    bool mUGCEditorDpadLeftPressedPrev = false;
    bool mUGCEditorDpadRightPressedPrev = false;
    bool mUGCEditorDpadUpPressedPrev = false;
    bool mUGCEditorDpadDownPressedPrev = false;
    bool mKeyboardConfirmPressedPrev = false;
    bool mShouldIgnoreNextSyntheticCursorMotion = false;
    bool mHasPreviousCursorPosition = false;
    double mPreviousCursorX = 0.0;
    double mPreviousCursorY = 0.0;
};
