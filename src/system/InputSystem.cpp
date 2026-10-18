#include "system/InputSystem.h"

#include "app/Game.h"
#include "actor/Player.h"
#include "system/CameraSystem.h"
#include "system/PlayerInputDeviceRouting.h"
#include "system/SceneSystem.h"

#include "imgui.h"
#include <GLFW/glfw3.h>
#include <SDL.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <iostream>

namespace {

constexpr int controllerMovementDeadZone = static_cast<int>(0.25f * 32767.0f);

bool IsControllerMovementPressed(const InputSystem& inputSystem, int playerNum)
{
    if (!inputSystem.HasControllerInput(playerNum)) {
        return false;
    }

    const int horizontalAxis = inputSystem.GetControllerAxis(playerNum, SDL_CONTROLLER_AXIS_LEFTX);
    const int verticalAxis = inputSystem.GetControllerAxis(playerNum, SDL_CONTROLLER_AXIS_LEFTY);

    return std::abs(horizontalAxis) >= controllerMovementDeadZone ||
           std::abs(verticalAxis) >= controllerMovementDeadZone;
}

bool IsKeyboardMovementPressed(const InputSystem& inputSystem)
{
    return inputSystem.IsKeyPressed(GLFW_KEY_W) || inputSystem.IsKeyPressed(GLFW_KEY_A) ||
           inputSystem.IsKeyPressed(GLFW_KEY_S) || inputSystem.IsKeyPressed(GLFW_KEY_D);
}

bool IsSecondPlayerDebugKeyboardMovementPressed(const InputSystem& inputSystem)
{
    return inputSystem.IsKeyPressed(GLFW_KEY_UP) || inputSystem.IsKeyPressed(GLFW_KEY_DOWN) ||
           inputSystem.IsKeyPressed(GLFW_KEY_LEFT) || inputSystem.IsKeyPressed(GLFW_KEY_RIGHT);
}

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

InputSystem::InputSystem(Game* game)
    : mGame(game)
{
}

void InputSystem::CaptureFrameInput()
{
    mSnapshot = InputSnapshot{};
    if (!mGame || !mGame->GetWindow()) {
        return;
    }

    GLFWwindow* window = mGame->GetWindow();
    for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key) {
        mSnapshot.keys[static_cast<std::size_t>(key)] = glfwGetKey(window, key) == GLFW_PRESS;
    }
    for (int button = GLFW_MOUSE_BUTTON_1; button <= GLFW_MOUSE_BUTTON_LAST; ++button) {
        mSnapshot.mouseButtons[static_cast<std::size_t>(button)] = glfwGetMouseButton(window, button) == GLFW_PRESS;
    }
    glfwGetCursorPos(window, &mSnapshot.cursorX, &mSnapshot.cursorY);

    for (int playerIndex = 0; playerIndex < 2; ++playerIndex) {
        SDL_GameController* controller = mGame->GetSdlControllerForPlayer(playerIndex + 1);
        if (!controller) {
            continue;
        }
        mSnapshot.hasController[static_cast<std::size_t>(playerIndex)] = true;
        for (int axis = 0; axis < SDL_CONTROLLER_AXIS_MAX; ++axis) {
            mSnapshot.controllerAxes[static_cast<std::size_t>(playerIndex)][static_cast<std::size_t>(axis)] =
                static_cast<int>(SDL_GameControllerGetAxis(controller, static_cast<SDL_GameControllerAxis>(axis)));
        }
        for (int button = 0; button < SDL_CONTROLLER_BUTTON_MAX; ++button) {
            mSnapshot.controllerButtons[static_cast<std::size_t>(playerIndex)][static_cast<std::size_t>(button)] =
                SDL_GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(button)) != 0;
        }
    }
}

bool InputSystem::IsKeyPressed(int key) const
{
    return key >= 0 && key < static_cast<int>(mSnapshot.keys.size()) && mSnapshot.keys[static_cast<std::size_t>(key)];
}

bool InputSystem::IsMouseButtonPressed(int button) const
{
    return button >= 0 && button < static_cast<int>(mSnapshot.mouseButtons.size()) &&
           mSnapshot.mouseButtons[static_cast<std::size_t>(button)];
}

