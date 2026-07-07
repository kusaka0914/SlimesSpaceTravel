#include "system/InputSystem.h"

#include "Game.h"
#include "system/SceneSystem.h"

#include <GLFW/glfw3.h>
#include <SDL.h>

InputSystem::InputSystem(Game* game)
    : mGame(game)
{
}

void InputSystem::ProcessGameInput()
{
    if (!mGame || !mGame->GetWindow()) {
        return;
    }

    ProcessPauseToggleInput();

    if (mGame->GetIsPauseMenuOpen()) {
        ProcessPauseMenuInput();
    }

    ProcessDebugReloadInput();
    ProcessPlayerJoinInput();
    ProcessSceneConfirmInput();
    ProcessDebugEditorToggleInput();
    ProcessFreeCameraToggleInput();
    ProcessStartInput();
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

    const bool uiReloadKeyPressed = glfwGetKey(mGame->GetWindow(), GLFW_KEY_I) == GLFW_PRESS;
    if (mGame->GetIsDebugMode() && uiReloadKeyPressed && !mUIReloadKeyPressedPrev) {
        mGame->ReloadUIData();
    }
    mUIReloadKeyPressedPrev = uiReloadKeyPressed;
}

void InputSystem::ProcessPlayerJoinInput()
{
    const bool qPressed = glfwGetKey(mGame->GetWindow(), GLFW_KEY_Q) == GLFW_PRESS;
    if (qPressed && !mQPressedPrev) {
        if (mGame->IsGameControllerConnected() && !mGame->GetIsPlayer2Joined() && mGame->GetPlayers().size() < 2) {
            mGame->TryCreatePlayer2();
        }
    }
    mQPressedPrev = qPressed;
}

void InputSystem::ProcessSceneConfirmInput()
{
    SceneSystem* sceneSystem = mGame->GetSceneSystem();
    if (!sceneSystem) {
        return;
    }

    const bool controllerConfirmPressed =
        mGame->GetSdlController() &&
        SDL_GameControllerGetButton(mGame->GetSdlController(), SDL_CONTROLLER_BUTTON_X);

    const bool keyboardConfirmPressed = glfwGetKey(mGame->GetWindow(), GLFW_KEY_K) == GLFW_PRESS;

    if (controllerConfirmPressed && !mControllerConfirmPressedPrev) {
        sceneSystem->OnConfirmPressed(1);
    }

    if (keyboardConfirmPressed && !mKeyboardConfirmPressedPrev) {
        const int keyboardPlayerNum = mGame->IsGameControllerConnected() && mGame->GetIsPlayer2Joined() ? 2 : 1;
        sceneSystem->OnConfirmPressed(keyboardPlayerNum);
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
