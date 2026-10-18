#pragma once

#include <array>

class Game;
class GameSessionController;
class InputSystem;
class UIDebugEditorBridge;
class UGCModeController;

// 取得済みの入力をアプリ操作へ振り分け、モードごとの押下履歴を保持する。依存先はすべて借用する。
class GameInputController {
public:
    GameInputController(Game& game, const InputSystem& input, UIDebugEditorBridge& editor,
                        GameSessionController& session, UGCModeController& ugc);

    // CaptureFrameInputとセッション入力の処理後に呼ぶ。既存のEditor・ポーズ・シーン操作の優先順位を保つ。
    void ProcessGameInput();

    // 入力を離すまで単発操作の再発火を抑え、分裂・合体の押し続け状態も解除する。
    void SuppressOneShotInputUntilReleased();

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

    Game& mGame;
    const InputSystem& mInput;
    UIDebugEditorBridge& mEditor;
    GameSessionController& mSession;
    UGCModeController& mUGC;

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