int InputSystem::GetControllerAxis(int playerNum, int axis) const
{
    const int playerIndex = playerNum - 1;
    if (playerIndex < 0 || playerIndex >= 2 || axis < 0 || axis >= 8) {
        return 0;
    }
    return mSnapshot.controllerAxes[static_cast<std::size_t>(playerIndex)][static_cast<std::size_t>(axis)];
}

bool InputSystem::IsControllerButtonPressed(int playerNum, int button) const
{
    const int playerIndex = playerNum - 1;
    if (playerIndex < 0 || playerIndex >= 2 || button < 0 || button >= 32) {
        return false;
    }
    return mSnapshot.controllerButtons[static_cast<std::size_t>(playerIndex)][static_cast<std::size_t>(button)];
}

bool InputSystem::IsAnyControllerButtonPressed(int button) const
{
    return IsControllerButtonPressed(1, button) || IsControllerButtonPressed(2, button);
}

bool InputSystem::HasControllerInput(int playerNum) const
{
    const int playerIndex = playerNum - 1;
    return playerIndex >= 0 && playerIndex < 2 && mSnapshot.hasController[static_cast<std::size_t>(playerIndex)];
}

bool InputSystem::IsMovementInputPressedForPlayer(const Player* player) const
{
    if (!mGame || !player) {
        return false;
    }

    const int playerNum = player->GetPlayerNum();
    const Player* controlledPlayer = mGame->GetControlledPlayer();
    const PlayerInputDevice inputDevice = ResolvePlayerInputDevice(
        playerNum, {
                       .isTwoPlayerMode = mGame->GetIsPlayer2Joined(),
                       .isDebugMode = mGame->GetIsDebugMode(),
                       .hasControllerOne = HasControllerInput(1),
                       .hasControllerTwo = HasControllerInput(2),
                       .controlledPlayerNum = controlledPlayer ? controlledPlayer->GetPlayerNum() : 1,
                   });

    const int controllerPlayerNum = ResolveControllerPlayerNum(inputDevice);
    if (controllerPlayerNum != 0) {
        return IsControllerMovementPressed(*this, controllerPlayerNum);
    }
    if (inputDevice == PlayerInputDevice::PrimaryKeyboard) {
        return IsKeyboardMovementPressed(*this);
    }
    if (inputDevice == PlayerInputDevice::SecondaryKeyboard) {
        return IsSecondPlayerDebugKeyboardMovementPressed(*this);
    }
    return false;
}

void InputSystem::ProcessGameInput()
{
    if (!mGame || !mGame->GetWindow()) {
        return;
    }

    if (mGame->IsEditorKeyboardInputCaptured()) {
        UpdateLastUsedInputDevice();
        ProcessUGCEditorCursorInput();
        SuppressOneShotInputUntilReleased();
        return;
    }

    UpdateLastUsedInputDevice();

    if (ProcessTgsEndShortcutInput()) {
        return;
    }

    if (mGame->GetIsUGCClearResultShowing()) {
        ProcessUGCClearResultInput();
        ProcessStartInput(false);
        return;
    }

    ProcessDebugEditorToggleInput();
    ProcessUGCEditorCursorInput();
    const bool isUGCEditorActive =
        mGame->GetIsUGCMode() && mGame->GetIsDebugEditorShowing() && !mGame->GetIsUGCDebugEditorShowing();
    if (isUGCEditorActive) {
        ProcessUGCEditorCommandInput();
        return;
    }
    ProcessPauseToggleInput();

    const bool wasPauseMenuOpen = mGame->GetIsPauseMenuOpen();
    if (wasPauseMenuOpen) {
        ProcessPauseMenuInput();
    }

    ProcessDebugReloadInput();
    ProcessPlayerJoinInput();
    ProcessPlayerSplitInput();
    ProcessPlayerSwitchInput();
    const bool wasTitleMenuActive =
        mGame->GetSceneSystem() && mGame->GetSceneSystem()->IsTitle() && !mGame->GetIsUGCWorkBrowserShowing();
    ProcessUGCModeInput();
    ProcessTitleMenuInput();
    ProcessBattleStyleSelectionInput();

    // ポーズ決定もAを使うため、そのフレームの入力を会話開始へ流さない。
    const bool didExecuteSceneConfirm =
        ProcessSceneConfirmInput(!wasPauseMenuOpen && !isUGCEditorActive && !wasTitleMenuActive);
    ProcessFreeCameraToggleInput();
    const bool allowsStartAction =
        !wasPauseMenuOpen && !wasTitleMenuActive && !mGame->GetIsUGCWorkBrowserShowing() && !didExecuteSceneConfirm;
    ProcessStartInput(allowsStartAction);
}

