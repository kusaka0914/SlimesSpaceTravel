#include "app/GameInputController.h"

#include "actor/Player.h"
#include "app/Game.h"
#include "app/GameSessionController.h"
#include "editor/core/UIDebugEditorBridge.h"
#include "system/CameraSystem.h"
#include "system/InputSystem.h"
#include "system/SceneSystem.h"
#include "system/UGCModeController.h"

#include "imgui.h"
#include <GLFW/glfw3.h>
#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <iostream>

namespace {
bool IsKeyboardOrMouseInputActive(const InputSystem& inputSystem)
{
    constexpr std::array<int, 30> trackedKeys = {
        GLFW_KEY_W,    GLFW_KEY_A,     GLFW_KEY_S,      GLFW_KEY_D,       GLFW_KEY_UP,        GLFW_KEY_DOWN,
        GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_SPACE,  GLFW_KEY_J,       GLFW_KEY_K,         GLFW_KEY_M,
        GLFW_KEY_U,    GLFW_KEY_Q,     GLFW_KEY_E,      GLFW_KEY_X,       GLFW_KEY_Z,         GLFW_KEY_N,
        GLFW_KEY_I,    GLFW_KEY_O,     GLFW_KEY_Y,      GLFW_KEY_PAGE_UP, GLFW_KEY_PAGE_DOWN, GLFW_KEY_TAB,
        GLFW_KEY_F5,   GLFW_KEY_ENTER, GLFW_KEY_ESCAPE, GLFW_KEY_P,       GLFW_KEY_L,         GLFW_KEY_F,
    };
    for (const int key : trackedKeys) {
        if (inputSystem.IsKeyPressed(key)) {
            return true;
        }
    }

    for (int button = GLFW_MOUSE_BUTTON_1; button <= GLFW_MOUSE_BUTTON_LAST; ++button) {
        if (inputSystem.IsMouseButtonPressed(button)) {
            return true;
        }
    }
    return false;
}

bool IsGameControllerInputActive(const InputSystem& inputSystem)
{
    for (int playerNum = 1; playerNum <= 2; ++playerNum) {
        if (!inputSystem.HasControllerInput(playerNum)) {
            continue;
        }

        for (int button = 0; button < SDL_CONTROLLER_BUTTON_MAX; ++button) {
            if (inputSystem.IsControllerButtonPressed(playerNum, button)) {
                return true;
            }
        }

        constexpr Sint16 axisActivityThreshold = 8000;
        for (int axis = 0; axis < SDL_CONTROLLER_AXIS_MAX; ++axis) {
            const auto controllerAxis = static_cast<SDL_GameControllerAxis>(axis);
            const int axisValue = inputSystem.GetControllerAxis(playerNum, controllerAxis);
            const bool isTrigger =
                controllerAxis == SDL_CONTROLLER_AXIS_TRIGGERLEFT || controllerAxis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
            if ((isTrigger && axisValue > axisActivityThreshold) ||
                (!isTrigger && std::abs(static_cast<int>(axisValue)) > axisActivityThreshold)) {
                return true;
            }
        }
    }
    return false;
}

void SuppressPlayerJumpUntilReleased(Game& game, int playerNum)
{
    for (Player* player : game.GetPlayers()) {
        if (player && player->GetPlayerNum() == playerNum) {
            player->SuppressJumpUntilReleased();
            return;
        }
    }
}
} // namespace

GameInputController::GameInputController(Game& game, const InputSystem& input, UIDebugEditorBridge& editor,
                                         GameSessionController& session, UGCModeController& ugc)
    : mGame(game),
      mInput(input),
      mEditor(editor),
      mSession(session),
      mUGC(ugc)
{
}

void GameInputController::ProcessGameInput()
{
    if (!mGame.GetWindow()) {
        return;
    }

    if (mGame.IsEditorKeyboardInputCaptured()) {
        UpdateLastUsedInputDevice();
        ProcessUGCEditorCursorInput();
        SuppressOneShotInputUntilReleased();
        return;
    }

    UpdateLastUsedInputDevice();

    if (ProcessTgsEndShortcutInput()) {
        return;
    }

    if (mUGC.IsClearResultShowing()) {
        ProcessUGCClearResultInput();
        ProcessStartInput(false);
        return;
    }

    ProcessDebugEditorToggleInput();
    ProcessUGCEditorCursorInput();
    const bool isUGCEditorActive =
        mUGC.IsModeActive() && mGame.GetIsDebugEditorShowing() && !mUGC.IsDebugPanelShowing();
    if (isUGCEditorActive) {
        ProcessUGCEditorCommandInput();
        return;
    }
    ProcessPlayAndMenuInput();
}

