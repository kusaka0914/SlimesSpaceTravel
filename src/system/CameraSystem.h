#pragma once

#include "system/camera/CameraCollisionResolver.h"
#include "system/camera/CameraShakeEffect.h"
#include "system/camera/CinematicCamera.h"
#include "system/camera/CinematicSequenceLibrary.h"
#include "system/camera/DebugCamera.h"
#include "system/camera/FocusCamera.h"
#include "system/camera/PlayerCamera.h"
#include "system/camera/PlayerCameraSettings.h"
#include "system/camera/zones/CameraZoneController.h"
#include "system/sequence/BossDefeatSequence.h"

#include <glm/glm.hpp>

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

class GameWorld;
class InputSystem;
class SceneSystem;
class AudioSystem;
class Actor;
class Player;
class Boat;
class Planet;
class Enemy;
class Star;

// 操作権・表示モードの現在値。Playerはその呼出時点の借用先で、ステージをまたいで保持しない。
struct CameraControlState {
    Player* mainPlayer = nullptr;
    Player* controlledPlayer = nullptr;
    int controlledPlayerIndex = 0;
    bool isPlayer2Joined = false;
    bool isFreeCameraMode = false;
    bool isUGCMode = false;
    bool isDebugEditorShowing = false;
    bool isEditorKeyboardInputCaptured = false;
    bool hasPrimaryController = false;
    bool hasSecondController = false;
};

// 必須参照はCameraSystemより長く有効に保つ。コールバックは同期実行し、再読込後の対象を利用時に取得する。
// readControlStateは必須。physicsProvider・findPlacedActorが未設定なら、物理補正・配置Actorへの注目を省略する。
struct CameraSystemDependencies {
    const GameWorld& world;
    const InputSystem& input;
    SceneSystem& scene;
    AudioSystem* audio = nullptr;
    std::function<PhysicsSystem*()> physicsProvider{};
    std::function<CameraControlState()> readControlState{};
    std::function<Actor*(const std::string&, int)> findPlacedActor{};
};

// プレイヤー追従・会話・演出のカメラを調停し、描画用のビューを生成する。
class CameraSystem {
public:
    using PlayerCameraState = ::PlayerCameraState;

    explicit CameraSystem(CameraSystemDependencies dependencies);

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

    // 操作対象の現在のビューと画角を取得する。ビュー未生成時はnullopt。フリーカメラにも対応する。
    std::optional<CameraPose> GetCurrentCameraPose();

    CameraPose GetDebugCameraPose() const { return mDebugCamera.GetPose(mReadControlState().isUGCMode); }
    void SetDebugCameraPose(const CameraPose& pose) { mDebugCamera.SetPose(pose, mReadControlState().isUGCMode); }

    float GetFieldOfViewDegrees() const;
    float GetPlayerFieldOfViewDegrees(int playerIndex) const;
    // 実際の描画先の幅/高さ。上下分割する前の画面全体の比率を渡す。
    void SetViewportAspectRatio(float aspect) { mViewportAspectRatio = aspect > 0.0f ? aspect : 1.0f; }
    // Editor専用。-1でプレビューを解除し、現在位置からゾーンを再評価する。
    void SetCameraZonePreview(int yamlIndex);
    int GetCameraZonePreview() const { return mCameraZonePreviewIndex; }

    void SetIsTargetFocus(bool isTargetFocus) { mIsTargetFocus = isTargetFocus; }

    bool GetIsTargetFocus() const { return mIsTargetFocus; }
    bool HasActiveRevealFocus() const;
    bool AllowsPlayerInput() const;

    // 分裂した側へ元の行き・帰りを引き継ぐ。接地判定の確定前でも呼べる。
    void InheritSplitPlayerCameraDirection(int fromPlayerIndex, int toPlayerIndex);

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

    const GameWorld& mWorld;
    const InputSystem& mInput;
    SceneSystem& mScene;
    std::function<CameraControlState()> mReadControlState;
    std::function<Actor*(const std::string&, int)> mFindPlacedActor;
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
    CameraZoneController mCameraZones;
    std::uint64_t mCameraFrameNumber = 0;
    float mCameraDeltaTime = 0.0f;
    float mViewportAspectRatio = 1.0f;
    int mCameraZonePreviewIndex = -1;
    PlayerCamera mPlayerCamera;
    PlayerCameraSettings mPlayerCameraSettings;
    PlayerCameraSettingsRepository mPlayerCameraSettingsRepository;

    CinematicSequenceLibrary mCinematicLibrary;
    CinematicCamera mCinematicCamera;
};