void InputSystem::SuppressOneShotInputUntilReleased()
{
    if (mGame) {
        mGame->SetPlayerSplitMergeButtonHeld(false);
    }
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
    mWasKeyboardConfirmPressed = true;
}

void InputSystem::SuppressUGCPlayShortcutUntilReleased()
{
    mWasPauseMenuKeyPressed = true;
    mWasUGCEditorPlayPressed = true;
}

void InputSystem::ProcessUGCModeInput()
{
    SceneSystem* sceneSystem = mGame->GetSceneSystem();
    if (!sceneSystem || !sceneSystem->IsTitle()) {
        mWasUGCModePressed = false;
        mWasUGCWorkBrowserPressed = false;
        return;
    }

    if (mGame->GetIsUGCWorkBrowserShowing()) {
        mWasUGCModePressed = false;
        mWasUGCWorkBrowserPressed = false;
        return;
    }

    const bool keyboardPressed = IsKeyPressed(GLFW_KEY_C);
    const bool controllerPressed = IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_X);
    const bool ugcModePressed = keyboardPressed || controllerPressed;

    if (ugcModePressed && !mWasUGCModePressed) {
        mGame->StartUGCMode();
    }
    mWasUGCModePressed = ugcModePressed;

    const bool browserKeyboardPressed = IsKeyPressed(GLFW_KEY_V);
    const bool browserControllerPressed = IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_Y);
    const bool browserPressed = browserKeyboardPressed || browserControllerPressed;
    if (browserPressed && !mWasUGCWorkBrowserPressed) {
        mGame->OpenUGCWorkBrowser();
    }
    mWasUGCWorkBrowserPressed = browserPressed;
}

void InputSystem::ProcessUGCClearResultInput()
{
    constexpr Sint16 directionThreshold = 16000;
    const bool upPressed = IsKeyPressed(GLFW_KEY_UP) || IsKeyPressed(GLFW_KEY_W) ||
                           IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_UP) ||
                           GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTY) < -directionThreshold ||
                           GetControllerAxis(2, SDL_CONTROLLER_AXIS_LEFTY) < -directionThreshold;
    const bool downPressed = IsKeyPressed(GLFW_KEY_DOWN) || IsKeyPressed(GLFW_KEY_S) ||
                             IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_DOWN) ||
                             GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTY) > directionThreshold ||
                             GetControllerAxis(2, SDL_CONTROLLER_AXIS_LEFTY) > directionThreshold;
    const bool directionPressed = upPressed || downPressed;
    if (directionPressed && !mWasUGCClearResultDirectionPressed) {
        mGame->MoveUGCClearResultSelection(upPressed ? -1 : 1);
    }
    mWasUGCClearResultDirectionPressed = directionPressed;

    const bool confirmPressed = IsKeyPressed(GLFW_KEY_SPACE) || IsKeyPressed(GLFW_KEY_ENTER) ||
                                IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_A);
    if (confirmPressed && !mWasUGCClearResultConfirmPressed) {
        mGame->ExecuteUGCClearResultSelection();
    }
    mWasUGCClearResultConfirmPressed = confirmPressed;
}

void InputSystem::ProcessTitleMenuInput()
{
    SceneSystem* sceneSystem = mGame->GetSceneSystem();
    if (!sceneSystem || !sceneSystem->IsTitle() || mGame->GetIsUGCWorkBrowserShowing()) {
        mWasTitleMenuDirectionPressed = false;
        mWasTitleMenuConfirmPressed = false;
        return;
    }

    constexpr Sint16 directionThreshold = 16000;
    const bool upPressed = IsKeyPressed(GLFW_KEY_UP) || IsKeyPressed(GLFW_KEY_W) ||
                           IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_UP) ||
                           GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTY) < -directionThreshold ||
                           GetControllerAxis(2, SDL_CONTROLLER_AXIS_LEFTY) < -directionThreshold;
    const bool downPressed = IsKeyPressed(GLFW_KEY_DOWN) || IsKeyPressed(GLFW_KEY_S) ||
                             IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_DOWN) ||
                             GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTY) > directionThreshold ||
                             GetControllerAxis(2, SDL_CONTROLLER_AXIS_LEFTY) > directionThreshold;
    const bool directionPressed = upPressed || downPressed;
    if (directionPressed && !mWasTitleMenuDirectionPressed) {
        mGame->MoveTitleMenuSelection(upPressed ? -1 : 1);
    }
    mWasTitleMenuDirectionPressed = directionPressed;

    const bool confirmPressed = IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_A) || IsKeyPressed(GLFW_KEY_SPACE) ||
                                IsKeyPressed(GLFW_KEY_ENTER);
    if (confirmPressed && !mWasTitleMenuConfirmPressed) {
        mGame->ExecuteTitleMenuSelection();
    }
    mWasTitleMenuConfirmPressed = confirmPressed;
}

