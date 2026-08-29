#include "system/InputSystem.h"

#include "Game.h"
#include "actor/Player.h"
#include "system/CameraSystem.h"
#include "system/SceneSystem.h"

#include <GLFW/glfw3.h>
#include <SDL.h>
#include "imgui.h"

#include <cstdlib>
#include <algorithm>
#include <array>
#include <cmath>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

namespace {
constexpr int controllerMovementDeadZone =
    static_cast<int>(0.25f * 32767.0f);

bool IsControllerMovementPressed(const InputSystem& inputSystem)
{
    if (!inputSystem.HasControllerInput(1)) {
        return false;
    }

    const int horizontalAxis = inputSystem.GetControllerAxis(
        1, SDL_CONTROLLER_AXIS_LEFTX);
    const int verticalAxis = inputSystem.GetControllerAxis(
        1, SDL_CONTROLLER_AXIS_LEFTY);

    return std::abs(horizontalAxis) >= controllerMovementDeadZone ||
           std::abs(verticalAxis) >= controllerMovementDeadZone;
}

bool IsKeyboardMovementPressed(const InputSystem& inputSystem)
{
    return inputSystem.IsKeyPressed(GLFW_KEY_W) ||
           inputSystem.IsKeyPressed(GLFW_KEY_A) ||
           inputSystem.IsKeyPressed(GLFW_KEY_S) ||
           inputSystem.IsKeyPressed(GLFW_KEY_D);
}

bool IsKeyboardOrMouseInputActive(const InputSystem& inputSystem)
{
    constexpr std::array<int, 30> TrackedKeys = {
        GLFW_KEY_W,
        GLFW_KEY_A,
        GLFW_KEY_S,
        GLFW_KEY_D,
        GLFW_KEY_UP,
        GLFW_KEY_DOWN,
        GLFW_KEY_LEFT,
        GLFW_KEY_RIGHT,
        GLFW_KEY_SPACE,
        GLFW_KEY_J,
        GLFW_KEY_K,
        GLFW_KEY_M,
        GLFW_KEY_U,
        GLFW_KEY_Q,
        GLFW_KEY_E,
        GLFW_KEY_X,
        GLFW_KEY_Z,
        GLFW_KEY_N,
        GLFW_KEY_I,
        GLFW_KEY_O,
        GLFW_KEY_Y,
        GLFW_KEY_PAGE_UP,
        GLFW_KEY_PAGE_DOWN,
        GLFW_KEY_TAB,
        GLFW_KEY_F5,
        GLFW_KEY_ENTER,
        GLFW_KEY_ESCAPE,
        GLFW_KEY_P,
        GLFW_KEY_L,
        GLFW_KEY_F,
    };
    for (const int key : TrackedKeys) {
        if (inputSystem.IsKeyPressed(key)) {
            return true;
        }
    }

    for (int button = GLFW_MOUSE_BUTTON_1;
         button <= GLFW_MOUSE_BUTTON_LAST;
         ++button) {
        if (inputSystem.IsMouseButtonPressed(button)) {
            return true;
        }
    }
    return false;
}

bool IsGameControllerInputActive(const InputSystem& inputSystem)
{
    if (!inputSystem.HasControllerInput(1)) {
        return false;
    }

    for (int button = 0; button < SDL_CONTROLLER_BUTTON_MAX; ++button) {
        if (inputSystem.IsControllerButtonPressed(1, button)) {
            return true;
        }
    }

    constexpr Sint16 AxisActivityThreshold = 8000;
    for (int axis = 0; axis < SDL_CONTROLLER_AXIS_MAX; ++axis) {
        const auto controllerAxis =
            static_cast<SDL_GameControllerAxis>(axis);
        const int axisValue = inputSystem.GetControllerAxis(
            1, controllerAxis);
        const bool isTrigger =
            controllerAxis == SDL_CONTROLLER_AXIS_TRIGGERLEFT ||
            controllerAxis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
        if ((isTrigger && axisValue > AxisActivityThreshold) ||
            (!isTrigger && std::abs(static_cast<int>(axisValue)) >
                               AxisActivityThreshold)) {
            return true;
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
}

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
        mSnapshot.keys[static_cast<std::size_t>(key)] =
            glfwGetKey(window, key) == GLFW_PRESS;
    }
    for (int button = GLFW_MOUSE_BUTTON_1;
         button <= GLFW_MOUSE_BUTTON_LAST;
         ++button) {
        mSnapshot.mouseButtons[static_cast<std::size_t>(button)] =
            glfwGetMouseButton(window, button) == GLFW_PRESS;
    }
    glfwGetCursorPos(
        window,
        &mSnapshot.cursorX,
        &mSnapshot.cursorY);

    for (int playerIndex = 0; playerIndex < 2; ++playerIndex) {
        SDL_GameController* controller =
            mGame->GetSdlControllerForPlayer(playerIndex + 1);
        if (!controller) {
            continue;
        }
        mSnapshot.hasController[static_cast<std::size_t>(playerIndex)] = true;
        for (int axis = 0; axis < SDL_CONTROLLER_AXIS_MAX; ++axis) {
            mSnapshot.controllerAxes[static_cast<std::size_t>(playerIndex)]
                                    [static_cast<std::size_t>(axis)] =
                static_cast<int>(SDL_GameControllerGetAxis(
                    controller,
                    static_cast<SDL_GameControllerAxis>(axis)));
        }
        for (int button = 0;
             button < SDL_CONTROLLER_BUTTON_MAX;
             ++button) {
            mSnapshot.controllerButtons[static_cast<std::size_t>(playerIndex)]
                                       [static_cast<std::size_t>(button)] =
                SDL_GameControllerGetButton(
                    controller,
                    static_cast<SDL_GameControllerButton>(button)) != 0;
        }
    }
}

bool InputSystem::IsKeyPressed(int key) const
{
    return key >= 0 &&
           key < static_cast<int>(mSnapshot.keys.size()) &&
           mSnapshot.keys[static_cast<std::size_t>(key)];
}

bool InputSystem::IsMouseButtonPressed(int button) const
{
    return button >= 0 &&
           button < static_cast<int>(mSnapshot.mouseButtons.size()) &&
           mSnapshot.mouseButtons[static_cast<std::size_t>(button)];
}

int InputSystem::GetControllerAxis(int playerNum, int axis) const
{
    const int playerIndex = playerNum - 1;
    if (playerIndex < 0 || playerIndex >= 2 ||
        axis < 0 || axis >= 8) {
        return 0;
    }
    return mSnapshot.controllerAxes[static_cast<std::size_t>(playerIndex)]
                                   [static_cast<std::size_t>(axis)];
}

bool InputSystem::IsControllerButtonPressed(
    int playerNum,
    int button) const
{
    const int playerIndex = playerNum - 1;
    if (playerIndex < 0 || playerIndex >= 2 ||
        button < 0 || button >= 32) {
        return false;
    }
    return mSnapshot.controllerButtons[static_cast<std::size_t>(playerIndex)]
                                      [static_cast<std::size_t>(button)];
}

bool InputSystem::HasControllerInput(int playerNum) const
{
    const int playerIndex = playerNum - 1;
    return playerIndex >= 0 && playerIndex < 2 &&
           mSnapshot.hasController[static_cast<std::size_t>(playerIndex)];
}

bool InputSystem::IsMovementInputPressedForPlayer(
    const Player* player) const
{
    if (!mGame || !player) {
        return false;
    }

    const bool isControllerConnected =
        mGame->IsGameControllerConnected();
    const bool isTwoPlayerMode = mGame->GetIsPlayer2Joined();
    const bool usesController =
        isControllerConnected &&
        (isTwoPlayerMode
             ? player->GetPlayerNum() == 1
             : mGame->GetControlledPlayer() == player);
    if (usesController) {
        return IsControllerMovementPressed(*this);
    }

    const bool usesKeyboard =
        isTwoPlayerMode
            ? (isControllerConnected
                   ? player->GetPlayerNum() == 2
                   : player->GetPlayerNum() == 1)
            : (!isControllerConnected &&
               mGame->GetControlledPlayer() == player);
    return usesKeyboard &&
           IsKeyboardMovementPressed(*this);
}

void InputSystem::ProcessGameInput()
{
    if (!mGame || !mGame->GetWindow()) {
        return;
    }

    if (mGame->IsEditorKeyboardInputCaptured()) {
        SuppressOneShotInputUntilReleased();
        return;
    }

    UpdateLastUsedInputDevice();


    ProcessDebugEditorToggleInput();
    ProcessUGCEditorCursorInput();
    const bool isUGCEditorActive =
        mGame->GetIsUGCMode() &&
        mGame->GetIsDebugEditorShowing() &&
        !mGame->GetIsUGCDebugEditorShowing();
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
        mGame->GetSceneSystem() &&
        mGame->GetSceneSystem()->IsTitle() &&
        !mGame->GetIsUGCWorkBrowserShowing();
    ProcessUGCModeInput();
    ProcessTitleMenuInput();
    ProcessBattleStyleSelectionInput();
    // ポーズ決定もAを使うため、そのフレームの入力を会話開始へ流さない。
    ProcessSceneConfirmInput(
        !wasPauseMenuOpen && !isUGCEditorActive && !wasTitleMenuActive);
    ProcessFreeCameraToggleInput();
    ProcessStartInput();
}

void InputSystem::SuppressOneShotInputUntilReleased()
{
    mReloadKeyPressedPrev = true;
    mUIReloadKeyPressedPrev = true;
    mPPressedPrev = true;
    mLPressedPrev = true;
    mQPressedPrev = true;
    mPlayerSplitPressedPrev = true;
    mPlayerSwitchPressedPrev = true;
    mBattleStyleDirectionPressedPrev = true;
    mTitleMenuDirectionPressedPrev = true;
    mTitleMenuConfirmPressedPrev = true;
    mUGCModePressedPrev = true;
    mUGCWorkBrowserPressedPrev = true;
    mStartPressedPrev = true;
    mPauseMenuKeyPressedPrev = true;
    mPauseMenuUpPressedPrev = true;
    mPauseMenuDownPressedPrev = true;
    mPauseMenuConfirmPressedPrev = true;
    mControllerConfirmPressedPrev = true;
    mUGCEditorUndoPressedPrev = true;
    mUGCEditorRedoPressedPrev = true;
    mUGCEditorEraserPressedPrev = true;
    mUGCEditorZoomInPressedPrev = true;
    mUGCEditorZoomOutPressedPrev = true;
    mUGCEditorLayerDownPressedPrev = true;
    mUGCEditorLayerUpPressedPrev = true;
    mUGCEditorPlayPressedPrev = true;
    mUGCEditorSelectionPressedPrev = true;
    mKeyboardConfirmPressedPrev = true;
}

void InputSystem::SuppressUGCPlayShortcutUntilReleased()
{
    mPauseMenuKeyPressedPrev = true;
    mUGCEditorPlayPressedPrev = true;
}

void InputSystem::ProcessUGCModeInput()
{
    SceneSystem* sceneSystem = mGame->GetSceneSystem();
    if (!sceneSystem || !sceneSystem->IsTitle()) {
        mUGCModePressedPrev = false;
        mUGCWorkBrowserPressedPrev = false;
        return;
    }

    if (mGame->GetIsUGCWorkBrowserShowing()) {
        mUGCModePressedPrev = false;
        mUGCWorkBrowserPressedPrev = false;
        return;
    }

    const bool keyboardPressed = IsKeyPressed(GLFW_KEY_C);
    const bool controllerPressed =
        IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_X);
    const bool ugcModePressed = keyboardPressed || controllerPressed;

