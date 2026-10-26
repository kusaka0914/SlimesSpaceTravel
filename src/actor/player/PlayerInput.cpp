#include "actor/player/PlayerInput.h"

#include "actor/player/PlayerMovement.h"
#include "system/InputSystem.h"
#include "system/PlayerInputDeviceRouting.h"

#include <GLFW/glfw3.h>
#include <SDL.h>
#include <algorithm>
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

    const PlayerInputDevice device = ResolvePlayerInputDevice(movement.GetPlayerNum(), context.devices);
    if ((mHasReadInputDevice && device != mPreviousInputDevice) ||
        (context.isEditorKeyboardCaptured &&
         (device == PlayerInputDevice::PrimaryKeyboard || device == PlayerInputDevice::SecondaryKeyboard))) {
        SuppressCombatInputUntilReleased();
    }
    mPreviousInputDevice = device;
    mHasReadInputDevice = true;

    // 機器切断・操作対象変更のときも、前フレームの押下を残さない。
    mAttackPressed = mWideAttackPressed = mRecoveryButtonPressed = false;
    mJumpPressed = mDodgePressed = false;
    mMoveForward = mMoveLeft = 0.0f;
    ProcessGameController(movement, context.devices);
    ProcessKeyboard(movement, context);
    ApplyTutorialInputRestriction(context.isWaitingForTutorialJump);
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
    mRecoveryButtonPressed = false;
    mRecoverPressed = false;

    SuppressCombatInputUntilReleased();
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
    mRecoveryButtonPressed = false;
    mRecoverPressed = false;

    SuppressCombatInputUntilReleased();
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
    mRecoveryButtonPressed =
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
    mRecoveryButtonPressed = false;
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
    const int recoveryKey = usesSecondaryKeyboard ? GLFW_KEY_RIGHT_ALT : GLFW_KEY_N;

    mJumpPressed = mInputSystem.IsKeyPressed(jumpKey);
    mAttackPressed = mInputSystem.IsKeyPressed(attackKey);
    mWideAttackPressed = mInputSystem.IsKeyPressed(wideAttackKey);
    mDodgePressed = mInputSystem.IsKeyPressed(dodgeKey);
    mRecoveryButtonPressed = mInputSystem.IsKeyPressed(recoveryKey);
}

void PlayerInput::UpdateCombatInput(float deltaTime, bool isAssistStyle, bool isPreparingChargedAttack)
{
    if (mInputAvailableTimer >= 0.0f) {
        SuppressCombatInputUntilReleased();
        return;
    }
    const auto requests = mCombatInput.Update(mWideAttackPressed, mAttackPressed, mRecoveryButtonPressed, isAssistStyle,
                                              isPreparingChargedAttack, deltaTime);
    mRecoverPressed = requests.recover;
    mChargedAttackFireRequested = requests.fireChargedAttack;
    mContinuousAttackRequested = mWideAttackPressed && (mContinuousAttackRequested || requests.continuousAttack);
    mChargeAttackRequested = mAttackPressed && (mChargeAttackRequested || requests.chargeAttack);

    if (requests.strongAttack || requests.weakAttack) {
        mBufferedAttackInput = requests.strongAttack ? PlayerAttackInputKind::Normal : PlayerAttackInputKind::Wide;
        mAttackBufferRemaining = mAttackBufferDuration;
    }
}

void PlayerInput::SuppressCombatInputUntilReleased()
{
    mCombatInput.SuppressUntilReleased();
    mAirDodgeButtonSuppressed = true;
    ConsumeAirDodgeInput();
    mContinuousAttackRequested = false;
    mChargeAttackRequested = false;
    mChargedAttackFireRequested = false;
    mRecoverPressed = false;
    ClearAttackBuffer();
}

void PlayerInput::EndFrame()
{
    mDodgePressedPrev = mDodgePressed;
    mJumpPressedPrev = mJumpPressed;
    mWideAttackPressedPrev = mWideAttackPressed;
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

void PlayerInput::CaptureAirDodgeInput(bool isAirborne, bool allowsBuffering)
{
    if (mAirDodgeButtonSuppressed) {
        if (!mJumpPressed) {
            mAirDodgeButtonSuppressed = false;
        }
        ConsumeAirDodgeInput();
        return;
    }
    if (!isAirborne || !allowsBuffering) {
        ConsumeAirDodgeInput();
        return;
    }
    if (mJumpPressed && !mJumpPressedPrev) {
        mAirDodgeBufferRemaining = 0.2f;
    }
}

void PlayerInput::UpdateAirDodgeBuffer(float deltaTime)
{
    mAirDodgeBufferRemaining = std::max(0.0f, mAirDodgeBufferRemaining - std::max(0.0f, deltaTime));
}