void InputSystem::ProcessUGCEditorCursorInput()
{
    const bool isUGCEditorActive = mGame->GetIsUGCMode() && mGame->GetIsDebugEditorShowing() &&
                                   !mGame->GetIsUGCDebugEditorShowing() && HasControllerInput(1);
    if (!isUGCEditorActive) {
        if (mWasUGCEditorControllerClickPressed && ImGui::GetCurrentContext()) {
            const bool isPhysicalLeftMousePressed = IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
            ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Left, isPhysicalLeftMousePressed);
        }
        mWasUGCEditorCursorActive = false;
        mWasUGCEditorControllerClickPressed = false;
        return;
    }

    const bool isControllerClickPressed = IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_A);
    if (!mWasUGCEditorCursorActive) {
        // タイトル決定に使ったAを、編集画面の最初のボタンへ流さない。
        mWasUGCEditorCursorActive = true;
        mWasUGCEditorControllerClickPressed = isControllerClickPressed;
        return;
    }
    if (isControllerClickPressed != mWasUGCEditorControllerClickPressed && ImGui::GetCurrentContext()) {
        const bool isPhysicalLeftMousePressed = IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
        ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Left,
                                           isControllerClickPressed || isPhysicalLeftMousePressed);
    }
    mWasUGCEditorControllerClickPressed = isControllerClickPressed;

    constexpr float axisScale = 1.0f / 32767.0f;
    constexpr float deadZone = 0.2f;
    float previewTurnInput = GetControllerAxis(1, SDL_CONTROLLER_AXIS_RIGHTX) * axisScale;
    if (std::abs(previewTurnInput) < deadZone) {
        previewTurnInput = 0.0f;
    }
    if (previewTurnInput != 0.0f) {
        constexpr float previewTurnRadiansPerSecond = 1.8f;
        const float deltaTime = std::max(0.001f, mGame->GetLastDeltaTime());
        mGame->AdjustUGCPreviewYaw(-previewTurnInput * previewTurnRadiansPerSecond * deltaTime);
    }

    if (!mGame->GetCameraSystem()) {
        return;
    }

    float horizontalInput = GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTX) * axisScale;
    float verticalInput = GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTY) * axisScale;
    if (std::abs(horizontalInput) < deadZone) {
        horizontalInput = 0.0f;
    }
    if (std::abs(verticalInput) < deadZone) {
        verticalInput = 0.0f;
    }
    if (horizontalInput == 0.0f && verticalInput == 0.0f) {
        return;
    }

    GLFWwindow* window = mGame->GetWindow();
    int windowWidth = 0;
    int windowHeight = 0;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    double cursorX = GetCursorX();
    double cursorY = GetCursorY();

    constexpr float normalCursorSpeedPixelsPerSecond = 650.0f;
    const float deltaTime = std::max(0.001f, mGame->GetLastDeltaTime());
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

    CameraPose cameraPose = mGame->GetCameraSystem()->GetDebugCameraPose();
    const glm::vec3 forward = glm::normalize(cameraPose.target - cameraPose.position);
    const glm::vec3 right = glm::normalize(glm::cross(forward, cameraPose.up));
    constexpr float normalViewScrollWorldUnitsPerSecond = 10.0f;
    const glm::vec3 viewMovement = (right * edgeScrollDirection.x + cameraPose.up * edgeScrollDirection.y) *
                                   normalViewScrollWorldUnitsPerSecond * deltaTime;
    cameraPose.position += viewMovement;
    cameraPose.target += viewMovement;
    mGame->GetCameraSystem()->SetDebugCameraPose(cameraPose);
}

