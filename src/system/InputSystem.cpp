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

bool IsControllerMovementPressed(SDL_GameController* controller)
{
    if (!controller) {
        return false;
    }

    const int horizontalAxis = static_cast<int>(
        SDL_GameControllerGetAxis(
            controller,
            SDL_CONTROLLER_AXIS_LEFTX));
    const int verticalAxis = static_cast<int>(
        SDL_GameControllerGetAxis(
            controller,
            SDL_CONTROLLER_AXIS_LEFTY));

    return std::abs(horizontalAxis) >= controllerMovementDeadZone ||
           std::abs(verticalAxis) >= controllerMovementDeadZone;
}

bool IsKeyboardMovementPressed(GLFWwindow* window)
{
    if (!window) {
        return false;
    }

    return glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
           glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
           glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
           glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
}

bool IsKeyboardOrMouseInputActive(GLFWwindow* window)
{
    if (!window) {
        return false;
    }

    constexpr std::array<int, 21> TrackedKeys = {
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
        GLFW_KEY_U,
        GLFW_KEY_N,
        GLFW_KEY_I,
        GLFW_KEY_O,
        GLFW_KEY_Y,
        GLFW_KEY_ENTER,
        GLFW_KEY_ESCAPE,
        GLFW_KEY_P,
        GLFW_KEY_L,
        GLFW_KEY_F,
    };
    for (const int key : TrackedKeys) {
        if (glfwGetKey(window, key) == GLFW_PRESS) {
            return true;
        }
    }

    for (int button = GLFW_MOUSE_BUTTON_1;
         button <= GLFW_MOUSE_BUTTON_LAST;
         ++button) {
        if (glfwGetMouseButton(window, button) == GLFW_PRESS) {
            return true;
        }
    }
    return false;
}

