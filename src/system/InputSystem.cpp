#include "system/InputSystem.h"

#include "Game.h"
#include "actor/Player.h"
#include "system/SceneSystem.h"

#include <GLFW/glfw3.h>
#include <SDL.h>

#include <cstdlib>
#include <array>
#include <cmath>

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
    ProcessPauseToggleInput();

    const bool wasPauseMenuOpen = mGame->GetIsPauseMenuOpen();
    if (wasPauseMenuOpen) {
        ProcessPauseMenuInput();
    }

    ProcessDebugReloadInput();
    ProcessPlayerJoinInput();
    ProcessPlayerSplitInput();
    ProcessPlayerSwitchInput();
    ProcessBattleStyleSelectionInput();
    // ポーズ決定もAを使うため、そのフレームの入力を会話開始へ流さない。
    ProcessSceneConfirmInput(!wasPauseMenuOpen);
    ProcessDebugEditorToggleInput();
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
    mStartPressedPrev = true;
    mPauseMenuKeyPressedPrev = true;
    mPauseMenuUpPressedPrev = true;
    mPauseMenuDownPressedPrev = true;
    mPauseMenuConfirmPressedPrev = true;
    mControllerConfirmPressedPrev = true;
    mKeyboardConfirmPressedPrev = true;
}

void InputSystem::ProcessPauseToggleInput()
{
    SceneSystem* sceneSystem = mGame->GetSceneSystem();
    if (!sceneSystem) {
        return;
    }

    const bool pauseMenuKeyPressed =
        glfwGetKey(mGame->GetWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS ||
        (mGame->GetSdlController() &&
         SDL_GameControllerGetButton(mGame->GetSdlController(), SDL_CONTROLLER_BUTTON_BACK));

    if (pauseMenuKeyPressed && !mPauseMenuKeyPressedPrev &&
        (sceneSystem->IsPlaying() || sceneSystem->IsFocusing() || mGame->GetIsPauseMenuOpen())) {
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
    if (mGame->GetIsDebugMode() && pPressed && !mPPressedPrev) {
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