void InputSystem::ProcessUGCEditorCommandInput()
{
    const bool undoPressed = IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_B) || IsKeyPressed(GLFW_KEY_U);
    const bool redoPressed = IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_Y) || IsKeyPressed(GLFW_KEY_J);
    const bool eraserPressed = IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_X) || IsKeyPressed(GLFW_KEY_K);
    const bool zoomInPressed =
        IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) || IsKeyPressed(GLFW_KEY_M);
    const bool zoomOutPressed =
        IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_LEFTSHOULDER) || IsKeyPressed(GLFW_KEY_N);
    constexpr Sint16 triggerPressedThreshold = 16000;
    const bool layerDownPressed =
        GetControllerAxis(1, SDL_CONTROLLER_AXIS_TRIGGERLEFT) > triggerPressedThreshold || IsKeyPressed(GLFW_KEY_Y);
    const bool layerUpPressed =
        GetControllerAxis(1, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > triggerPressedThreshold || IsKeyPressed(GLFW_KEY_I);
    const bool playPressed = IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_BACK) || IsKeyPressed(GLFW_KEY_ESCAPE);
    const bool selectionPressed =
        IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_START) || IsKeyPressed(GLFW_KEY_ENTER);
    const bool dpadLeftPressed = IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    const bool dpadRightPressed = IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
    const bool dpadUpPressed = IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_DPAD_UP);
    const bool dpadDownPressed = IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_DPAD_DOWN);

    float previewTurnInput = 0.0f;
    if (IsKeyPressed(GLFW_KEY_LEFT)) {
        previewTurnInput = -1.0f;
    } else if (IsKeyPressed(GLFW_KEY_RIGHT)) {
        previewTurnInput = 1.0f;
    }
    if (previewTurnInput != 0.0f) {
        constexpr float previewTurnRadiansPerSecond = 1.8f;
        const float deltaTime = std::max(0.001f, mGame->GetLastDeltaTime());
        mGame->AdjustUGCPreviewYaw(-previewTurnInput * previewTurnRadiansPerSecond * deltaTime);
    }

    if (undoPressed && !mWasUGCEditorUndoPressed) {
        mGame->UndoUGCEdit();
    }
    if (redoPressed && !mWasUGCEditorRedoPressed) {
        mGame->RedoUGCEdit();
    }
    if (eraserPressed && !mWasUGCEditorEraserPressed) {
        mGame->ToggleUGCEraser();
    }
    if (zoomInPressed && !mWasUGCEditorZoomInPressed) {
        mGame->ZoomUGCEditor(0.8f);
    }
    if (zoomOutPressed && !mWasUGCEditorZoomOutPressed) {
        mGame->ZoomUGCEditor(1.25f);
    }
    if (layerDownPressed && !mWasUGCEditorLayerDownPressed) {
        mGame->ChangeUGCEditLayer(-1);
    }
    if (layerUpPressed && !mWasUGCEditorLayerUpPressed) {
        mGame->ChangeUGCEditLayer(1);
    }
    if (selectionPressed && !mWasUGCEditorSelectionPressed) {
        mGame->SelectUGCEditorMode();
    }
    if (playPressed && !mWasUGCEditorPlayPressed) {
        mGame->StartUGCPlaytest();
    }
    if (dpadLeftPressed && !mWasUGCEditorDpadLeftPressed) {
        mGame->MoveUGCSelectionByGrid(-1, 0);
    }
    if (dpadRightPressed && !mWasUGCEditorDpadRightPressed) {
        mGame->MoveUGCSelectionByGrid(1, 0);
    }
    if (dpadUpPressed && !mWasUGCEditorDpadUpPressed) {
        mGame->MoveUGCSelectionByGrid(0, -1);
    }
    if (dpadDownPressed && !mWasUGCEditorDpadDownPressed) {
        mGame->MoveUGCSelectionByGrid(0, 1);
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

void InputSystem::ProcessPauseToggleInput()
{
    SceneSystem* sceneSystem = mGame->GetSceneSystem();
    if (!sceneSystem) {
        return;
    }

    const bool escapePressed = IsKeyPressed(GLFW_KEY_ESCAPE);
    const bool controllerBackPressed = IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_BACK);

    const bool returnToUGCEditorPressed = escapePressed || controllerBackPressed;
    if (returnToUGCEditorPressed && !mWasPauseMenuKeyPressed && mGame->GetIsUGCPlaytestActive()) {
        mGame->ReturnToUGCEditor();
        SuppressUGCPlayShortcutUntilReleased();
        return;
    }

    const bool pauseMenuKeyPressed = escapePressed || controllerBackPressed;

    if (pauseMenuKeyPressed && !mWasPauseMenuKeyPressed && (mGame->GetIsPauseMenuOpen() || mGame->CanOpenPauseMenu())) {
        mGame->TogglePauseMenu();
    }

    mWasPauseMenuKeyPressed = pauseMenuKeyPressed;
}

