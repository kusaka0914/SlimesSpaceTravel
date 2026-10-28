#pragma once

#include "actor/player/PlayerSoulPresentation.h"
#include "system/PlayerControlConfigurationState.h"
#include "system/PlayerSplitGuardState.h"

#include <glm/glm.hpp>

class CameraSystem;
class GameProgressController;
class GameWorld;
class GamepadRumbleService;
class PauseMenuController;
class Player;
class PhysicsSystem;
class SceneSystem;
class SequenceSystem;
class StagePlayerLoader;

struct PlayerConfigurationDependencies {
    GameWorld& world;
    StagePlayerLoader& playerLoader;
    SceneSystem& sceneSystem;
    CameraSystem& cameraSystem;
    SequenceSystem& sequenceSystem;
    GameProgressController& progressController;
    GamepadRumbleService& gamepadService;
    PauseMenuController& pauseMenuController;
    PhysicsSystem& physicsSystem;
    bool allowsKeyboardOnlyTwoPlayer = false;
};

// Playerの参加・分裂合体・操作対象切替と、それに伴うカメラや共有状態を調整する。
class PlayerConfigurationController {
public:
    // 依存先は借用する。各オブジェクトはこのコントローラーより長く生存すること。
    explicit PlayerConfigurationController(const PlayerConfigurationDependencies& dependencies);

    // 操作対象・分裂遷移・魂演出・ガードと切替予約を初期化する。2人参加の状態は保持する。
    void Reset();

    // 追加Playerの有効・操作ロック・分裂形態を、現在の参加状態に合わせる。
    void ConfigureAddedPlayer(Player& player);

    // 2人参加時に、再読込された2人目の配置と接地状態を1人目へ合わせる。
    void SynchronizeAfterStageReload();
    void JoinSecondPlayer();
    void ReturnToSinglePlayer();
    bool CanStartTwoPlayerFromPauseMenu() const;

    enum class SplitToggleResult { Accepted, Unavailable, BlockedByObstacle, MergeTooFar };

    // 分裂・合体を要求し、入力制限・障害物・合体距離を区別する。MergeTooFarでは既存の呼び寄せを開始する。
    SplitToggleResult ToggleSplit();
    bool CanToggleSplit() const;
    void SetSplitMergeButtonHeld(bool isHeld);

    // 合体ガイドの対象への借用参照とワールド単位の半径を返す。表示条件を満たさなければfalse。
    bool TryResolveMergeGuide(const Player*& targetPlayer, float& radiusWorldUnits) const;

    // 非操作側への攻撃を1回防ぐ。最後の1回では、Actor更新後に被弾側へ操作を移す。適用不可ならfalse。
    bool TryConsumeSplitGuard(const Player& damagedPlayer);
    int GetSplitGuardCount() const;
    int GetMaximumSplitGuardCount() const;

    // 1人プレイの分裂中だけ魂の表示情報を返す。Playerへの参照は描画中のみ有効。
    PlayerSoulPresentation CreateSoulPresentation() const;

    // Actor更新後に呼ぶ。秒単位で分裂合体・魂の演出・ガード回復と、ガード切れによる切替を更新する。
    void UpdateSplitMergeTransition(float deltaTime);
    bool IsSplitMergeTransitionActive() const;
    bool SwitchControlledPlayer();
    bool CanSwitchControlledPlayer() const;

    // 乗船後の操作対象切替を予約する。即座には切り替えない。
    void RequestControlSwitchAfterBoarding();
    void UpdatePendingControlSwitch(float deltaTime);
    void MergeSoloSplitAfterBoatArrival();
    void MergeSoloSplitBeforeRestart();
    void SynchronizeSoloSplitResources(const Player& sourcePlayer);

    bool IsSecondPlayerJoined() const;
    bool IsPlayerSplit() const;
    int GetControlledPlayerIndex() const;
    Player* GetMainPlayer() const;
    Player* GetControlledPlayer() const;

private:
    enum class SplitMergeTransitionKind {
        None,
        Splitting,
        Merging,
    };

    enum class ControlledPlayerCameraTransition {
        Snap,
        Smooth,
    };

    struct SplitMergeTransitionState {
        SplitMergeTransitionKind kind = SplitMergeTransitionKind::None;
        float elapsedSeconds = 0.0f;
        glm::vec3 splitPlayerStartPosition{0.0f};
        glm::vec3 splitDirection{1.0f, 0.0f, 0.0f};
        glm::vec3 splitPlayerStartScale{1.0f};
        glm::vec3 mainPlayerStartScale{1.0f};
    };

    struct SoulTransferState {
        float transferSeconds = 1.0f;
        glm::vec3 startPosition{0.0f};
        glm::vec3 upDirection{0.0f, 1.0f, 0.0f};
    };

public:
    // ステージ構築中に変更されるPlayer構成を退避する。Actorそのものや依存先は含まない。
    class StageReloadState {
        friend class PlayerConfigurationController;
        bool mIsSecondPlayerJoined = false;
        PlayerControlConfigurationState mControlState;
        SplitMergeTransitionState mSplitMergeTransition;
        bool mIsSplitMergeButtonHeld = false;
        bool mIsMergeGuideRequested = false;
        PlayerSplitGuardState mSplitGuardState;
        SoulTransferState mSoulTransfer;
        int mPendingGuardBreakPlayerIndex = -1;
    };

    StageReloadState CaptureStageReloadState() const { return mRuntimeState; }

    // 構築失敗で旧GameWorldを戻した後に呼び、参加・操作対象・分裂遷移・魂演出・ガードと切替予約を復元する。
    void RestoreStageReloadState(const StageReloadState& state) { mRuntimeState = state; }

private:
    SplitToggleResult CheckSplitMergeInput() const;
    bool CanChangeSoloConfiguration() const;
    void UpdateGuardBreakControlSwitch();
    void BeginSoulTransfer(int previousPlayerIndex, int currentPlayerIndex);
    bool SplitPlayer();
    SplitToggleResult BeginSoloSplitTransition();
    bool TryResolveSplitDirection(Player& mainPlayer, glm::vec3& splitDirection, bool& wasBlockedByObstacle) const;
    bool IsSplitDirectionClear(Player& mainPlayer, const glm::vec3& splitDirection, bool& wasBlockedByObstacle) const;
    bool BeginSoloMergeTransition();
    void UpdatePendingSoloMergeRequest();
    void UpdateMergeRecall(float deltaTime);
    Player* FindMergeGuideTargetPlayer() const;
    void UpdateSoloSplitTransition(float progress);
    void UpdateSoloMergeTransition(float progress);
    void CompleteSoloSplitTransition();
    void CompleteSoloMergeTransition();
    bool AreSplitPlayersCloseEnoughToMerge() const;
    bool MergePlayerInto(int targetPlayerIndex,
                         ControlledPlayerCameraTransition cameraTransition = ControlledPlayerCameraTransition::Snap);
    void
    SelectControlledPlayer(int playerIndex,
                           ControlledPlayerCameraTransition cameraTransition = ControlledPlayerCameraTransition::Snap);

    GameWorld& mWorld;
    StagePlayerLoader& mPlayerLoader;
    SceneSystem& mSceneSystem;
    CameraSystem& mCameraSystem;
    SequenceSystem& mSequenceSystem;
    GameProgressController& mProgressController;
    GamepadRumbleService& mGamepadService;
    PauseMenuController& mPauseMenuController;
    PhysicsSystem& mPhysicsSystem;
    bool mAllowsKeyboardOnlyTwoPlayer = false;
    StageReloadState mRuntimeState;
};