    if (ugcModePressed && !mUGCModePressedPrev) {
        mGame->StartUGCMode();
    }
    mUGCModePressedPrev = ugcModePressed;

    const bool browserKeyboardPressed = IsKeyPressed(GLFW_KEY_V);
    const bool browserControllerPressed =
        IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_Y);
    const bool browserPressed =
        browserKeyboardPressed || browserControllerPressed;
    if (browserPressed && !mUGCWorkBrowserPressedPrev) {
        mGame->OpenUGCWorkBrowser();
    }
    mUGCWorkBrowserPressedPrev = browserPressed;
}

void InputSystem::ProcessTitleMenuInput()
{
    SceneSystem* sceneSystem = mGame->GetSceneSystem();
    if (!sceneSystem || !sceneSystem->IsTitle() ||
        mGame->GetIsUGCWorkBrowserShowing()) {
        mTitleMenuDirectionPressedPrev = false;
        mTitleMenuConfirmPressedPrev = false;
        return;
    }

    constexpr Sint16 directionThreshold = 16000;
    const bool upPressed =
        IsKeyPressed(GLFW_KEY_UP) ||
        IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_DPAD_UP) ||
        GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTY) < -directionThreshold;
    const bool downPressed =
        IsKeyPressed(GLFW_KEY_DOWN) ||
        IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_DPAD_DOWN) ||
        GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTY) > directionThreshold;
    const bool directionPressed = upPressed || downPressed;
    if (directionPressed && !mTitleMenuDirectionPressedPrev) {
        mGame->MoveTitleMenuSelection(upPressed ? -1 : 1);
    }
    mTitleMenuDirectionPressedPrev = directionPressed;

    const bool confirmPressed =
        IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_A) ||
        IsKeyPressed(GLFW_KEY_SPACE);
    if (confirmPressed && !mTitleMenuConfirmPressedPrev) {
        mGame->ExecuteTitleMenuSelection();
    }
    mTitleMenuConfirmPressedPrev = confirmPressed;
}