void InputSystem::ProcessPauseMenuInput()
{
    constexpr Sint16 directionThreshold = 16000;
    const bool upPressed = IsKeyPressed(GLFW_KEY_UP) || IsKeyPressed(GLFW_KEY_W) ||
                           IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_UP) ||
                           GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTY) < -directionThreshold ||
                           GetControllerAxis(2, SDL_CONTROLLER_AXIS_LEFTY) < -directionThreshold;

    const bool downPressed = IsKeyPressed(GLFW_KEY_DOWN) || IsKeyPressed(GLFW_KEY_S) ||
                             IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_DOWN) ||
                             GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTY) > directionThreshold ||
                             GetControllerAxis(2, SDL_CONTROLLER_AXIS_LEFTY) > directionThreshold;

    const bool confirmPressed = IsKeyPressed(GLFW_KEY_SPACE) || IsKeyPressed(GLFW_KEY_ENTER) ||
                                IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_A);

    if (upPressed && !mWasPauseMenuUpPressed) {
        mGame->MovePauseMenuSelection(-1);
    }

    if (downPressed && !mWasPauseMenuDownPressed) {
        mGame->MovePauseMenuSelection(1);
    }

    if (confirmPressed && !mWasPauseMenuConfirmPressed) {
        mGame->ExecutePauseMenuItem();
        SuppressAllPlayerJumpInputUntilReleased();
    }

    mWasPauseMenuUpPressed = upPressed;
    mWasPauseMenuDownPressed = downPressed;
    mWasPauseMenuConfirmPressed = confirmPressed;
}

void InputSystem::SuppressAllPlayerJumpInputUntilReleased()
{
    for (Player* player : mGame->GetPlayers()) {
        if (player) {
            player->SuppressJumpUntilReleased();
        }
    }
}

void InputSystem::ProcessDebugReloadInput()
{
    const bool reloadKeyPressed = IsKeyPressed(GLFW_KEY_F);
    if (mGame->GetIsDebugMode() && reloadKeyPressed && !mWasReloadKeyPressed) {
        if (!mGame->ReloadCurrentStage()) {
            std::cerr << "Failed to reload the current stage.\n";
        }
    }
    mWasReloadKeyPressed = reloadKeyPressed;

    const bool uiReloadKeyPressed = IsKeyPressed(GLFW_KEY_O);
    if (mGame->GetIsDebugMode() && uiReloadKeyPressed && !mWasUIReloadKeyPressed) {
        mGame->ReloadUIData();
    }
    mWasUIReloadKeyPressed = uiReloadKeyPressed;
}

void InputSystem::ProcessPlayerJoinInput()
{
    const bool playerJoinPressed = IsKeyPressed(GLFW_KEY_Q);
    if (playerJoinPressed && !mWasPlayerJoinPressed) {
        const bool canJoinWithoutController = mGame->GetIsDebugMode() && !mGame->IsGameControllerConnected();
        if ((mGame->IsGameControllerConnected() || canJoinWithoutController) && !mGame->GetIsPlayer2Joined()) {
            mGame->TryCreatePlayer2();
        }
    }
    mWasPlayerJoinPressed = playerJoinPressed;
}

