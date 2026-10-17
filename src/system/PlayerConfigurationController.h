#pragma once

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

    // 操作対象・分裂遷移・ガードを初期化する。2人参加の状態は保持する。
    void Reset();

    // 追加Playerの有効・操作ロック・分裂形態を、現在の参加状態に合わせる。
    void ConfigureAddedPlayer(Player& player);

    // 2人参加時に、再読込された2人目の配置と接地状態を1人目へ合わせる。
    void SynchronizeAfterStageReload();
    void JoinSecondPlayer();
    void ReturnToSinglePlayer();
    bool CanStartTwoPlayerFromPauseMenu() const;

    // 実行条件を満たす場合に分裂・合体を要求する。受理時はtrue。演出の完了を表す値ではない。
    bool ToggleSplit();
    bool CanToggleSplit() const;
    void SetSplitMergeButtonHeld(bool isHeld);

    // 合体ガイドの対象への借用参照とワールド単位の半径を返す。表示条件を満たさなければfalse。
    bool TryResolveMergeGuide(const Player*& targetPlayer, float& radiusWorldUnits) const;

    // 対象Playerがガード可能な分裂状態なら1回分を消費する。適用できなければfalse。
    bool TryConsumeSplitGuard(const Player& damagedPlayer);
    int GetSplitGuardCount() const;
    int GetMaximumSplitGuardCount() const;

    // 秒単位で分裂合体の遷移とガード回復を更新する。
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

    bool CanStartSplitMergeInput() const;
    bool CanChangeSoloConfiguration() const;
    bool SplitPlayer();
    bool BeginSoloSplitTransition();
    bool TryResolveSplitDirection(Player& mainPlayer, glm::vec3& splitDirection) const;
    bool IsSplitDirectionClear(Player& mainPlayer, const glm::vec3& splitDirection) const;
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
    bool mIsSecondPlayerJoined = false;
    PlayerControlConfigurationState mControlState;
    SplitMergeTransitionState mSplitMergeTransition;
    bool mIsSplitMergeButtonHeld = false;
    bool mIsMergeGuideRequested = false;
    PlayerSplitGuardState mSplitGuardState;
};