void InputSystem::ProcessUGCEditorCursorInput()
{
    const bool isUGCEditorActive =
        mGame->GetIsUGCMode() &&
        mGame->GetIsDebugEditorShowing() &&
        !mGame->GetIsUGCDebugEditorShowing() &&
        HasControllerInput(1);
    if (!isUGCEditorActive) {
        if (mUGCEditorControllerClickPressedPrev &&
            ImGui::GetCurrentContext()) {
            const bool isPhysicalLeftMousePressed =
                IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
            ImGui::GetIO().AddMouseButtonEvent(
                ImGuiMouseButton_Left,
                isPhysicalLeftMousePressed);
        }
        mUGCEditorControllerClickPressedPrev = false;
        return;
    }

    const bool isControllerClickPressed =
        IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_A);
    if (isControllerClickPressed !=
            mUGCEditorControllerClickPressedPrev &&
        ImGui::GetCurrentContext()) {
        const bool isPhysicalLeftMousePressed =
            IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT);
        ImGui::GetIO().AddMouseButtonEvent(
            ImGuiMouseButton_Left,
            isControllerClickPressed || isPhysicalLeftMousePressed);
    }
    mUGCEditorControllerClickPressedPrev = isControllerClickPressed;

    constexpr float axisScale = 1.0f / 32767.0f;
    constexpr float deadZone = 0.2f;
    float previewTurnInput =
        GetControllerAxis(1, SDL_CONTROLLER_AXIS_RIGHTX) * axisScale;
    if (std::abs(previewTurnInput) < deadZone) {
        previewTurnInput = 0.0f;
    }
    if (previewTurnInput != 0.0f) {
        constexpr float previewTurnRadiansPerSecond = 1.8f;
        const float deltaTime = std::max(
            0.001f, mGame->GetLastDeltaTime());
        mGame->AdjustUGCPreviewYaw(
            -previewTurnInput * previewTurnRadiansPerSecond * deltaTime);
    }

    if (!mGame->GetCameraSystem()) {
        return;
    }

    float horizontalInput =
        GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTX) * axisScale;
    float verticalInput =
        GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTY) * axisScale;
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
    const float deltaTime = std::max(
        0.001f, mGame->GetLastDeltaTime());
    cursorX += horizontalInput *
        normalCursorSpeedPixelsPerSecond * deltaTime;
    cursorY += verticalInput *
        normalCursorSpeedPixelsPerSecond * deltaTime;
    cursorX = std::clamp(
        cursorX, 0.0, static_cast<double>(windowWidth - 1));
    cursorY = std::clamp(
        cursorY, 0.0, static_cast<double>(windowHeight - 1));
    mShouldIgnoreNextSyntheticCursorMotion = true;
    glfwSetCursorPos(window, cursorX, cursorY);

    constexpr float edgeScrollAreaPixels = 24.0f;
    glm::vec2 edgeScrollDirection(0.0f);
    if (cursorX <= edgeScrollAreaPixels && horizontalInput < 0.0f) {
        edgeScrollDirection.x = -1.0f;
    } else if (cursorX >= windowWidth - edgeScrollAreaPixels &&
               horizontalInput > 0.0f) {
        edgeScrollDirection.x = 1.0f;
    }
    if (cursorY <= edgeScrollAreaPixels && verticalInput < 0.0f) {
        edgeScrollDirection.y = 1.0f;
    } else if (cursorY >= windowHeight - edgeScrollAreaPixels &&
               verticalInput > 0.0f) {
        edgeScrollDirection.y = -1.0f;
    }
    if (edgeScrollDirection == glm::vec2(0.0f)) {
        return;
    }

    CameraPose cameraPose =
        mGame->GetCameraSystem()->GetDebugCameraPose();
    const glm::vec3 forward = glm::normalize(
        cameraPose.target - cameraPose.position);
    const glm::vec3 right = glm::normalize(
        glm::cross(forward, cameraPose.up));
    constexpr float normalViewScrollWorldUnitsPerSecond = 10.0f;
    const glm::vec3 viewMovement =
        (right * edgeScrollDirection.x +
         cameraPose.up * edgeScrollDirection.y) *
        normalViewScrollWorldUnitsPerSecond * deltaTime;
    cameraPose.position += viewMovement;
    cameraPose.target += viewMovement;
    mGame->GetCameraSystem()->SetDebugCameraPose(cameraPose);
}