void GameInputController::ProcessPlayAndMenuInput()
{
    ProcessPauseToggleInput();

    const bool wasPauseMenuOpen = mGame.GetIsPauseMenuOpen();
    if (wasPauseMenuOpen) {
        ProcessPauseMenuInput();
    }

    ProcessDebugReloadInput();
    ProcessPlayerJoinInput();
    ProcessPlayerSplitInput();
    ProcessPlayerSwitchInput();
    const bool wasTitleMenuActive =
        mGame.GetSceneSystem() && mGame.GetSceneSystem()->IsTitle() && !mUGC.IsWorkBrowserShowing();
    ProcessUGCModeInput();
    ProcessTitleMenuInput();
    ProcessBattleStyleSelectionInput();

    // ポーズ決定もAを使うため、そのフレームの入力を会話開始へ流さない。
    const bool didExecuteSceneConfirm = ProcessSceneConfirmInput(!wasPauseMenuOpen && !wasTitleMenuActive);
    ProcessFreeCameraToggleInput();
    const bool allowsStartAction =
        !wasPauseMenuOpen && !wasTitleMenuActive && !mUGC.IsWorkBrowserShowing() && !didExecuteSceneConfirm;
    ProcessStartInput(allowsStartAction);
}

void GameInputController::SuppressOneShotInputUntilReleased()
{
    mGame.SetPlayerSplitMergeButtonHeld(false);
    mWasReloadKeyPressed = true;
    mWasUIReloadKeyPressed = true;
    mWasDebugEditorTogglePressed = true;
    mWasFreeCameraTogglePressed = true;
    mWasPlayerJoinPressed = true;
    mWasTgsEndShortcutPressed = true;
    mWasTgsNoticeShortcutPressed = true;
    mWasPlayerSplitPressed = true;
    mWasPlayerSwitchPressed = true;
    mWasBattleStyleDirectionPressed = true;
    mWasTitleMenuDirectionPressed = true;
    mWasTitleMenuConfirmPressed = true;
    mWasUGCModePressed = true;
    mWasUGCWorkBrowserPressed = true;
    mWasUGCClearResultDirectionPressed = true;
    mWasUGCClearResultConfirmPressed = true;
    mWasStartPressed = true;
    mWasPauseMenuKeyPressed = true;
    mWasPauseMenuUpPressed = true;
    mWasPauseMenuDownPressed = true;
    mWasPauseMenuConfirmPressed = true;
    mWasControllerConfirmPressed.fill(true);
    mWasUGCEditorUndoPressed = true;
    mWasUGCEditorRedoPressed = true;
    mWasUGCEditorEraserPressed = true;
    mWasUGCEditorZoomInPressed = true;
    mWasUGCEditorZoomOutPressed = true;
    mWasUGCEditorLayerDownPressed = true;
    mWasUGCEditorLayerUpPressed = true;
    mWasUGCEditorPlayPressed = true;
    mWasUGCEditorSelectionPressed = true;
    mWasUGCEditorDpadLeftPressed = true;
    mWasUGCEditorDpadRightPressed = true;
    mWasUGCEditorDpadUpPressed = true;
    mWasUGCEditorDpadDownPressed = true;
    mWasKeyboardConfirmPressed = true;
}

void GameInputController::SuppressUGCPlayShortcutUntilReleased()
{
    mWasPauseMenuKeyPressed = true;
    mWasUGCEditorPlayPressed = true;
}

void GameInputController::ProcessUGCModeInput()
{
    SceneSystem* sceneSystem = mGame.GetSceneSystem();
    if (!sceneSystem || !sceneSystem->IsTitle()) {
        mWasUGCModePressed = false;
        mWasUGCWorkBrowserPressed = false;
        return;
    }

    if (mUGC.IsWorkBrowserShowing()) {
        mWasUGCModePressed = false;
        mWasUGCWorkBrowserPressed = false;
        return;
    }

    const bool keyboardPressed = mInput.IsKeyPressed(GLFW_KEY_C);
    const bool controllerPressed = mInput.IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_X);
    const bool ugcModePressed = keyboardPressed || controllerPressed;

    if (ugcModePressed && !mWasUGCModePressed) {
        mUGC.StartMode();
    }
    mWasUGCModePressed = ugcModePressed;

    const bool browserKeyboardPressed = mInput.IsKeyPressed(GLFW_KEY_V);
    const bool browserControllerPressed = mInput.IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_Y);
    const bool browserPressed = browserKeyboardPressed || browserControllerPressed;
    if (browserPressed && !mWasUGCWorkBrowserPressed) {
        mUGC.OpenWorkBrowser();
    }
    mWasUGCWorkBrowserPressed = browserPressed;
}