void InputSystem::UpdateLastUsedInputDevice()
{
    const double cursorX = GetCursorX();
    const double cursorY = GetCursorY();

    constexpr double cursorMovementThresholdPixels = 0.25;
    const bool hasMouseMoved =
        mHasPreviousCursorPosition && (std::abs(cursorX - mPreviousCursorX) > cursorMovementThresholdPixels ||
                                       std::abs(cursorY - mPreviousCursorY) > cursorMovementThresholdPixels);
    mPreviousCursorX = cursorX;
    mPreviousCursorY = cursorY;
    mHasPreviousCursorPosition = true;

    const bool hasPhysicalMouseMoved = hasMouseMoved && !mShouldIgnoreNextSyntheticCursorMotion;
    mShouldIgnoreNextSyntheticCursorMotion = false;

    if (IsKeyboardOrMouseInputActive(*this) || hasPhysicalMouseMoved) {
        mGame->RecordInputDeviceUsage(InputDeviceType::KeyboardMouse);
    }

    if (IsGameControllerInputActive(*this)) {
        mGame->RecordInputDeviceUsage(InputDeviceType::GameController);
    }

    const bool isKeyboardModifierHeld = IsKeyPressed(GLFW_KEY_N);
    const bool isGameControllerModifierHeld = IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
    mGame->SetInputModifierHeld(isKeyboardModifierHeld || isGameControllerModifierHeld);
}

void InputSystem::ProcessPlayerSwitchInput()
{
    constexpr Sint16 triggerPressedThreshold = 16000;
    const bool switchPressed =
        IsKeyPressed(GLFW_KEY_Y) || GetControllerAxis(1, SDL_CONTROLLER_AXIS_TRIGGERLEFT) > triggerPressedThreshold;

    if (switchPressed && !mWasPlayerSwitchPressed) {
        mGame->SwitchControlledPlayer();
    }

    mWasPlayerSwitchPressed = switchPressed;
}

void InputSystem::ProcessPlayerSplitInput()
{
    constexpr Sint16 triggerPressedThreshold = 16000;
    const bool splitPressed =
        IsKeyPressed(GLFW_KEY_I) || GetControllerAxis(1, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > triggerPressedThreshold;

    mGame->SetPlayerSplitMergeButtonHeld(splitPressed);
    if (splitPressed && !mWasPlayerSplitPressed) {
        mGame->TogglePlayerSplit();
    }

    mWasPlayerSplitPressed = splitPressed;
}

void InputSystem::ProcessBattleStyleSelectionInput()
{
    SceneSystem* sceneSystem = mGame->GetSceneSystem();
    if (!sceneSystem || !sceneSystem->IsBattleStyleSelection()) {
        mWasBattleStyleDirectionPressed = false;
        return;
    }

    constexpr Sint16 directionThreshold = 16000;

    const bool previousDirectionPressed = IsKeyPressed(GLFW_KEY_LEFT) || IsKeyPressed(GLFW_KEY_UP) ||
                                          IsKeyPressed(GLFW_KEY_A) || IsKeyPressed(GLFW_KEY_W) ||
                                          IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_LEFT) ||
                                          IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_UP) ||
                                          GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTX) < -directionThreshold ||
                                          GetControllerAxis(2, SDL_CONTROLLER_AXIS_LEFTX) < -directionThreshold ||
                                          GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTY) < -directionThreshold ||
                                          GetControllerAxis(2, SDL_CONTROLLER_AXIS_LEFTY) < -directionThreshold;
    const bool nextDirectionPressed = IsKeyPressed(GLFW_KEY_RIGHT) || IsKeyPressed(GLFW_KEY_DOWN) ||
                                      IsKeyPressed(GLFW_KEY_D) || IsKeyPressed(GLFW_KEY_S) ||
                                      IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_RIGHT) ||
                                      IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_DPAD_DOWN) ||
                                      GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTX) > directionThreshold ||
                                      GetControllerAxis(2, SDL_CONTROLLER_AXIS_LEFTX) > directionThreshold ||
                                      GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTY) > directionThreshold ||
                                      GetControllerAxis(2, SDL_CONTROLLER_AXIS_LEFTY) > directionThreshold;
    const bool directionPressed = previousDirectionPressed || nextDirectionPressed;

    if (directionPressed && !mWasBattleStyleDirectionPressed) {
        sceneSystem->MoveBattleStyleSelection(previousDirectionPressed ? -1 : 1);
    }
    mWasBattleStyleDirectionPressed = directionPressed;
}