void InputSystem::ProcessUGCEditorCommandInput()
{
    const bool undoPressed =
        IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_B) ||
        IsKeyPressed(GLFW_KEY_U);
    const bool redoPressed =
        IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_Y) ||
        IsKeyPressed(GLFW_KEY_J);
    const bool eraserPressed =
        IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_X) ||
        IsKeyPressed(GLFW_KEY_K);
    const bool zoomInPressed =
        IsControllerButtonPressed(
            1, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) ||
        IsKeyPressed(GLFW_KEY_M);
    const bool zoomOutPressed =
        IsControllerButtonPressed(
            1, SDL_CONTROLLER_BUTTON_LEFTSHOULDER) ||
        IsKeyPressed(GLFW_KEY_N);
    constexpr Sint16 triggerPressedThreshold = 16000;
    const bool layerDownPressed =
        GetControllerAxis(1, SDL_CONTROLLER_AXIS_TRIGGERLEFT) >
            triggerPressedThreshold ||
        IsKeyPressed(GLFW_KEY_Y);
    const bool layerUpPressed =
        GetControllerAxis(1, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) >
            triggerPressedThreshold ||
        IsKeyPressed(GLFW_KEY_I);
    const bool playPressed =
        IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_BACK) ||
        IsKeyPressed(GLFW_KEY_ESCAPE);
    const bool selectionPressed =
        IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_START) ||
        IsKeyPressed(GLFW_KEY_ENTER);
    const bool dpadLeftPressed =
        IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    const bool dpadRightPressed =
        IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
    const bool dpadUpPressed =
        IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_DPAD_UP);
    const bool dpadDownPressed =
        IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_DPAD_DOWN);

    float previewTurnInput = 0.0f;
    if (IsKeyPressed(GLFW_KEY_LEFT)) {
        previewTurnInput = -1.0f;
    } else if (IsKeyPressed(GLFW_KEY_RIGHT)) {
        previewTurnInput = 1.0f;
    }
    if (previewTurnInput != 0.0f) {
        constexpr float previewTurnRadiansPerSecond = 1.8f;
        const float deltaTime = std::max(
            0.001f,
            mGame->GetLastDeltaTime());
        mGame->AdjustUGCPreviewYaw(
            -previewTurnInput * previewTurnRadiansPerSecond * deltaTime);
    }

    if (undoPressed && !mUGCEditorUndoPressedPrev) {
        mGame->UndoUGCEdit();
    }
    if (redoPressed && !mUGCEditorRedoPressedPrev) {
        mGame->RedoUGCEdit();
    }
    if (eraserPressed && !mUGCEditorEraserPressedPrev) {
        mGame->ToggleUGCEraser();
    }
    if (zoomInPressed && !mUGCEditorZoomInPressedPrev) {
        mGame->ZoomUGCEditor(0.8f);
    }
    if (zoomOutPressed && !mUGCEditorZoomOutPressedPrev) {
        mGame->ZoomUGCEditor(1.25f);
    }
    if (layerDownPressed && !mUGCEditorLayerDownPressedPrev) {
        mGame->ChangeUGCEditLayer(-1);
    }
    if (layerUpPressed && !mUGCEditorLayerUpPressedPrev) {
        mGame->ChangeUGCEditLayer(1);
    }
    if (selectionPressed && !mUGCEditorSelectionPressedPrev) {
        mGame->SelectUGCEditorMode();
    }
    if (playPressed && !mUGCEditorPlayPressedPrev) {
        mGame->StartUGCPlaytest();
    }
    if (dpadLeftPressed && !mUGCEditorDpadLeftPressedPrev) {
        mGame->MoveUGCSelectionByGrid(-1, 0);
    }
    if (dpadRightPressed && !mUGCEditorDpadRightPressedPrev) {
        mGame->MoveUGCSelectionByGrid(1, 0);
    }
    if (dpadUpPressed && !mUGCEditorDpadUpPressedPrev) {
        mGame->MoveUGCSelectionByGrid(0, -1);
    }
    if (dpadDownPressed && !mUGCEditorDpadDownPressedPrev) {
        mGame->MoveUGCSelectionByGrid(0, 1);
    }

    mUGCEditorUndoPressedPrev = undoPressed;
    mUGCEditorRedoPressedPrev = redoPressed;
    mUGCEditorEraserPressedPrev = eraserPressed;
    mUGCEditorZoomInPressedPrev = zoomInPressed;
    mUGCEditorZoomOutPressedPrev = zoomOutPressed;
    mUGCEditorLayerDownPressedPrev = layerDownPressed;
    mUGCEditorLayerUpPressedPrev = layerUpPressed;
    mUGCEditorPlayPressedPrev = playPressed;
    mUGCEditorSelectionPressedPrev = selectionPressed;
    mUGCEditorDpadLeftPressedPrev = dpadLeftPressed;
    mUGCEditorDpadRightPressedPrev = dpadRightPressed;
    mUGCEditorDpadUpPressedPrev = dpadUpPressed;
    mUGCEditorDpadDownPressedPrev = dpadDownPressed;


    mPauseMenuKeyPressedPrev = playPressed;
    mStartPressedPrev = selectionPressed;
}

