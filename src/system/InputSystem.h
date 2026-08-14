#pragma once

class Game;
class Player;

class InputSystem {
public:
    explicit InputSystem(Game* game);

    void ProcessGameInput();
    bool IsMovementInputPressedForPlayer(const Player* player) const;

private:
    void SuppressOneShotInputUntilReleased();
    void UpdateLastUsedInputDevice();
    void ProcessPauseToggleInput();
    void ProcessPauseMenuInput();
    void ProcessDebugReloadInput();
    void ProcessPlayerJoinInput();
    void ProcessPlayerSplitInput();
    void ProcessPlayerSwitchInput();
    void ProcessBattleStyleSelectionInput();
    void ProcessSceneConfirmInput(bool allowsSceneAction);
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
    bool mPlayerSplitPressedPrev = false;
    bool mPlayerSwitchPressedPrev = false;
    bool mBattleStyleDirectionPressedPrev = false;
    bool mStartPressedPrev = false;
    bool mPauseMenuKeyPressedPrev = false;
    bool mPauseMenuUpPressedPrev = false;
    bool mPauseMenuDownPressedPrev = false;
    bool mPauseMenuConfirmPressedPrev = false;
    bool mControllerConfirmPressedPrev = false;
    bool mKeyboardConfirmPressedPrev = false;
    bool mHasPreviousCursorPosition = false;
    double mPreviousCursorX = 0.0;
    double mPreviousCursorY = 0.0;
};