void GameInputController::ProcessUGCClearResultInput()
{
    constexpr Sint16 directionThreshold = 16000;
    const bool upPressed = mInput.IsKeyPressed(GLFW_KEY_UP) || mInput.IsKeyPressed(GLFW_KEY_W) ||
                           mInput.IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_UP) ||
                           mInput.GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTY) < -directionThreshold ||
                           mInput.GetControllerAxis(2, SDL_CONTROLLER_AXIS_LEFTY) < -directionThreshold;
    const bool downPressed = mInput.IsKeyPressed(GLFW_KEY_DOWN) || mInput.IsKeyPressed(GLFW_KEY_S) ||
                             mInput.IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_DOWN) ||
                             mInput.GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTY) > directionThreshold ||
                             mInput.GetControllerAxis(2, SDL_CONTROLLER_AXIS_LEFTY) > directionThreshold;
    const bool directionPressed = upPressed || downPressed;
    if (directionPressed && !mWasUGCClearResultDirectionPressed) {
        mUGC.MoveClearResultSelection(upPressed ? -1 : 1);
    }
    mWasUGCClearResultDirectionPressed = directionPressed;

    const bool confirmPressed = mInput.IsKeyPressed(GLFW_KEY_SPACE) || mInput.IsKeyPressed(GLFW_KEY_ENTER) ||
                                mInput.IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_A);
    if (confirmPressed && !mWasUGCClearResultConfirmPressed) {
        mUGC.ExecuteClearResultSelection();
    }
    mWasUGCClearResultConfirmPressed = confirmPressed;
}

void GameInputController::ProcessTitleMenuInput()
{
    SceneSystem* sceneSystem = mGame.GetSceneSystem();
    if (!sceneSystem || !sceneSystem->IsTitle() || mUGC.IsWorkBrowserShowing()) {
        mWasTitleMenuDirectionPressed = false;
        mWasTitleMenuConfirmPressed = false;
        return;
    }

    constexpr Sint16 directionThreshold = 16000;
    const bool upPressed = mInput.IsKeyPressed(GLFW_KEY_UP) || mInput.IsKeyPressed(GLFW_KEY_W) ||
                           mInput.IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_UP) ||
                           mInput.GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTY) < -directionThreshold ||
                           mInput.GetControllerAxis(2, SDL_CONTROLLER_AXIS_LEFTY) < -directionThreshold;
    const bool downPressed = mInput.IsKeyPressed(GLFW_KEY_DOWN) || mInput.IsKeyPressed(GLFW_KEY_S) ||
                             mInput.IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_DOWN) ||
                             mInput.GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTY) > directionThreshold ||
                             mInput.GetControllerAxis(2, SDL_CONTROLLER_AXIS_LEFTY) > directionThreshold;
    const bool directionPressed = upPressed || downPressed;
    if (directionPressed && !mWasTitleMenuDirectionPressed) {
        mSession.MoveTitleMenuSelection(upPressed ? -1 : 1);
    }
    mWasTitleMenuDirectionPressed = directionPressed;

    const bool confirmPressed = mInput.IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_A) ||
                                mInput.IsKeyPressed(GLFW_KEY_SPACE) || mInput.IsKeyPressed(GLFW_KEY_ENTER);
    if (confirmPressed && !mWasTitleMenuConfirmPressed) {
        mSession.ExecuteTitleMenuSelection();
    }
    mWasTitleMenuConfirmPressed = confirmPressed;
}

