#pragma once

#include "system/camera/CameraCollisionResolver.h"
#include "system/camera/CameraShakeEffect.h"
#include "system/camera/CinematicCamera.h"
#include "system/camera/CinematicSequenceLibrary.h"
#include "system/camera/DebugCamera.h"
#include "system/camera/FocusCamera.h"
#include "system/camera/PlayerCamera.h"
#include "system/camera/PlayerCameraSettings.h"
#include "system/sequence/BossDefeatSequence.h"

#include <glm/glm.hpp>

#include <string_view>
#include <vector>

class Game;
class Actor;
class Player;
class Boat;
class Planet;
class Enemy;
class Star;

// プレイヤー追従・会話・演出のカメラを調停し、描画用のビューを生成する。
class CameraSystem {
public:
    using PlayerCameraState = ::PlayerCameraState;

    explicit CameraSystem(Game* game);

    void ProcessInput();

    // カメラ振動を含む各カメラの状態を、秒単位のdeltaTimeで更新する。
    void Update(float deltaTime);
    void UpdateShakeEffects(float deltaTime);

    // 1始まりのPlayer番号に対して着地攻撃の命中振動を開始する。
    void StartAirStrongAttackHitShake(int playerNum);

    // 1始まりのPlayer番号に対して被ダメージ振動を開始する。
    void StartPlayerDamagedShake(int playerNum);

    // 指定シーケンスを再生する。未登録・再生不可ならfalse。shouldHoldFinalPoseで終了姿勢を保持する。
    bool PlayCinematic(std::string_view sequenceId, bool shouldHoldFinalPose = false);
    void StopCinematic();

    bool IsCinematicPlaying() const { return mCinematicCamera.IsActive(); }
    bool HasCinematicFinished() const { return mCinematicCamera.HasFinished(); }

    float GetCinematicElapsedTime() const { return mCinematicCamera.GetElapsedTime(); }
    float GetCinematicDuration() const { return mCinematicCamera.GetDuration(); }

    bool SaveCinematicSequences() const { return mCinematicLibrary.Save(); }

    // 再生中の演出を停止して定義を再読込する。読込結果を返す。
    bool ReloadCinematicSequences();

    const PlayerCameraSettings& GetPlayerCameraSettings() const { return mPlayerCameraSettings; }
    void SetPlayerCameraSettings(PlayerCameraSettings settings);
    bool SavePlayerCameraSettings() const;
    bool ReloadPlayerCameraSettings();

    // ステージ切替前に演出・追従状態と借用中のActor参照をクリアする。
    void ResetForStageChange();
    void SnapBehindControlledPlayer();

    bool GetTalkCameraPreviewEnabled() const { return mIsTalkCameraPreviewEnabled; }
    void SetTalkCameraPreviewEnabled(bool enabled) { mIsTalkCameraPreviewEnabled = enabled; }
    bool GetBoatRideCameraPreviewEnabled() const { return mIsBoatRideCameraPreviewEnabled; }
    void SetBoatRideCameraPreviewEnabled(bool enabled) { mIsBoatRideCameraPreviewEnabled = enabled; }

    CinematicSequenceLibrary& GetCinematicLibrary() { return mCinematicLibrary; }
    const CinematicSequenceLibrary& GetCinematicLibrary() const { return mCinematicLibrary; }

    CameraPose GetDebugCameraPose() const { return mDebugCamera.GetPose(); }
    void SetDebugCameraPose(const CameraPose& pose) { mDebugCamera.SetPose(pose); }

    float GetFieldOfViewDegrees() const;

    void SetIsTargetFocus(bool isTargetFocus) { mIsTargetFocus = isTargetFocus; }

    bool GetIsTargetFocus() const { return mIsTargetFocus; }
    bool HasActiveRevealFocus() const;
    bool AllowsPlayerInput() const;

    // 0始まりの操作対象切替に合わせてカメラを即座に合わせる。
    void SnapToControlledPlayer(int fromPlayerIndex, int toPlayerIndex);

    // 0始まりの操作対象切替に合わせてカメラを滑らかに遷移させる。
    void TransitionToControlledPlayer(int fromPlayerIndex, int toPlayerIndex);
    void StartBossDefeatSequence(Enemy* boss, Star* star);
    bool PreviewBossDefeatSequence();
    void StopBossDefeatSequence();
    bool IsBossDefeatSequencePlaying() const { return mBossDefeatSequence.IsActive(); }

