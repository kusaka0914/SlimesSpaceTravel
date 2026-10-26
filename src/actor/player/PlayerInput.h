#pragma once

#include "actor/player/PlayerCombatInput.h"
#include "actor/player/PlayerTypes.h"
#include "system/PlayerInputDeviceRouting.h"

class PlayerMovement;
class InputSystem;

// 入力取得時点のゲーム状態。ActorやGameへの参照を保持しない。
struct PlayerInputContext {
    PlayerInputDeviceContext devices{};
    bool isNonControlledSoloPlayer = false;
    bool isEditorKeyboardCaptured = false;
    bool isWaitingForTutorialJump = false;
};

// 操作デバイスの入力を集約し、押下履歴・攻撃の先行入力・チュートリアル中の入力制限を管理する。
class PlayerInput {
public:
    explicit PlayerInput(InputSystem& inputSystem);

    // デバイス入力・操作対象・チュートリアル制限を反映し、攻撃の押下開始を先行入力へ保存する。
    void ProcessActor(const PlayerMovement& movement, const PlayerInputContext& context);
    void ProcessGameController(const PlayerMovement& movement, const PlayerInputDeviceContext& devices);
    void ProcessKeyboard(const PlayerMovement& movement, const PlayerInputContext& context);

    // 状態更新で押下開始を判定した後に呼び、現在のボタン状態を前フレーム値へ保存する。
    void EndFrame();

    // 状態更新の直前に1回呼ぶ。長押し要求はボタン解放または消費まで保持する。
    void UpdateCombatInput(float deltaTime, bool isAssistStyle, bool isPreparingChargedAttack);
    bool HasContinuousAttackRequest() const { return mContinuousAttackRequested; }
    bool HasChargeAttackRequest() const { return mChargeAttackRequested; }
    bool HasChargedAttackFireRequest() const { return mChargedAttackFireRequested; }
    void ConsumeContinuousAttackRequest() { mContinuousAttackRequested = false; }
    void ConsumeChargeAttackRequest() { mChargeAttackRequested = false; }
    void SuppressCombatInputUntilReleased();
    void UpdateInputAvailableTimer(float deltaTime);
    void UpdateAttackBuffer(float deltaTime);

    bool GetDodgePressed() const { return mDodgePressed; }
    bool GetDodgePressedPrev() const { return mDodgePressedPrev; }
    bool GetJumpPressed() const { return mJumpPressed; }
    bool GetJumpPressedPrev() const { return mJumpPressedPrev; }
    bool GetWideAttackPressed() const { return mWideAttackPressed; }
    bool GetWideAttackPressedPrev() const { return mWideAttackPressedPrev; }
    bool GetRecoverPressed() const { return mRecoverPressed; }

    // このフレームの手動カメラ回転量（ラジアン）。絶対角ではない。
    float GetCameraYaw() const { return mCameraYaw; }
    float GetMoveForward() const { return mMoveForward; }
    float GetMoveLeft() const { return mMoveLeft; }
    float GetInputAvailableTimer() const { return mInputAvailableTimer; }

    PlayerAttackInputKind GetBufferedAttackInput() const { return mBufferedAttackInput; }
    bool HasBufferedAttackInput() const { return mBufferedAttackInput != PlayerAttackInputKind::None; }
    void ConsumeBufferedAttackInput();
    void ClearAttackBuffer();

    // 押下履歴を押下中にし、ボタンを離すまで新しい押下として扱わない。
    void SuppressJumpUntilReleased() { mJumpPressedPrev = true; }
    void SuppressDodgeUntilReleased() { mDodgePressedPrev = true; }

    void SetCameraYaw(float cameraYaw) { mCameraYaw = cameraYaw; }

    // 非負の間は通常の入力読取を停止する。時間は秒で、負値なら読取を許可する。
    void SetInputAvailableTimer(float inputAvailableTimer) { mInputAvailableTimer = inputAvailableTimer; }


private:
    void ApplyTutorialInputRestriction(bool isWaitingForTutorialJump);
    void ClearNonControlledPlayerInput();

private:
    InputSystem& mInputSystem;
    PlayerCombatInput mCombatInput;
    PlayerInputDevice mPreviousInputDevice = PlayerInputDevice::None;
    bool mHasReadInputDevice = false;
    bool mContinuousAttackRequested = false;
    bool mChargeAttackRequested = false;
    bool mChargedAttackFireRequested = false;

    bool mDodgePressed = false;
    bool mDodgePressedPrev = false;
    bool mJumpPressed = false;
    bool mJumpPressedPrev = false;
    bool mAttackPressed = false;
    bool mWideAttackPressed = false;
    bool mWideAttackPressedPrev = false;
    bool mRecoveryButtonPressed = false;
    bool mRecoverPressed = false;

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