void GameInputController::ProcessUGCEditorCursorInput()
{
    const bool isUGCEditorActive = mUGC.IsModeActive() && mGame.GetIsDebugEditorShowing() &&
                                   !mUGC.IsDebugPanelShowing() && mInput.HasControllerInput(1);
    if (!isUGCEditorActive) {
        if (mWasUGCEditorControllerClickPressed && ImGui::GetCurrentContext()) {
            const bool isPhysicalLeftMousePressed = mInput.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
            ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Left, isPhysicalLeftMousePressed);
        }
        mWasUGCEditorCursorActive = false;
        mWasUGCEditorControllerClickPressed = false;
        return;
    }

    const bool isControllerClickPressed = mInput.IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_A);
    if (!mWasUGCEditorCursorActive) {
        // タイトル決定に使ったAを、編集画面の最初のボタンへ流さない。
        mWasUGCEditorCursorActive = true;
        mWasUGCEditorControllerClickPressed = isControllerClickPressed;
        return;
    }
    if (isControllerClickPressed != mWasUGCEditorControllerClickPressed && ImGui::GetCurrentContext()) {
        const bool isPhysicalLeftMousePressed = mInput.IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
        ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Left,
                                           isControllerClickPressed || isPhysicalLeftMousePressed);
    }
    mWasUGCEditorControllerClickPressed = isControllerClickPressed;

    constexpr float axisScale = 1.0f / 32767.0f;
    constexpr float deadZone = 0.2f;
    float previewTurnInput = mInput.GetControllerAxis(1, SDL_CONTROLLER_AXIS_RIGHTX) * axisScale;
    if (std::abs(previewTurnInput) < deadZone) {
        previewTurnInput = 0.0f;
    }
    if (previewTurnInput != 0.0f) {
        constexpr float previewTurnRadiansPerSecond = 1.8f;
        const float deltaTime = std::max(0.001f, mGame.GetLastDeltaTime());
        mGame.AdjustUGCPreviewYaw(-previewTurnInput * previewTurnRadiansPerSecond * deltaTime);
    }

    if (!mGame.GetCameraSystem()) {
        return;
    }

    float horizontalInput = mInput.GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTX) * axisScale;
    float verticalInput = mInput.GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTY) * axisScale;
    if (std::abs(horizontalInput) < deadZone) {
        horizontalInput = 0.0f;
    }
    if (std::abs(verticalInput) < deadZone) {
        verticalInput = 0.0f;
    }
    if (horizontalInput == 0.0f && verticalInput == 0.0f) {
        return;
    }

    GLFWwindow* window = mGame.GetWindow();
    int windowWidth = 0;
    int windowHeight = 0;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    if (windowWidth <= 0 || windowHeight <= 0) {
        return;
    }
    double cursorX = mInput.GetCursorX();
    double cursorY = mInput.GetCursorY();

    constexpr float normalCursorSpeedPixelsPerSecond = 650.0f;
    const float deltaTime = std::max(0.001f, mGame.GetLastDeltaTime());
    cursorX += horizontalInput * normalCursorSpeedPixelsPerSecond * deltaTime;
    cursorY += verticalInput * normalCursorSpeedPixelsPerSecond * deltaTime;
    cursorX = std::clamp(cursorX, 0.0, static_cast<double>(windowWidth - 1));
    cursorY = std::clamp(cursorY, 0.0, static_cast<double>(windowHeight - 1));
    mShouldIgnoreNextSyntheticCursorMotion = true;
    glfwSetCursorPos(window, cursorX, cursorY);

    constexpr float edgeScrollAreaPixels = 24.0f;
    glm::vec2 edgeScrollDirection(0.0f);
    if (cursorX <= edgeScrollAreaPixels && horizontalInput < 0.0f) {
        edgeScrollDirection.x = -1.0f;
    } else if (cursorX >= windowWidth - edgeScrollAreaPixels && horizontalInput > 0.0f) {
        edgeScrollDirection.x = 1.0f;
    }
    if (cursorY <= edgeScrollAreaPixels && verticalInput < 0.0f) {
        edgeScrollDirection.y = 1.0f;
    } else if (cursorY >= windowHeight - edgeScrollAreaPixels && verticalInput > 0.0f) {
        edgeScrollDirection.y = -1.0f;
    }
    if (edgeScrollDirection == glm::vec2(0.0f)) {
        return;
    }

    CameraPose cameraPose = mGame.GetCameraSystem()->GetDebugCameraPose();
    const glm::vec3 forward = glm::normalize(cameraPose.target - cameraPose.position);
    const glm::vec3 right = glm::normalize(glm::cross(forward, cameraPose.up));
    constexpr float normalViewScrollWorldUnitsPerSecond = 10.0f;
    const glm::vec3 viewMovement = (right * edgeScrollDirection.x + cameraPose.up * edgeScrollDirection.y) *
                                   normalViewScrollWorldUnitsPerSecond * deltaTime;
    cameraPose.position += viewMovement;
    cameraPose.target += viewMovement;
    mGame.GetCameraSystem()->SetDebugCameraPose(cameraPose);
}