    // 演出を優先して描画ビューを生成し、追従・注目状態も更新する。通常の2人プレイではPlayer順に返す。
    std::vector<glm::mat4> GetViews();
    glm::vec3 GetCameraPos() const;

    // 0始まりのPlayerインデックスに対応する追従カメラ位置を返す。
    glm::vec3 GetPlayerCameraPos(int playerIndex) const;

private:
    struct AirSlamCameraState {
        float distanceBlend = 0.0f;
        float pitchBlend = 0.0f;
        float returnDelayRemainingSeconds = 0.0f;
        bool isWaitingForLanding = false;
    };

    void UpdateCamera(float deltaTime);
    void BeginBossDefeatSequence(Enemy* boss, Star* star, bool isPreview);
    void UpdateTalkCameraTransition(float deltaTime);
    void UpdateTalkCameraAim();
    void UpdateTalkPageFocus(float deltaTime);
    void UpdatePlayerPitchOffsets(float deltaTime);
    void UpdateAirSlamCameraStates(float deltaTime);
    int GetPrimaryPlayerIndex() const;
    float GetEasedTalkCameraBlend() const;
    float GetEasedTalkPageFocusBlend() const;
    void CopyPlayerPitchOffset(int fromPlayerIndex, int toPlayerIndex);
    glm::mat4 GetPlayerCameraView(Player* player, int playerIndex);
    glm::mat4 ApplyPlayerShake(const glm::mat4& view, int playerIndex) const;
    glm::mat4 GetTalkPageFocusView(Player* player, int playerIndex);
    Actor* ResolveTalkPageFocusActor() const;
    std::vector<Actor*> FindFocusingActors() const;
    Boat* FindMovingBoat() const;
    Boat* ResolveBoatRideCameraTarget() const;
    Enemy* FindBossEnemy(Planet* planet) const;

    Game* mGame;
    BossDefeatSequence mBossDefeatSequence;

    bool mIsTargetFocus = false;
    bool mIsShowingRevealFocus = false;
    std::vector<Actor*> mRevealFocusActors;
    glm::mat4 mRevealFocusView{1.0f};
    bool mIsTalkCameraPreviewEnabled = false;
    bool mIsBoatRideCameraPreviewEnabled = false;
    bool mWasAlignCameraPressed = false;
    bool mWasSecondControllerAlignCameraPressed = false;
    float mCameraStickX = 0.0f;
    float mCameraStickY = 0.0f;
    float mSecondControllerStickX = 0.0f;
    float mSecondControllerStickY = 0.0f;
    float mKeyboardYawInput = 0.0f;
    float mKeyboardPitchInput = 0.0f;
    float mTalkCameraBlend = 0.0f;
    std::vector<float> mPlayerPitchOffsetsDegrees;
    std::vector<AirSlamCameraState> mAirSlamCameraStates;
    std::vector<CameraShakeEffect> mPlayerShakeEffects;
    Player* mTalkCameraPlayer = nullptr;
    bool mHasTalkCameraTarget = false;
    glm::vec3 mTalkCameraTargetPos{0.0f};
    Actor* mTalkPageFocusActor = nullptr;
    float mTalkPageFocusBlend = 0.0f;
    bool mHasTalkPageFocusPose = false;
    glm::vec3 mTalkPageFocusCameraPos{0.0f};
    glm::vec3 mTalkPageFocusTargetPos{0.0f};
    glm::vec3 mTalkPageFocusUpVec{0.0f, 1.0f, 0.0f};
    glm::vec3 mRenderedTalkPageCameraPos{0.0f};
    Boat* mBoatRideCameraTarget = nullptr;

    CameraCollisionResolver mCollisionResolver;
    DebugCamera mDebugCamera;
    FocusCamera mFocusCamera;
    PlayerCamera mPlayerCamera;
    PlayerCameraSettings mPlayerCameraSettings;
    PlayerCameraSettingsRepository mPlayerCameraSettingsRepository;

    CinematicSequenceLibrary mCinematicLibrary;
    CinematicCamera mCinematicCamera;
};