bool IsGameControllerInputActive(SDL_GameController* controller)
{
    if (!controller) {
        return false;
    }

    for (int button = 0; button < SDL_CONTROLLER_BUTTON_MAX; ++button) {
        if (SDL_GameControllerGetButton(
                controller,
                static_cast<SDL_GameControllerButton>(button))) {
            return true;
        }
    }

    constexpr Sint16 AxisActivityThreshold = 8000;
    for (int axis = 0; axis < SDL_CONTROLLER_AXIS_MAX; ++axis) {
        const auto controllerAxis =
            static_cast<SDL_GameControllerAxis>(axis);
        const Sint16 axisValue =
            SDL_GameControllerGetAxis(controller, controllerAxis);
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
} // namespace

InputSystem::InputSystem(Game* game)
    : mGame(game)
{
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
        return IsControllerMovementPressed(
            mGame->GetSdlController());
    }

    const bool usesKeyboard =
        isTwoPlayerMode
            ? (isControllerConnected
                   ? player->GetPlayerNum() == 2
                   : player->GetPlayerNum() == 1)
            : (!isControllerConnected &&
               mGame->GetControlledPlayer() == player);
    return usesKeyboard &&
           IsKeyboardMovementPressed(mGame->GetWindow());
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
    // UGC editor input normally returns early below.  Toggle handling must
    // happen first so P can also close/open the editor while creating.
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

    const bool keyboardPressed =
        glfwGetKey(mGame->GetWindow(), GLFW_KEY_C) == GLFW_PRESS;
    const bool controllerPressed =
        mGame->GetSdlController() &&
        SDL_GameControllerGetButton(
            mGame->GetSdlController(),
            SDL_CONTROLLER_BUTTON_X);
    const bool ugcModePressed = keyboardPressed || controllerPressed;

    if (ugcModePressed && !mUGCModePressedPrev) {
        mGame->StartUGCMode();
    }
    mUGCModePressedPrev = ugcModePressed;

    const bool browserKeyboardPressed =
        glfwGetKey(mGame->GetWindow(), GLFW_KEY_V) == GLFW_PRESS;
    const bool browserControllerPressed =
        mGame->GetSdlController() &&
        SDL_GameControllerGetButton(
            mGame->GetSdlController(),
            SDL_CONTROLLER_BUTTON_Y);
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

    GLFWwindow* window = mGame->GetWindow();
    SDL_GameController* controller = mGame->GetSdlController();
    constexpr Sint16 directionThreshold = 16000;
    const bool upPressed =
        glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS ||
        (controller &&
         (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP) ||
          SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY) < -directionThreshold));
    const bool downPressed =
        glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS ||
        (controller &&
         (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN) ||
          SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY) > directionThreshold));
    const bool directionPressed = upPressed || downPressed;
    if (directionPressed && !mTitleMenuDirectionPressedPrev) {
        mGame->MoveTitleMenuSelection(upPressed ? -1 : 1);
    }
    mTitleMenuDirectionPressedPrev = directionPressed;

    const bool confirmPressed =
        (controller && SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A)) ||
        glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;
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
        mGame->GetSdlController();
    if (!isUGCEditorActive) {
        if (mUGCEditorControllerClickPressedPrev &&
            ImGui::GetCurrentContext()) {
            const bool isPhysicalLeftMousePressed =
                glfwGetMouseButton(
                    mGame->GetWindow(), GLFW_MOUSE_BUTTON_LEFT) ==
                GLFW_PRESS;
            ImGui::GetIO().AddMouseButtonEvent(
                ImGuiMouseButton_Left,
                isPhysicalLeftMousePressed);
        }
        mUGCEditorControllerClickPressedPrev = false;
        return;
    }

    SDL_GameController* controller = mGame->GetSdlController();
    const bool isControllerClickPressed =
        SDL_GameControllerGetButton(
            controller, SDL_CONTROLLER_BUTTON_A) != 0;
    if (isControllerClickPressed !=
            mUGCEditorControllerClickPressedPrev &&
        ImGui::GetCurrentContext()) {
        const bool isPhysicalLeftMousePressed =
            glfwGetMouseButton(
                mGame->GetWindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        ImGui::GetIO().AddMouseButtonEvent(
            ImGuiMouseButton_Left,
            isControllerClickPressed || isPhysicalLeftMousePressed);
    }
    mUGCEditorControllerClickPressedPrev = isControllerClickPressed;

    constexpr float axisScale = 1.0f / 32767.0f;
    constexpr float deadZone = 0.2f;
    float previewTurnInput =
        SDL_GameControllerGetAxis(
            controller, SDL_CONTROLLER_AXIS_RIGHTX) * axisScale;
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
        SDL_GameControllerGetAxis(
            controller, SDL_CONTROLLER_AXIS_LEFTX) * axisScale;
    float verticalInput =
        SDL_GameControllerGetAxis(
            controller, SDL_CONTROLLER_AXIS_LEFTY) * axisScale;
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
    double cursorX = 0.0;
    double cursorY = 0.0;
    glfwGetCursorPos(window, &cursorX, &cursorY);

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
    SDL_GameController* controller = mGame->GetSdlController();
    if (!controller) {
        mUGCEditorUndoPressedPrev = false;
        mUGCEditorRedoPressedPrev = false;
        mUGCEditorEraserPressedPrev = false;
        mUGCEditorZoomInPressedPrev = false;
        mUGCEditorZoomOutPressedPrev = false;
        mUGCEditorLayerDownPressedPrev = false;
        mUGCEditorLayerUpPressedPrev = false;
        mUGCEditorPlayPressedPrev = false;
        mUGCEditorSelectionPressedPrev = false;
        mUGCEditorDpadLeftPressedPrev = false;
        mUGCEditorDpadRightPressedPrev = false;
        mUGCEditorDpadUpPressedPrev = false;
        mUGCEditorDpadDownPressedPrev = false;
        return;
    }

    const bool undoPressed = SDL_GameControllerGetButton(
        controller, SDL_CONTROLLER_BUTTON_B) != 0;
    const bool redoPressed = SDL_GameControllerGetButton(
        controller, SDL_CONTROLLER_BUTTON_Y) != 0;
    const bool eraserPressed = SDL_GameControllerGetButton(
        controller, SDL_CONTROLLER_BUTTON_X) != 0;
    const bool zoomInPressed = SDL_GameControllerGetButton(
        controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER) != 0;
    const bool zoomOutPressed = SDL_GameControllerGetButton(
        controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) != 0;
    constexpr Sint16 triggerPressedThreshold = 16000;
    const bool layerDownPressed = SDL_GameControllerGetAxis(
        controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT) >
        triggerPressedThreshold;
    const bool layerUpPressed = SDL_GameControllerGetAxis(
        controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) >
        triggerPressedThreshold;
    const bool playPressed = SDL_GameControllerGetButton(
        controller, SDL_CONTROLLER_BUTTON_BACK) != 0;
    const bool selectionPressed = SDL_GameControllerGetButton(
        controller, SDL_CONTROLLER_BUTTON_START) != 0;
    const bool dpadLeftPressed = SDL_GameControllerGetButton(
        controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT) != 0;
    const bool dpadRightPressed = SDL_GameControllerGetButton(
        controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) != 0;
    const bool dpadUpPressed = SDL_GameControllerGetButton(
        controller, SDL_CONTROLLER_BUTTON_DPAD_UP) != 0;
    const bool dpadDownPressed = SDL_GameControllerGetButton(
        controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN) != 0;

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
    // Prevent held minus/plus buttons from triggering their normal game
    // actions on the frame after leaving the editor.
    mPauseMenuKeyPressedPrev = playPressed;
    mStartPressedPrev = selectionPressed;
}

