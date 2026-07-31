#pragma once

class Game;

class InputSystem {
public:
    explicit InputSystem(Game* game);

    void ProcessGameInput();

private:
    void ProcessPauseToggleInput();
    void ProcessPauseMenuInput();
    void ProcessDebugReloadInput();
    void ProcessPlayerJoinInput();
    void ProcessPlayerSwitchInput();
    void ProcessSceneConfirmInput();
    void ProcessDebugEditorToggleInput();
    void ProcessFreeCameraToggleInput();
    void ProcessStartInput();

private:
    Game* mGame = nullptr;

    bool mReloadKeyPressedPrev = false;
    bool mUIReloadKeyPressedPrev = false;
    bool mPPressedPrev = false;
    bool mLPressedPrev = false;
    bool mQPressedPrev = false;
    bool mPlayerSwitchPressedPrev = false;
    bool mStartPressedPrev = false;
    bool mPauseMenuKeyPressedPrev = false;
    bool mPauseMenuUpPressedPrev = false;
    bool mPauseMenuDownPressedPrev = false;
    bool mPauseMenuConfirmPressedPrev = false;
    bool mControllerConfirmPressedPrev = false;
    bool mKeyboardConfirmPressedPrev = false;
};
