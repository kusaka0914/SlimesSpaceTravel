#pragma once

#include "actor/player/PlayerTypes.h"

class Player;
class PlayerMovement;

class PlayerInput {
public:
    void ProcessActor(Player& player, const PlayerMovement& movement);
    void ProcessGameController(Player& player, const PlayerMovement& movement);
    void ProcessKeyboard(Player& player, const PlayerMovement& movement);

    void EndFrame();
    void UpdateInputAvailableTimer(float deltaTime);
    void UpdateAttackBuffer(float deltaTime);

    bool GetDodgePressed() const { return mDodgePressed; }
    bool GetDodgePressedPrev() const { return mDodgePressedPrev; }
    bool GetJumpPressed() const { return mJumpPressed; }
    bool GetJumpPressedPrev() const { return mJumpPressedPrev; }
    bool GetAttackPressed() const { return mAttackPressed; }
    bool GetAttackPressedPrev() const { return mAttackPressedPrev; }
    bool GetWideAttackPressed() const { return mWideAttackPressed; }
    bool GetWideAttackPressedPrev() const { return mWideAttackPressedPrev; }
    bool GetSpecialAttackPressed() const { return mSpecialAttackPressed; }
    bool GetSpecialAttackPressedPrev() const { return mSpecialAttackPressedPrev; }
    bool GetRecoverPressed() const { return mRecoverPressed; }
    bool GetRecoverPressedPrev() const { return mRecoverPressedPrev; }

    float GetCameraYaw() const { return mCameraYaw; }
    float GetMoveForward() const { return mMoveForward; }
    float GetMoveLeft() const { return mMoveLeft; }
    float GetInputAvailableTimer() const { return mInputAvailableTimer; }

    PlayerAttackInputKind GetBufferedAttackInput() const { return mBufferedAttackInput; }
    bool HasBufferedAttackInput() const { return mBufferedAttackInput != PlayerAttackInputKind::None; }
    void ConsumeBufferedAttackInput();
    void ClearAttackBuffer();
    void SuppressJumpUntilReleased() { mJumpPressedPrev = true; }

    void SetCameraYaw(float cameraYaw) { mCameraYaw = cameraYaw; }
    void SetInputAvailableTimer(float inputAvailableTimer) { mInputAvailableTimer = inputAvailableTimer; }

    void SyncAttackButtonPrev();

private:
    void ApplyTutorialInputRestriction(Player& player);
    void CaptureAttackInput();
    void ClearNonControlledPlayerInput();

private:
    bool mDodgePressed = false;
    bool mDodgePressedPrev = false;
    bool mJumpPressed = false;
    bool mJumpPressedPrev = false;
    bool mAttackPressed = false;
    bool mAttackPressedPrev = false;
    bool mWideAttackPressed = false;
    bool mWideAttackPressedPrev = false;
    bool mSpecialAttackPressed = false;
    bool mSpecialAttackPressedPrev = false;
    bool mRecoverPressed = false;
    bool mRecoverPressedPrev = false;

    float mCameraYaw = 0.0f;
    float mMoveForward = 0.0f;
    float mMoveLeft = 0.0f;
    float mCameraStickX = 0.0f;
    float mCameraStickY = 0.0f;
    float mInputAvailableTimer = -1.0f;

    PlayerAttackInputKind mBufferedAttackInput = PlayerAttackInputKind::None;
    float mAttackBufferRemaining = 0.0f;
    float mAttackBufferDuration = 0.5f;
};