void InputSystem::ProcessPauseToggleInput()
{
    SceneSystem* sceneSystem = mGame->GetSceneSystem();
    if (!sceneSystem) {
        return;
    }

    const bool escapePressed = IsKeyPressed(GLFW_KEY_ESCAPE);
    const bool controllerBackPressed =
        IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_BACK);

    const bool returnToUGCEditorPressed =
        escapePressed || controllerBackPressed;
    if (returnToUGCEditorPressed && !mPauseMenuKeyPressedPrev &&
        mGame->GetIsUGCPlaytestActive()) {
        mGame->ReturnToUGCEditor();
        SuppressUGCPlayShortcutUntilReleased();
        return;
    }

    const bool pauseMenuKeyPressed =
        escapePressed || controllerBackPressed;

    if (pauseMenuKeyPressed && !mPauseMenuKeyPressedPrev &&
        (mGame->GetIsPauseMenuOpen() || mGame->CanOpenPauseMenu())) {
        mGame->TogglePauseMenu();
    }

    mPauseMenuKeyPressedPrev = pauseMenuKeyPressed;
}

void InputSystem::ProcessPauseMenuInput()
{
    const bool upPressed =
        IsKeyPressed(GLFW_KEY_UP) ||
        IsKeyPressed(GLFW_KEY_W) ||
        IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_DPAD_UP);

    const bool downPressed =
        IsKeyPressed(GLFW_KEY_DOWN) ||
        IsKeyPressed(GLFW_KEY_S) ||
        IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_DPAD_DOWN);

    const bool confirmPressed =
        IsKeyPressed(GLFW_KEY_SPACE) ||
        IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_A);

    if (upPressed && !mPauseMenuUpPressedPrev) {
        mGame->MovePauseMenuSelection(-1);
    }

    if (downPressed && !mPauseMenuDownPressedPrev) {
        mGame->MovePauseMenuSelection(1);
    }

    if (confirmPressed && !mPauseMenuConfirmPressedPrev) {
        mGame->ExecutePauseMenuItem();
    }

    mPauseMenuUpPressedPrev = upPressed;
    mPauseMenuDownPressedPrev = downPressed;
    mPauseMenuConfirmPressedPrev = confirmPressed;
}

