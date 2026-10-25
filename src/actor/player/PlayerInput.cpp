#include "actor/player/PlayerInput.h"

#include "actor/player/PlayerMovement.h"
#include "system/InputSystem.h"
#include "system/PlayerInputDeviceRouting.h"

#include <GLFW/glfw3.h>
#include <SDL.h>
#include <cmath>
#include <glm/glm.hpp>

PlayerInput::PlayerInput(InputSystem& inputSystem)
    : mInputSystem(inputSystem)
{
}

void PlayerInput::ProcessActor(const PlayerMovement& movement, const PlayerInputContext& context)
{
    if (context.isNonControlledSoloPlayer) {
        ClearNonControlledPlayerInput();
        return;
    }

    if (mInputAvailableTimer >= 0.0f) {
        if (context.isWaitingForTutorialJump) {
            mJumpPressed = false;
            ApplyTutorialInputRestriction(context.isWaitingForTutorialJump);
        }
        return;
    }

    ProcessGameController(movement, context.devices);
    ProcessKeyboard(movement, context);
    UpdateRecoverInput(context.isAssistControlStyle);
    ApplyTutorialInputRestriction(context.isWaitingForTutorialJump);
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

void PlayerInput::ApplyTutorialInputRestriction(bool isWaitingForTutorialJump)
{
    if (!isWaitingForTutorialJump) {
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

void PlayerInput::ProcessGameController(const PlayerMovement& movement, const PlayerInputDeviceContext& devices)
{
    const PlayerInputDevice inputDevice = ResolvePlayerInputDevice(movement.GetPlayerNum(), devices);
    const int controllerPlayerNum = ResolveControllerPlayerNum(inputDevice);
    if (controllerPlayerNum == 0) {
        return;
    }

    constexpr float deadZone = 0.25f;
    constexpr float scale = 1.0f / 32767.0f;

    mMoveForward = mInputSystem.GetControllerAxis(controllerPlayerNum, SDL_CONTROLLER_AXIS_LEFTY) * scale;
    mMoveLeft = mInputSystem.GetControllerAxis(controllerPlayerNum, SDL_CONTROLLER_AXIS_LEFTX) * scale;

    if (std::abs(mMoveForward) < deadZone) {
        mMoveForward = 0.0f;
    }

    if (std::abs(mMoveLeft) < deadZone) {
        mMoveLeft = 0.0f;
    }

    mJumpPressed = mInputSystem.IsControllerButtonPressed(controllerPlayerNum, SDL_CONTROLLER_BUTTON_A);
    mAttackPressed = mInputSystem.IsControllerButtonPressed(controllerPlayerNum, SDL_CONTROLLER_BUTTON_X);
    mWideAttackPressed = mInputSystem.IsControllerButtonPressed(controllerPlayerNum, SDL_CONTROLLER_BUTTON_Y);
    mDodgePressed = mInputSystem.IsControllerButtonPressed(controllerPlayerNum, SDL_CONTROLLER_BUTTON_B);
    mSpecialAttackPressed =
        mInputSystem.IsControllerButtonPressed(controllerPlayerNum, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
}

void PlayerInput::ProcessKeyboard(const PlayerMovement& movement, const PlayerInputContext& context)
{
    const PlayerInputDevice inputDevice = ResolvePlayerInputDevice(movement.GetPlayerNum(), context.devices);
    const bool usesPrimaryKeyboard = inputDevice == PlayerInputDevice::PrimaryKeyboard;
    const bool usesSecondaryKeyboard = inputDevice == PlayerInputDevice::SecondaryKeyboard;
    if (!usesPrimaryKeyboard && !usesSecondaryKeyboard) {
        return;
    }

    mMoveForward = 0.0f;
    mMoveLeft = 0.0f;
    mJumpPressed = false;
    mAttackPressed = false;
    mWideAttackPressed = false;
    mDodgePressed = false;
    mSpecialAttackPressed = false;
    mRecoverPressed = false;

    if (context.isEditorKeyboardCaptured) {
        return;
    }

    const int forwardKey = usesSecondaryKeyboard ? GLFW_KEY_UP : GLFW_KEY_W;
    const int backwardKey = usesSecondaryKeyboard ? GLFW_KEY_DOWN : GLFW_KEY_S;
    const int leftKey = usesSecondaryKeyboard ? GLFW_KEY_LEFT : GLFW_KEY_A;
    const int rightKey = usesSecondaryKeyboard ? GLFW_KEY_RIGHT : GLFW_KEY_D;

    if (mInputSystem.IsKeyPressed(forwardKey)) {
        mMoveForward -= 1.0f;
    }
    if (mInputSystem.IsKeyPressed(backwardKey)) {
        mMoveForward += 1.0f;
    }
    if (mInputSystem.IsKeyPressed(leftKey)) {
        mMoveLeft -= 1.0f;
    }
    if (mInputSystem.IsKeyPressed(rightKey)) {
        mMoveLeft += 1.0f;
    }

    glm::vec2 moveInput(mMoveLeft, mMoveForward);
    if (glm::length(moveInput) > 1.0f) {
        moveInput = glm::normalize(moveInput);
    }

    mMoveLeft = moveInput.x;
    mMoveForward = moveInput.y;

    const int jumpKey = usesSecondaryKeyboard ? GLFW_KEY_RIGHT_SHIFT : GLFW_KEY_SPACE;
    const int attackKey = usesSecondaryKeyboard ? GLFW_KEY_SLASH : GLFW_KEY_K;
    const int wideAttackKey = usesSecondaryKeyboard ? GLFW_KEY_PERIOD : GLFW_KEY_J;
    const int dodgeKey = usesSecondaryKeyboard ? GLFW_KEY_RIGHT_CONTROL : GLFW_KEY_U;
    const int specialAttackKey = usesSecondaryKeyboard ? GLFW_KEY_RIGHT_ALT : GLFW_KEY_N;

    mJumpPressed = mInputSystem.IsKeyPressed(jumpKey);
    mAttackPressed = mInputSystem.IsKeyPressed(attackKey);
    mWideAttackPressed = mInputSystem.IsKeyPressed(wideAttackKey);
    mDodgePressed = mInputSystem.IsKeyPressed(dodgeKey);
    mSpecialAttackPressed = mInputSystem.IsKeyPressed(specialAttackKey);
}

void PlayerInput::UpdateRecoverInput(bool isAssistControlStyle)
{
    const bool isCombinationRecoveryRequested = mSpecialAttackPressed && mJumpPressed;
    const bool isAssistRecoveryRequested = isAssistControlStyle && mAttackPressed && !mSpecialAttackPressed;
    mRecoverPressed = isCombinationRecoveryRequested || isAssistRecoveryRequested;
}

void PlayerInput::CaptureAttackInput()
{
    // 特殊攻撃用の同時押しを、通常攻撃の先行入力として二重に予約しない。
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