void GameInputController::ProcessUGCEditorCommandInput()
{
    const bool undoPressed =
        mInput.IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_B) || mInput.IsKeyPressed(GLFW_KEY_U);
    const bool redoPressed =
        mInput.IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_Y) || mInput.IsKeyPressed(GLFW_KEY_J);
    const bool eraserPressed =
        mInput.IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_X) || mInput.IsKeyPressed(GLFW_KEY_K);
    const bool zoomInPressed =
        mInput.IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) || mInput.IsKeyPressed(GLFW_KEY_M);
    const bool zoomOutPressed =
        mInput.IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_LEFTSHOULDER) || mInput.IsKeyPressed(GLFW_KEY_N);
    constexpr Sint16 triggerPressedThreshold = 16000;
    const bool layerDownPressed =
        mInput.GetControllerAxis(1, SDL_CONTROLLER_AXIS_TRIGGERLEFT) > triggerPressedThreshold ||
        mInput.IsKeyPressed(GLFW_KEY_Y);
    const bool layerUpPressed =
        mInput.GetControllerAxis(1, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > triggerPressedThreshold ||
        mInput.IsKeyPressed(GLFW_KEY_I);
    const bool playPressed =
        mInput.IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_BACK) || mInput.IsKeyPressed(GLFW_KEY_ESCAPE);
    const bool selectionPressed =
        mInput.IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_START) || mInput.IsKeyPressed(GLFW_KEY_ENTER);
    const bool dpadLeftPressed = mInput.IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    const bool dpadRightPressed = mInput.IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
    const bool dpadUpPressed = mInput.IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_DPAD_UP);
    const bool dpadDownPressed = mInput.IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_DPAD_DOWN);

    float previewTurnInput = 0.0f;
    if (mInput.IsKeyPressed(GLFW_KEY_LEFT)) {
        previewTurnInput = -1.0f;
    } else if (mInput.IsKeyPressed(GLFW_KEY_RIGHT)) {
        previewTurnInput = 1.0f;
    }
    if (previewTurnInput != 0.0f) {
        constexpr float previewTurnRadiansPerSecond = 1.8f;
        const float deltaTime = std::max(0.001f, mGame.GetLastDeltaTime());
        mGame.AdjustUGCPreviewYaw(-previewTurnInput * previewTurnRadiansPerSecond * deltaTime);
    }

    if (undoPressed && !mWasUGCEditorUndoPressed) {
        mEditor.HandleUndo();
    }
    if (redoPressed && !mWasUGCEditorRedoPressed) {
        mEditor.HandleRedo();
    }
    if (eraserPressed && !mWasUGCEditorEraserPressed) {
        mEditor.ToggleEraser();
    }
    if (zoomInPressed && !mWasUGCEditorZoomInPressed) {
        mEditor.AdjustZoom(0.8f);
    }
    if (zoomOutPressed && !mWasUGCEditorZoomOutPressed) {
        mEditor.AdjustZoom(1.25f);
    }
    if (layerDownPressed && !mWasUGCEditorLayerDownPressed) {
        mEditor.ChangeLayer(-1);
    }
    if (layerUpPressed && !mWasUGCEditorLayerUpPressed) {
        mEditor.ChangeLayer(1);
    }
    if (selectionPressed && !mWasUGCEditorSelectionPressed) {
        mEditor.ActivateSelectionMode();
    }
    if (playPressed && !mWasUGCEditorPlayPressed) {
        mUGC.StartPlaytest();
    }
    if (dpadLeftPressed && !mWasUGCEditorDpadLeftPressed) {
        mEditor.MoveSelectionOnGrid(-1, 0);
    }
    if (dpadRightPressed && !mWasUGCEditorDpadRightPressed) {
        mEditor.MoveSelectionOnGrid(1, 0);
    }
    if (dpadUpPressed && !mWasUGCEditorDpadUpPressed) {
        mEditor.MoveSelectionOnGrid(0, -1);
    }
    if (dpadDownPressed && !mWasUGCEditorDpadDownPressed) {
        mEditor.MoveSelectionOnGrid(0, 1);
    }

    mWasUGCEditorUndoPressed = undoPressed;
    mWasUGCEditorRedoPressed = redoPressed;
    mWasUGCEditorEraserPressed = eraserPressed;
    mWasUGCEditorZoomInPressed = zoomInPressed;
    mWasUGCEditorZoomOutPressed = zoomOutPressed;
    mWasUGCEditorLayerDownPressed = layerDownPressed;
    mWasUGCEditorLayerUpPressed = layerUpPressed;
    mWasUGCEditorPlayPressed = playPressed;
    mWasUGCEditorSelectionPressed = selectionPressed;
    mWasUGCEditorDpadLeftPressed = dpadLeftPressed;
    mWasUGCEditorDpadRightPressed = dpadRightPressed;
    mWasUGCEditorDpadUpPressed = dpadUpPressed;
    mWasUGCEditorDpadDownPressed = dpadDownPressed;

    mWasPauseMenuKeyPressed = playPressed;
    mWasStartPressed = selectionPressed;
}

