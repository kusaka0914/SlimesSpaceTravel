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
};

class PlayerConfigurationController {
public:
    explicit PlayerConfigurationController(
        const PlayerConfigurationDependencies& dependencies);

    void Reset();
    void ConfigureAddedPlayer(Player& player);
    void SynchronizeAfterStageReload();
    void JoinSecondPlayer();
    void ReturnToSinglePlayer();
    bool CanStartTwoPlayerFromPauseMenu() const;

    bool ToggleSplit();
    bool CanToggleSplit() const;
    void SetSplitMergeButtonHeld(bool isHeld);
    bool TryResolveMergeGuide(
        const Player*& targetPlayer,
        float& radiusWorldUnits) const;
    bool TryConsumeSplitGuard(const Player& damagedPlayer);
    int GetSplitGuardCount() const;
    int GetMaximumSplitGuardCount() const;
    void UpdateSplitMergeTransition(float deltaTime);
    bool IsSplitMergeTransitionActive() const;
    bool SwitchControlledPlayer();
    bool CanSwitchControlledPlayer() const;
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
    bool TryResolveSplitDirection(
        Player& mainPlayer,
        glm::vec3& splitDirection) const;
    bool IsSplitDirectionClear(
        Player& mainPlayer,
        const glm::vec3& splitDirection) const;
    bool BeginSoloMergeTransition();
    void UpdatePendingSoloMergeRequest();
    void UpdateMergeRecall(float deltaTime);
    Player* FindMergeGuideTargetPlayer() const;
    void UpdateSoloSplitTransition(float progress);
    void UpdateSoloMergeTransition(float progress);
    void CompleteSoloSplitTransition();
    void CompleteSoloMergeTransition();
    bool AreSplitPlayersCloseEnoughToMerge() const;
    bool MergePlayerInto(
        int targetPlayerIndex,
        ControlledPlayerCameraTransition cameraTransition =
            ControlledPlayerCameraTransition::Snap);
    void SelectControlledPlayer(
        int playerIndex,
        ControlledPlayerCameraTransition cameraTransition =
            ControlledPlayerCameraTransition::Snap);

    GameWorld& mWorld;
    StagePlayerLoader& mPlayerLoader;
    SceneSystem& mSceneSystem;
    CameraSystem& mCameraSystem;
    SequenceSystem& mSequenceSystem;
    GameProgressController& mProgressController;
    GamepadRumbleService& mGamepadService;
    PauseMenuController& mPauseMenuController;
    PhysicsSystem& mPhysicsSystem;
    bool mIsSecondPlayerJoined = false;
    PlayerControlConfigurationState mControlState;
    SplitMergeTransitionState mSplitMergeTransition;
    bool mIsSplitMergeButtonHeld = false;
    bool mIsMergeGuideRequested = false;
    PlayerSplitGuardState mSplitGuardState;
};