void InputSystem::ProcessDebugReloadInput()
{
    const bool reloadKeyPressed = IsKeyPressed(GLFW_KEY_F);
    if (mGame->GetIsDebugMode() && reloadKeyPressed && !mReloadKeyPressedPrev) {
        mGame->ReloadCurrentStage();
    }
    mReloadKeyPressedPrev = reloadKeyPressed;

    const bool uiReloadKeyPressed = IsKeyPressed(GLFW_KEY_O);
    if (mGame->GetIsDebugMode() && uiReloadKeyPressed && !mUIReloadKeyPressedPrev) {
        mGame->ReloadUIData();
    }
    mUIReloadKeyPressedPrev = uiReloadKeyPressed;
}

void InputSystem::ProcessPlayerJoinInput()
{
    const bool qPressed = IsKeyPressed(GLFW_KEY_Q);
    if (qPressed && !mQPressedPrev) {
        if (mGame->IsGameControllerConnected() && !mGame->GetIsPlayer2Joined()) {
            mGame->TryCreatePlayer2();
        }
    }
    mQPressedPrev = qPressed;
}

void InputSystem::UpdateLastUsedInputDevice()
{
    const double cursorX = GetCursorX();
    const double cursorY = GetCursorY();

    constexpr double CursorMovementThresholdPixels = 0.25;
    const bool hasMouseMoved =
        mHasPreviousCursorPosition &&
        (std::abs(cursorX - mPreviousCursorX) >
             CursorMovementThresholdPixels ||
         std::abs(cursorY - mPreviousCursorY) >
             CursorMovementThresholdPixels);
    mPreviousCursorX = cursorX;
    mPreviousCursorY = cursorY;
    mHasPreviousCursorPosition = true;

    const bool hasPhysicalMouseMoved =
        hasMouseMoved && !mShouldIgnoreNextSyntheticCursorMotion;
    mShouldIgnoreNextSyntheticCursorMotion = false;

    if (IsKeyboardOrMouseInputActive(*this) || hasPhysicalMouseMoved) {
        mGame->RecordInputDeviceUsage(InputDeviceType::KeyboardMouse);
    }

    if (IsGameControllerInputActive(*this)) {
        mGame->RecordInputDeviceUsage(InputDeviceType::GameController);
    }

    const bool isKeyboardModifierHeld =
        IsKeyPressed(GLFW_KEY_N);
    const bool isGameControllerModifierHeld =
        IsControllerButtonPressed(
            1, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
    mGame->SetInputModifierHeld(
        isKeyboardModifierHeld || isGameControllerModifierHeld);
}

void InputSystem::ProcessPlayerSwitchInput()
{
    constexpr Sint16 triggerPressedThreshold = 16000;
    const bool switchPressed =
        IsKeyPressed(GLFW_KEY_Y) ||
        GetControllerAxis(1, SDL_CONTROLLER_AXIS_TRIGGERLEFT) >
            triggerPressedThreshold;

    if (switchPressed && !mPlayerSwitchPressedPrev) {
        mGame->SwitchControlledPlayer();
    }

    mPlayerSwitchPressedPrev = switchPressed;
}

void InputSystem::ProcessPlayerSplitInput()
{
    constexpr Sint16 triggerPressedThreshold = 16000;
    const bool splitPressed =
        IsKeyPressed(GLFW_KEY_I) ||
        GetControllerAxis(1, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) >
            triggerPressedThreshold;

    if (splitPressed && !mPlayerSplitPressedPrev) {
        mGame->TogglePlayerSplit();
    }

    mPlayerSplitPressedPrev = splitPressed;
}

void InputSystem::ProcessBattleStyleSelectionInput()
{
    SceneSystem* sceneSystem = mGame->GetSceneSystem();
    if (!sceneSystem || !sceneSystem->IsBattleStyleSelection()) {
        mBattleStyleDirectionPressedPrev = false;
        return;
    }

    constexpr Sint16 DirectionThreshold = 16000;

    const bool previousDirectionPressed =
        IsKeyPressed(GLFW_KEY_LEFT) ||
        IsKeyPressed(GLFW_KEY_UP) ||
        IsKeyPressed(GLFW_KEY_A) ||
        IsKeyPressed(GLFW_KEY_W) ||
        IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_DPAD_LEFT) ||
        IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_DPAD_UP) ||
        GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTX) <
            -DirectionThreshold ||
        GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTY) <
            -DirectionThreshold;
    const bool nextDirectionPressed =
        IsKeyPressed(GLFW_KEY_RIGHT) ||
        IsKeyPressed(GLFW_KEY_DOWN) ||
        IsKeyPressed(GLFW_KEY_D) ||
        IsKeyPressed(GLFW_KEY_S) ||
        IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) ||
        IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_DPAD_DOWN) ||
        GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTX) >
            DirectionThreshold ||
        GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTY) >
            DirectionThreshold;
    const bool directionPressed =
        previousDirectionPressed || nextDirectionPressed;

    if (directionPressed && !mBattleStyleDirectionPressedPrev) {
        sceneSystem->MoveBattleStyleSelection(
            previousDirectionPressed ? -1 : 1);
    }
    mBattleStyleDirectionPressedPrev = directionPressed;
}