void GameInputController::ProcessPauseToggleInput()
{
    SceneSystem* sceneSystem = mGame.GetSceneSystem();
    if (!sceneSystem) {
        return;
    }

    const bool escapePressed = mInput.IsKeyPressed(GLFW_KEY_ESCAPE);
    const bool controllerBackPressed = mInput.IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_BACK);

    const bool returnToUGCEditorPressed = escapePressed || controllerBackPressed;
    if (returnToUGCEditorPressed && !mWasPauseMenuKeyPressed && mUGC.IsPlaytestActive()) {
        mUGC.ReturnToEditor();
        SuppressUGCPlayShortcutUntilReleased();
        return;
    }

    const bool pauseMenuKeyPressed = escapePressed || controllerBackPressed;

    if (pauseMenuKeyPressed && !mWasPauseMenuKeyPressed && (mGame.GetIsPauseMenuOpen() || mGame.CanOpenPauseMenu())) {
        mGame.TogglePauseMenu();
    }

    mWasPauseMenuKeyPressed = pauseMenuKeyPressed;
}

void GameInputController::ProcessPauseMenuInput()
{
    constexpr Sint16 directionThreshold = 16000;
    const bool upPressed = mInput.IsKeyPressed(GLFW_KEY_UP) || mInput.IsKeyPressed(GLFW_KEY_W) ||
                           mInput.IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_UP) ||
                           mInput.GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTY) < -directionThreshold ||
                           mInput.GetControllerAxis(2, SDL_CONTROLLER_AXIS_LEFTY) < -directionThreshold;

    const bool downPressed = mInput.IsKeyPressed(GLFW_KEY_DOWN) || mInput.IsKeyPressed(GLFW_KEY_S) ||
                             mInput.IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_DOWN) ||
                             mInput.GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTY) > directionThreshold ||
                             mInput.GetControllerAxis(2, SDL_CONTROLLER_AXIS_LEFTY) > directionThreshold;

    const bool confirmPressed = mInput.IsKeyPressed(GLFW_KEY_SPACE) || mInput.IsKeyPressed(GLFW_KEY_ENTER) ||
                                mInput.IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_A);

    if (upPressed && !mWasPauseMenuUpPressed) {
        mGame.MovePauseMenuSelection(-1);
    }

    if (downPressed && !mWasPauseMenuDownPressed) {
        mGame.MovePauseMenuSelection(1);
    }

    if (confirmPressed && !mWasPauseMenuConfirmPressed) {
        mGame.ExecutePauseMenuItem();
        SuppressAllPlayerJumpInputUntilReleased();
    }

    mWasPauseMenuUpPressed = upPressed;
    mWasPauseMenuDownPressed = downPressed;
    mWasPauseMenuConfirmPressed = confirmPressed;
}

void GameInputController::SuppressAllPlayerJumpInputUntilReleased()
{
    for (Player* player : mGame.GetPlayers()) {
        if (player) {
            player->SuppressJumpUntilReleased();
        }
    }
}

void GameInputController::ProcessDebugReloadInput()
{
    const bool reloadKeyPressed = mInput.IsKeyPressed(GLFW_KEY_F);
    if (mGame.GetIsDebugMode() && reloadKeyPressed && !mWasReloadKeyPressed) {
        if (!mGame.ReloadCurrentStage()) {
            std::cerr << "Failed to reload the current stage.\n";
        }
    }
    mWasReloadKeyPressed = reloadKeyPressed;

    const bool uiReloadKeyPressed = mInput.IsKeyPressed(GLFW_KEY_O);
    if (mGame.GetIsDebugMode() && uiReloadKeyPressed && !mWasUIReloadKeyPressed) {
        mGame.ReloadUIData();
    }
    mWasUIReloadKeyPressed = uiReloadKeyPressed;
}

void GameInputController::ProcessPlayerJoinInput()
{
    const bool playerJoinPressed = mInput.IsKeyPressed(GLFW_KEY_Q);
    if (playerJoinPressed && !mWasPlayerJoinPressed) {
        const bool canJoinWithoutController = mGame.GetIsDebugMode() && !mGame.IsGameControllerConnected();
        if ((mGame.IsGameControllerConnected() || canJoinWithoutController) && !mGame.GetIsPlayer2Joined()) {
            mGame.TryCreatePlayer2();
        }
    }
    mWasPlayerJoinPressed = playerJoinPressed;
}

void GameInputController::UpdateLastUsedInputDevice()
{
    const double cursorX = mInput.GetCursorX();
    const double cursorY = mInput.GetCursorY();

    constexpr double cursorMovementThresholdPixels = 0.25;
    const bool hasMouseMoved =
        mHasPreviousCursorPosition && (std::abs(cursorX - mPreviousCursorX) > cursorMovementThresholdPixels ||
                                       std::abs(cursorY - mPreviousCursorY) > cursorMovementThresholdPixels);
    mPreviousCursorX = cursorX;
    mPreviousCursorY = cursorY;
    mHasPreviousCursorPosition = true;

    const bool hasPhysicalMouseMoved = hasMouseMoved && !mShouldIgnoreNextSyntheticCursorMotion;
    mShouldIgnoreNextSyntheticCursorMotion = false;

    if (IsKeyboardOrMouseInputActive(mInput) || hasPhysicalMouseMoved) {
        mGame.RecordInputDeviceUsage(InputDeviceType::KeyboardMouse);
    }

    if (IsGameControllerInputActive(mInput)) {
        mGame.RecordInputDeviceUsage(InputDeviceType::GameController);
    }

    const bool isKeyboardModifierHeld = mInput.IsKeyPressed(GLFW_KEY_N);
    const bool isGameControllerModifierHeld = mInput.IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
    mGame.SetInputModifierHeld(isKeyboardModifierHeld || isGameControllerModifierHeld);
}

