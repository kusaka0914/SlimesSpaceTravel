#include "actor/player/PlayerInput.h"

#include "Game.h"

#include "actor/Player.h"
#include "actor/player/PlayerMovement.h"
#include "system/InputSystem.h"
#include "system/SceneSystem.h"

#include <GLFW/glfw3.h>
#include <SDL.h>
#include <cmath>
#include <glm/glm.hpp>

PlayerInput::PlayerInput(InputSystem& inputSystem)
    : mInputSystem(inputSystem)
{
}

void PlayerInput::ProcessActor(Player& player, const PlayerMovement& movement)
{
    const bool isNonControlledSoloPlayer =
        !player.GetGame()->GetIsPlayer2Joined() &&
        player.GetGame()->GetPlayers().size() >= 2 &&
        player.GetGame()->GetControlledPlayer() != &player;
    if (isNonControlledSoloPlayer) {
        ClearNonControlledPlayerInput();
        return;
    }

    if (mInputAvailableTimer >= 0.0f) {
        SceneSystem* sceneSystem =
            player.GetGame()->GetSceneSystem();
        if (sceneSystem &&
            sceneSystem->IsWaitingForTutorialPlayerJump()) {
            mJumpPressed = false;
            ApplyTutorialInputRestriction(player);
        }
        return;
    }

    ProcessGameController(player, movement);
    ProcessKeyboard(player, movement);
    UpdateRecoverInput(player);
    ApplyTutorialInputRestriction(player);
    CaptureAttackInput();
}

void PlayerInput::ClearNonControlledPlayerInput()
{
    mMoveForward = 0.0f;
    mMoveLeft = 0.0f;
    mCameraYaw = 0.0f;
    mCameraStickX = 0.0f;
    mCameraStickY = 0.0f;

    mDodgePressed = false;
    mJumpPressed = false;
    mAttackPressed = false;
    mWideAttackPressed = false;
    mSpecialAttackPressed = false;
    mRecoverPressed = false;

    ClearAttackBuffer();
}

void PlayerInput::ApplyTutorialInputRestriction(Player& player)
{
    SceneSystem* sceneSystem =
        player.GetGame()->GetSceneSystem();
    if (!sceneSystem ||
        !sceneSystem->IsWaitingForTutorialPlayerJump()) {
        return;
    }

    mMoveForward = 0.0f;
    mMoveLeft = 0.0f;
    mCameraYaw = 0.0f;

    mDodgePressed = false;
    mAttackPressed = false;
    mWideAttackPressed = false;
    mSpecialAttackPressed = false;
    mRecoverPressed = false;

    ClearAttackBuffer();
}