void InputSystem::ProcessSceneConfirmInput(bool allowsSceneAction)
{
    SceneSystem* sceneSystem = mGame->GetSceneSystem();
    if (!sceneSystem) {
        return;
    }

    const bool controllerConfirmPressed =
        IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_A);

    const bool keyboardConfirmPressed =
        IsKeyPressed(GLFW_KEY_SPACE);

    const Player* controlledPlayer = mGame->GetControlledPlayer();
    const int controlledPlayerNum =
        controlledPlayer ? controlledPlayer->GetPlayerNum() : 1;
    const bool isTwoPlayerMode = mGame->GetIsPlayer2Joined();

    if (allowsSceneAction &&
        controllerConfirmPressed &&
        !mControllerConfirmPressedPrev) {
        const int controllerPlayerNum =
            isTwoPlayerMode ? 1 : controlledPlayerNum;
        if (sceneSystem->OnConfirmPressed(controllerPlayerNum)) {
            SuppressPlayerJumpUntilReleased(
                *mGame,
                controllerPlayerNum);
        }
    }

    if (allowsSceneAction &&
        keyboardConfirmPressed &&
        !mKeyboardConfirmPressedPrev) {
        const int keyboardPlayerNum =
            isTwoPlayerMode ? 2 : controlledPlayerNum;
        if (sceneSystem->OnConfirmPressed(keyboardPlayerNum)) {
            SuppressPlayerJumpUntilReleased(
                *mGame,
                keyboardPlayerNum);
        }
    }

    mControllerConfirmPressedPrev = controllerConfirmPressed;
    mKeyboardConfirmPressedPrev = keyboardConfirmPressed;
}

void InputSystem::ProcessDebugEditorToggleInput()
{
    const bool pPressed = IsKeyPressed(GLFW_KEY_P);
    if ((mGame->GetIsDebugMode() || mGame->GetIsUGCMode()) &&
        pPressed && !mPPressedPrev) {
        mGame->ToggleDebugEditor();
    }
    mPPressedPrev = pPressed;
}

void InputSystem::ProcessFreeCameraToggleInput()
{
    const bool lPressed = IsKeyPressed(GLFW_KEY_L);
    if (mGame->GetIsDebugMode() && lPressed && !mLPressedPrev) {
        mGame->ToggleFreeCameraMode();
    }
    mLPressedPrev = lPressed;
}

void InputSystem::ProcessStartInput()
{
    SceneSystem* sceneSystem = mGame->GetSceneSystem();
    if (!sceneSystem) {
        return;
    }

    const bool startPressed =
        IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_START) ||
        IsKeyPressed(GLFW_KEY_ENTER);

    if (startPressed && !mStartPressedPrev) {
        sceneSystem->OnStartPressed();
    }

    mStartPressedPrev = startPressed;
}