void InputSystem::ProcessPauseToggleInput()
{
    SceneSystem* sceneSystem = mGame->GetSceneSystem();
    if (!sceneSystem) {
        return;
    }

    const bool escapePressed =
        glfwGetKey(mGame->GetWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS;
    const bool controllerBackPressed =
        mGame->GetSdlController() &&
        SDL_GameControllerGetButton(
            mGame->GetSdlController(), SDL_CONTROLLER_BUTTON_BACK);

    if (controllerBackPressed && !mPauseMenuKeyPressedPrev &&
        mGame->GetIsUGCMode() && !mGame->GetIsDebugEditorShowing()) {
        mGame->ReturnToUGCEditor();
        mPauseMenuKeyPressedPrev = true;
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
        glfwGetKey(mGame->GetWindow(), GLFW_KEY_UP) == GLFW_PRESS ||
        glfwGetKey(mGame->GetWindow(), GLFW_KEY_W) == GLFW_PRESS ||
        (mGame->GetSdlController() &&
         SDL_GameControllerGetButton(mGame->GetSdlController(), SDL_CONTROLLER_BUTTON_DPAD_UP));

    const bool downPressed =
        glfwGetKey(mGame->GetWindow(), GLFW_KEY_DOWN) == GLFW_PRESS ||
        glfwGetKey(mGame->GetWindow(), GLFW_KEY_S) == GLFW_PRESS ||
        (mGame->GetSdlController() &&
         SDL_GameControllerGetButton(mGame->GetSdlController(), SDL_CONTROLLER_BUTTON_DPAD_DOWN));

    const bool confirmPressed =
        glfwGetKey(mGame->GetWindow(), GLFW_KEY_ENTER) == GLFW_PRESS ||
        (mGame->GetSdlController() &&
         SDL_GameControllerGetButton(mGame->GetSdlController(), SDL_CONTROLLER_BUTTON_A));

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
    const bool reloadKeyPressed = glfwGetKey(mGame->GetWindow(), GLFW_KEY_F) == GLFW_PRESS;
    if (mGame->GetIsDebugMode() && reloadKeyPressed && !mReloadKeyPressedPrev) {
        mGame->ReloadCurrentStage();
    }
    mReloadKeyPressedPrev = reloadKeyPressed;

    const bool uiReloadKeyPressed = glfwGetKey(mGame->GetWindow(), GLFW_KEY_O) == GLFW_PRESS;
    if (mGame->GetIsDebugMode() && uiReloadKeyPressed && !mUIReloadKeyPressedPrev) {
        mGame->ReloadUIData();
    }
    mUIReloadKeyPressedPrev = uiReloadKeyPressed;
}

void InputSystem::ProcessPlayerJoinInput()
{
    const bool qPressed = glfwGetKey(mGame->GetWindow(), GLFW_KEY_Q) == GLFW_PRESS;
    if (qPressed && !mQPressedPrev) {
        if (mGame->IsGameControllerConnected() && !mGame->GetIsPlayer2Joined()) {
            mGame->TryCreatePlayer2();
        }
    }
    mQPressedPrev = qPressed;
}

void InputSystem::UpdateLastUsedInputDevice()
{
    GLFWwindow* window = mGame->GetWindow();
    SDL_GameController* controller = mGame->GetSdlController();
    double cursorX = 0.0;
    double cursorY = 0.0;
    glfwGetCursorPos(window, &cursorX, &cursorY);

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

    if (IsKeyboardOrMouseInputActive(window) || hasMouseMoved) {
        mGame->RecordInputDeviceUsage(InputDeviceType::KeyboardMouse);
    }

    if (IsGameControllerInputActive(controller)) {
        mGame->RecordInputDeviceUsage(InputDeviceType::GameController);
    }

    const bool isKeyboardModifierHeld =
        glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS;
    const bool isGameControllerModifierHeld =
        controller &&
        SDL_GameControllerGetButton(
            controller,
            SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
    mGame->SetInputModifierHeld(
        isKeyboardModifierHeld || isGameControllerModifierHeld);
}

void InputSystem::ProcessPlayerSwitchInput()
{
    constexpr Sint16 triggerPressedThreshold = 16000;
    SDL_GameController* controller = mGame->GetSdlController();
    const bool switchPressed =
        glfwGetKey(mGame->GetWindow(), GLFW_KEY_Y) == GLFW_PRESS ||
        (controller &&
         SDL_GameControllerGetAxis(
             controller,
             SDL_CONTROLLER_AXIS_TRIGGERLEFT) > triggerPressedThreshold);

    if (switchPressed && !mPlayerSwitchPressedPrev) {
        mGame->SwitchControlledPlayer();
    }

    mPlayerSwitchPressedPrev = switchPressed;
}

void InputSystem::ProcessPlayerSplitInput()
{
    constexpr Sint16 triggerPressedThreshold = 16000;
    SDL_GameController* controller = mGame->GetSdlController();
    const bool splitPressed =
        glfwGetKey(mGame->GetWindow(), GLFW_KEY_I) == GLFW_PRESS ||
        (controller &&
         SDL_GameControllerGetAxis(
             controller,
             SDL_CONTROLLER_AXIS_TRIGGERRIGHT) >
             triggerPressedThreshold);

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

    GLFWwindow* window = mGame->GetWindow();
    SDL_GameController* controller = mGame->GetSdlController();
    constexpr Sint16 DirectionThreshold = 16000;

    const bool previousDirectionPressed =
        glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
        (controller &&
         (SDL_GameControllerGetButton(
              controller,
              SDL_CONTROLLER_BUTTON_DPAD_LEFT) ||
          SDL_GameControllerGetButton(
              controller,
              SDL_CONTROLLER_BUTTON_DPAD_UP) ||
          SDL_GameControllerGetAxis(
              controller,
              SDL_CONTROLLER_AXIS_LEFTX) < -DirectionThreshold ||
          SDL_GameControllerGetAxis(
              controller,
              SDL_CONTROLLER_AXIS_LEFTY) < -DirectionThreshold));
    const bool nextDirectionPressed =
        glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
        (controller &&
         (SDL_GameControllerGetButton(
              controller,
              SDL_CONTROLLER_BUTTON_DPAD_RIGHT) ||
          SDL_GameControllerGetButton(
              controller,
              SDL_CONTROLLER_BUTTON_DPAD_DOWN) ||
          SDL_GameControllerGetAxis(
              controller,
              SDL_CONTROLLER_AXIS_LEFTX) > DirectionThreshold ||
          SDL_GameControllerGetAxis(
              controller,
              SDL_CONTROLLER_AXIS_LEFTY) > DirectionThreshold));
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

    SDL_GameController* controller = mGame->GetSdlController();
    GLFWwindow* window = mGame->GetWindow();

    const bool controllerConfirmPressed =
        controller &&
        SDL_GameControllerGetButton(
            controller,
            SDL_CONTROLLER_BUTTON_A);

    const bool keyboardConfirmPressed =
        glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;

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
    const bool pPressed = glfwGetKey(mGame->GetWindow(), GLFW_KEY_P) == GLFW_PRESS;
    if ((mGame->GetIsDebugMode() || mGame->GetIsUGCMode()) &&
        pPressed && !mPPressedPrev) {
        mGame->ToggleDebugEditor();
    }
    mPPressedPrev = pPressed;
}

void InputSystem::ProcessFreeCameraToggleInput()
{
    const bool lPressed = glfwGetKey(mGame->GetWindow(), GLFW_KEY_L) == GLFW_PRESS;
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
        (mGame->GetSdlController() &&
         SDL_GameControllerGetButton(mGame->GetSdlController(), SDL_CONTROLLER_BUTTON_START)) ||
        glfwGetKey(mGame->GetWindow(), GLFW_KEY_ENTER) == GLFW_PRESS;

    if (startPressed && !mStartPressedPrev) {
        sceneSystem->OnStartPressed();
    }

    mStartPressedPrev = startPressed;
}