void PlayerInput::ProcessGameController(Player& player, const PlayerMovement& movement)
{
    Game* game = player.GetGame();

    if (!game->GetIsPlayer2Joined() &&
        game->GetControlledPlayer() != &player) {
        return;
    }




    const int controllerPlayerNum =
        game->GetIsPlayer2Joined()
            ? movement.GetPlayerNum()
            : 1;
    if (!mInputSystem.HasControllerInput(controllerPlayerNum)) {
        return;
    }

    constexpr float deadZone = 0.25f;
    constexpr float scale = 1.0f / 32767.0f;

    mMoveForward = mInputSystem.GetControllerAxis(
        controllerPlayerNum,
        SDL_CONTROLLER_AXIS_LEFTY) * scale;
    mMoveLeft = mInputSystem.GetControllerAxis(
        controllerPlayerNum,
        SDL_CONTROLLER_AXIS_LEFTX) * scale;

    if (std::abs(mMoveForward) < deadZone) {
        mMoveForward = 0.0f;
    }

    if (std::abs(mMoveLeft) < deadZone) {
        mMoveLeft = 0.0f;
    }

    mJumpPressed = mInputSystem.IsControllerButtonPressed(
        controllerPlayerNum, SDL_CONTROLLER_BUTTON_A);
    mAttackPressed = mInputSystem.IsControllerButtonPressed(
        controllerPlayerNum, SDL_CONTROLLER_BUTTON_X);
    mWideAttackPressed = mInputSystem.IsControllerButtonPressed(
        controllerPlayerNum, SDL_CONTROLLER_BUTTON_Y);
    mDodgePressed = mInputSystem.IsControllerButtonPressed(
        controllerPlayerNum, SDL_CONTROLLER_BUTTON_B);
    mSpecialAttackPressed = mInputSystem.IsControllerButtonPressed(
        controllerPlayerNum, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
}

void PlayerInput::ProcessKeyboard(Player& player, const PlayerMovement& movement)
{
    Game* game = player.GetGame();

    if (game->GetIsPlayer2Joined()) {
        if (game->HasGameControllerForPlayer(movement.GetPlayerNum())) {
            return;
        }
    } else {
        if (game->IsGameControllerConnected() ||
            game->GetControlledPlayer() != &player) {
            return;
        }
    }

    mMoveForward = 0.0f;
    mMoveLeft = 0.0f;
    mJumpPressed = false;
    mAttackPressed = false;
    mWideAttackPressed = false;
    mDodgePressed = false;
    mSpecialAttackPressed = false;
    mRecoverPressed = false;

    if (game->IsEditorKeyboardInputCaptured()) {
        return;
    }

    if (mInputSystem.IsKeyPressed(GLFW_KEY_W)) {
        mMoveForward -= 1.0f;
    }
    if (mInputSystem.IsKeyPressed(GLFW_KEY_S)) {
        mMoveForward += 1.0f;
    }
    if (mInputSystem.IsKeyPressed(GLFW_KEY_A)) {
        mMoveLeft -= 1.0f;
    }
    if (mInputSystem.IsKeyPressed(GLFW_KEY_D)) {
        mMoveLeft += 1.0f;
    }

    glm::vec2 moveInput(mMoveLeft, mMoveForward);
    if (glm::length(moveInput) > 1.0f) {
        moveInput = glm::normalize(moveInput);
    }

    mMoveLeft = moveInput.x;
    mMoveForward = moveInput.y;

    mJumpPressed = mInputSystem.IsKeyPressed(GLFW_KEY_SPACE);
    mAttackPressed = mInputSystem.IsKeyPressed(GLFW_KEY_K);
    mWideAttackPressed = mInputSystem.IsKeyPressed(GLFW_KEY_J);
    mDodgePressed = mInputSystem.IsKeyPressed(GLFW_KEY_U);
    mSpecialAttackPressed = mInputSystem.IsKeyPressed(GLFW_KEY_N);
}

void PlayerInput::UpdateRecoverInput(const Player& player)
{
    const bool isCombinationRecoveryRequested =
        mSpecialAttackPressed && mJumpPressed;
    const bool isAssistRecoveryRequested =
        player.GetGame()->IsAssistControlStyle() &&
        mAttackPressed &&
        !mSpecialAttackPressed;
    mRecoverPressed =
        isCombinationRecoveryRequested || isAssistRecoveryRequested;
}

void PlayerInput::CaptureAttackInput()
{
    // L/Nとの同時押しはスペシャル攻撃用なので、通常攻撃として予約しない。
    if (mSpecialAttackPressed) {
        return;
    }

    const bool normalAttackStarted = mAttackPressed && !mAttackPressedPrev;
    const bool wideAttackStarted = mWideAttackPressed && !mWideAttackPressedPrev;

    if (normalAttackStarted && !mRecoverPressed) {
        mBufferedAttackInput = PlayerAttackInputKind::Normal;
        mAttackBufferRemaining = mAttackBufferDuration;
    } else if (wideAttackStarted) {
        mBufferedAttackInput = PlayerAttackInputKind::Wide;
        mAttackBufferRemaining = mAttackBufferDuration;
    }
}

void PlayerInput::EndFrame()
{
    mDodgePressedPrev = mDodgePressed;
    mJumpPressedPrev = mJumpPressed;
    mAttackPressedPrev = mAttackPressed;
    mWideAttackPressedPrev = mWideAttackPressed;
    mSpecialAttackPressedPrev = mSpecialAttackPressed;
    mRecoverPressedPrev = mRecoverPressed;
}

void PlayerInput::UpdateInputAvailableTimer(float deltaTime)
{
    if (mInputAvailableTimer >= 0.0f) {
        mInputAvailableTimer -= deltaTime;
    }
}

void PlayerInput::UpdateAttackBuffer(float deltaTime)
{
    if (mBufferedAttackInput == PlayerAttackInputKind::None) {
        return;
    }

    mAttackBufferRemaining -= deltaTime;
    if (mAttackBufferRemaining <= 0.0f) {
        ClearAttackBuffer();
    }
}

void PlayerInput::ConsumeBufferedAttackInput()
{
    ClearAttackBuffer();
}

void PlayerInput::ClearAttackBuffer()
{
    mBufferedAttackInput = PlayerAttackInputKind::None;
    mAttackBufferRemaining = 0.0f;
}

void PlayerInput::SyncAttackButtonPrev()
{
    mAttackPressedPrev = mAttackPressed;
    mWideAttackPressedPrev = mWideAttackPressed;
    mSpecialAttackPressedPrev = mSpecialAttackPressed;
}