void GameInputController::ProcessPlayerSwitchInput()
{
    constexpr Sint16 triggerPressedThreshold = 16000;
    const bool switchPressed = mInput.IsKeyPressed(GLFW_KEY_Y) ||
                               mInput.GetControllerAxis(1, SDL_CONTROLLER_AXIS_TRIGGERLEFT) > triggerPressedThreshold;

    if (switchPressed && !mWasPlayerSwitchPressed) {
        mGame.SwitchControlledPlayer();
    }

    mWasPlayerSwitchPressed = switchPressed;
}

void GameInputController::ProcessPlayerSplitInput()
{
    constexpr Sint16 triggerPressedThreshold = 16000;
    const bool splitPressed = mInput.IsKeyPressed(GLFW_KEY_I) ||
                              mInput.GetControllerAxis(1, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > triggerPressedThreshold;

    mGame.SetPlayerSplitMergeButtonHeld(splitPressed);
    if (splitPressed && !mWasPlayerSplitPressed) {
        mGame.TogglePlayerSplit();
    }

    mWasPlayerSplitPressed = splitPressed;
}

void GameInputController::ProcessBattleStyleSelectionInput()
{
    SceneSystem* sceneSystem = mGame.GetSceneSystem();
    if (!sceneSystem || !sceneSystem->IsBattleStyleSelection()) {
        mWasBattleStyleDirectionPressed = false;
        return;
    }

    constexpr Sint16 directionThreshold = 16000;

    const bool previousDirectionPressed =
        mInput.IsKeyPressed(GLFW_KEY_LEFT) || mInput.IsKeyPressed(GLFW_KEY_UP) || mInput.IsKeyPressed(GLFW_KEY_A) ||
        mInput.IsKeyPressed(GLFW_KEY_W) || mInput.IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_LEFT) ||
        mInput.IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_UP) ||
        mInput.GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTX) < -directionThreshold ||
        mInput.GetControllerAxis(2, SDL_CONTROLLER_AXIS_LEFTX) < -directionThreshold ||
        mInput.GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTY) < -directionThreshold ||
        mInput.GetControllerAxis(2, SDL_CONTROLLER_AXIS_LEFTY) < -directionThreshold;
    const bool nextDirectionPressed = mInput.IsKeyPressed(GLFW_KEY_RIGHT) || mInput.IsKeyPressed(GLFW_KEY_DOWN) ||
                                      mInput.IsKeyPressed(GLFW_KEY_D) || mInput.IsKeyPressed(GLFW_KEY_S) ||
                                      mInput.IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_RIGHT) ||
                                      mInput.IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_DOWN) ||
                                      mInput.GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTX) > directionThreshold ||
                                      mInput.GetControllerAxis(2, SDL_CONTROLLER_AXIS_LEFTX) > directionThreshold ||
                                      mInput.GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTY) > directionThreshold ||
                                      mInput.GetControllerAxis(2, SDL_CONTROLLER_AXIS_LEFTY) > directionThreshold;
    const bool directionPressed = previousDirectionPressed || nextDirectionPressed;

    if (directionPressed && !mWasBattleStyleDirectionPressed) {
        sceneSystem->MoveBattleStyleSelection(previousDirectionPressed ? -1 : 1);
    }
    mWasBattleStyleDirectionPressed = directionPressed;
}

