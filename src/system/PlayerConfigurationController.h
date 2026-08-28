#pragma once

#include "system/PlayerControlConfigurationState.h"

class CameraSystem;
class GameProgressController;
class GameWorld;
class GamepadRumbleService;
class PauseMenuController;
class Player;
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
    bool CanChangeSoloConfiguration() const;
    bool SplitPlayer();
    bool MergePlayer();
    bool AreSplitPlayersCloseEnoughToMerge() const;
    bool MergePlayerInto(int targetPlayerIndex);
    void SelectControlledPlayer(int playerIndex);

    GameWorld& mWorld;
    StagePlayerLoader& mPlayerLoader;
    SceneSystem& mSceneSystem;
    CameraSystem& mCameraSystem;
    SequenceSystem& mSequenceSystem;
    GameProgressController& mProgressController;
    GamepadRumbleService& mGamepadService;
    PauseMenuController& mPauseMenuController;
    bool mIsSecondPlayerJoined = false;
    PlayerControlConfigurationState mControlState;
};