bool InputSystem::ProcessSceneConfirmInput(bool allowsSceneAction)
{
    SceneSystem* sceneSystem = mGame->GetSceneSystem();
    if (!sceneSystem) {
        return false;
    }

    const bool canProcessSceneAction = allowsSceneAction && !mGame->GetIsUGCWorkBrowserShowing();

    const std::array<bool, 2> controllerConfirmPressed = {
        IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_A),
        IsControllerButtonPressed(2, SDL_CONTROLLER_BUTTON_A),
    };

    const bool keyboardConfirmPressed = IsKeyPressed(GLFW_KEY_SPACE) || IsKeyPressed(GLFW_KEY_ENTER);

    const Player* controlledPlayer = mGame->GetControlledPlayer();
    const int controlledPlayerNum = controlledPlayer ? controlledPlayer->GetPlayerNum() : 1;
    const bool isTwoPlayerMode = mGame->GetIsPlayer2Joined();
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
            SuppressPlayerJumpUntilReleased(*mGame, actionPlayerNum);
            break;
        }
    }

    const bool playerTwoUsesController = isTwoPlayerMode && HasControllerInput(2);
    if (canProcessSceneAction && !didExecuteSceneConfirm && !playerTwoUsesController && keyboardConfirmPressed &&
        !mWasKeyboardConfirmPressed) {
        const int keyboardPlayerNum = isTwoPlayerMode ? 2 : controlledPlayerNum;
        if (sceneSystem->OnConfirmPressed(keyboardPlayerNum)) {
            didExecuteSceneConfirm = true;
            SuppressPlayerJumpUntilReleased(*mGame, keyboardPlayerNum);
        }
    }

    mWasControllerConfirmPressed = controllerConfirmPressed;
    mWasKeyboardConfirmPressed = keyboardConfirmPressed;
    return didExecuteSceneConfirm;
}

bool InputSystem::ProcessTgsEndShortcutInput()
{
    const bool isShiftPressed = IsKeyPressed(GLFW_KEY_LEFT_SHIFT) || IsKeyPressed(GLFW_KEY_RIGHT_SHIFT);
    const bool isQPressed = IsKeyPressed(GLFW_KEY_Q);
    const bool isShortcutPressed = mGame->IsTgsBuild() && isShiftPressed && isQPressed;
    const bool isNoticeShortcutPressed = mGame->IsTgsBuild() && isShiftPressed && IsKeyPressed(GLFW_KEY_W);

    if (isShortcutPressed && !mWasTgsEndShortcutPressed) {
        mGame->EndTgsExperienceNow();
    }

    mWasTgsEndShortcutPressed = isShortcutPressed;
    if (isNoticeShortcutPressed && !mWasTgsNoticeShortcutPressed) {
        mGame->SkipTgsExperienceToRemainingTimeNotice();
    }
    mWasTgsNoticeShortcutPressed = isNoticeShortcutPressed;
    if (isShortcutPressed) {
        // Q 単体の2P参加判定へ、終了ショートカットを流さない。
        mWasPlayerJoinPressed = true;
    }
    return isShortcutPressed || isNoticeShortcutPressed;
}

void InputSystem::ProcessDebugEditorToggleInput()
{
    const bool debugEditorTogglePressed = IsKeyPressed(GLFW_KEY_P);
    if ((mGame->GetIsDebugMode() || mGame->GetIsUGCMode()) && debugEditorTogglePressed &&
        !mWasDebugEditorTogglePressed) {
        mGame->ToggleDebugEditor();
    }
    mWasDebugEditorTogglePressed = debugEditorTogglePressed;
}

void InputSystem::ProcessFreeCameraToggleInput()
{
    const bool freeCameraTogglePressed = IsKeyPressed(GLFW_KEY_L);
    if (mGame->GetIsDebugMode() && freeCameraTogglePressed && !mWasFreeCameraTogglePressed) {
        mGame->ToggleFreeCameraMode();
    }
    mWasFreeCameraTogglePressed = freeCameraTogglePressed;
}

void InputSystem::ProcessStartInput(bool allowsStartAction)
{
    SceneSystem* sceneSystem = mGame->GetSceneSystem();
    if (!sceneSystem) {
        return;
    }

    const bool startPressed = IsAnyControllerButtonPressed(SDL_CONTROLLER_BUTTON_START) || IsKeyPressed(GLFW_KEY_ENTER);

    if (allowsStartAction && startPressed && !mWasStartPressed) {
        sceneSystem->OnStartPressed();
    }

    mWasStartPressed = startPressed;
}