bool GameInputController::ProcessSceneConfirmInput(bool allowsSceneAction)
{
    SceneSystem* sceneSystem = mGame.GetSceneSystem();
    if (!sceneSystem) {
        return false;
    }

    const bool canProcessSceneAction = allowsSceneAction && !mUGC.IsWorkBrowserShowing();

    const std::array<bool, 2> controllerConfirmPressed = {
        mInput.IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_A),
        mInput.IsControllerButtonPressed(2, SDL_CONTROLLER_BUTTON_A),
    };

    const bool keyboardConfirmPressed = mInput.IsKeyPressed(GLFW_KEY_SPACE) || mInput.IsKeyPressed(GLFW_KEY_ENTER);

    const Player* controlledPlayer = mGame.GetControlledPlayer();
    const int controlledPlayerNum = controlledPlayer ? controlledPlayer->GetPlayerNum() : 1;
    const bool isTwoPlayerMode = mGame.GetIsPlayer2Joined();
    bool didExecuteSceneConfirm = false;

    for (int controllerIndex = 0; controllerIndex < 2; ++controllerIndex) {
        const int controllerPlayerNum = controllerIndex + 1;
        const bool canControllerConfirm = canProcessSceneAction &&
                                          controllerConfirmPressed[static_cast<std::size_t>(controllerIndex)] &&
                                          !mWasControllerConfirmPressed[static_cast<std::size_t>(controllerIndex)] &&
                                          (controllerPlayerNum == 1 || isTwoPlayerMode);
        if (!canControllerConfirm) {
            continue;
        }

        const int actionPlayerNum = isTwoPlayerMode ? controllerPlayerNum : controlledPlayerNum;
        if (sceneSystem->OnConfirmPressed(actionPlayerNum)) {
            didExecuteSceneConfirm = true;
            SuppressPlayerJumpUntilReleased(mGame, actionPlayerNum);
            break;
        }
    }

    const bool playerTwoUsesController = isTwoPlayerMode && mInput.HasControllerInput(2);
    if (canProcessSceneAction && !didExecuteSceneConfirm && !playerTwoUsesController && keyboardConfirmPressed &&
        !mWasKeyboardConfirmPressed) {
        const int keyboardPlayerNum = isTwoPlayerMode ? 2 : controlledPlayerNum;
        if (sceneSystem->OnConfirmPressed(keyboardPlayerNum)) {
            didExecuteSceneConfirm = true;
            SuppressPlayerJumpUntilReleased(mGame, keyboardPlayerNum);
        }
    }

    mWasControllerConfirmPressed = controllerConfirmPressed;
    mWasKeyboardConfirmPressed = keyboardConfirmPressed;
    return didExecuteSceneConfirm;
}

bool GameInputController::ProcessTgsEndShortcutInput()
{
    const bool isShiftPressed = mInput.IsKeyPressed(GLFW_KEY_LEFT_SHIFT) || mInput.IsKeyPressed(GLFW_KEY_RIGHT_SHIFT);
    const bool isQPressed = mInput.IsKeyPressed(GLFW_KEY_Q);
    const bool isShortcutPressed = mGame.IsTgsBuild() && isShiftPressed && isQPressed;
    const bool isNoticeShortcutPressed = mGame.IsTgsBuild() && isShiftPressed && mInput.IsKeyPressed(GLFW_KEY_W);

    if (isShortcutPressed && !mWasTgsEndShortcutPressed) {
        mSession.EndTgsExperienceNow();
    }

    mWasTgsEndShortcutPressed = isShortcutPressed;
    if (isNoticeShortcutPressed && !mWasTgsNoticeShortcutPressed) {
        mSession.SkipTgsExperienceToRemainingTimeNotice();
    }
    mWasTgsNoticeShortcutPressed = isNoticeShortcutPressed;
    if (isShortcutPressed) {
        // Q 単体の2P参加判定へ、終了ショートカットを流さない。
        mWasPlayerJoinPressed = true;
    }
    return isShortcutPressed || isNoticeShortcutPressed;
}

void GameInputController::ProcessDebugEditorToggleInput()
{
    const bool debugEditorTogglePressed = mInput.IsKeyPressed(GLFW_KEY_P);
    if ((mGame.GetIsDebugMode() || mUGC.IsModeActive()) && debugEditorTogglePressed && !mWasDebugEditorTogglePressed) {
        mGame.ToggleDebugEditor();
    }
    mWasDebugEditorTogglePressed = debugEditorTogglePressed;
}

void GameInputController::ProcessFreeCameraToggleInput()
{
    const bool freeCameraTogglePressed = mInput.IsKeyPressed(GLFW_KEY_L);
    if (mGame.GetIsDebugMode() && freeCameraTogglePressed && !mWasFreeCameraTogglePressed) {
        mGame.ToggleFreeCameraMode();
    }
    mWasFreeCameraTogglePressed = freeCameraTogglePressed;
}

void GameInputController::ProcessStartInput(bool allowsStartAction)
{
    SceneSystem* sceneSystem = mGame.GetSceneSystem();
    if (!sceneSystem) {
        return;
    }

    const bool startPressed =
        mInput.IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_START) || mInput.IsKeyPressed(GLFW_KEY_ENTER);

    if (allowsStartAction && startPressed && !mWasStartPressed) {
        sceneSystem->OnStartPressed();
    }

    mWasStartPressed = startPressed;
}
