#include <GL/glew.h>

#include "system/PlayerConfigurationController.h"

#include "actor/Player.h"
#include "system/CameraSystem.h"
#include "system/GameWorld.h"
#include "system/GameProgressController.h"
#include "system/GamepadRumbleService.h"
#include "system/PauseMenuController.h"
#include "system/PlayerSplitService.h"
#include "system/SceneSystem.h"
#include "system/sequence/SequenceSystem.h"
#include "system/actor_loader/StagePlayerLoader.h"

#include <cstddef>
#include <optional>
#include <vector>

namespace {
constexpr float playerMergeMaximumDistanceWorldUnits = 1.25f;
constexpr float boardingControlSwitchDelaySeconds = 0.5f;
}

PlayerConfigurationController::PlayerConfigurationController(
    const PlayerConfigurationDependencies& dependencies)
    : mWorld(dependencies.world),
      mPlayerLoader(dependencies.playerLoader),
      mSceneSystem(dependencies.sceneSystem),
      mCameraSystem(dependencies.cameraSystem),
      mSequenceSystem(dependencies.sequenceSystem),
      mProgressController(dependencies.progressController),
      mGamepadService(dependencies.gamepadService),
      mPauseMenuController(dependencies.pauseMenuController)
{
}

void PlayerConfigurationController::Reset()
{
    mControlState.Reset();
}

void PlayerConfigurationController::ConfigureAddedPlayer(Player& player)
{
    const bool isSoloClone =
        !mIsSecondPlayerJoined &&
        !mControlState.IsPlayerSplit() &&
        !mWorld.GetPlayers().empty();
    if (isSoloClone) {
        player.SetIsActive(false);
        player.SetControlLocked(true);
    }

    if (mIsSecondPlayerJoined) {
        player.SetSplitForm(true);
        player.SetControlLocked(false);
        player.SetIsActive(true);
    }
}

void PlayerConfigurationController::SynchronizeAfterStageReload()
{
    if (mIsSecondPlayerJoined) {
        PlayerSplitService::SynchronizeSecondPlayerAfterStageReload(
            mWorld.GetPlayers());
    }
}

void PlayerConfigurationController::JoinSecondPlayer()
{
    if (mIsSecondPlayerJoined || !mGamepadService.IsConnected()) {
        return;
    }

    if (mWorld.GetPlayers().size() < 2 &&
        !mPlayerLoader.CreatePlayerFromCurrentStage(2)) {
        return;
    }

    if (!SplitPlayer()) {
        return;
    }

    mIsSecondPlayerJoined = true;
    SelectControlledPlayer(0);
}

void PlayerConfigurationController::ReturnToSinglePlayer()
{
    if (!mIsSecondPlayerJoined) {
        return;
    }

    mIsSecondPlayerJoined = false;
    MergePlayerInto(0);
}

bool PlayerConfigurationController::CanStartTwoPlayerFromPauseMenu() const
{
    return mIsSecondPlayerJoined ||
           (mProgressController.IsStageCleared(1) &&
            mGamepadService.IsConnected());
}

bool PlayerConfigurationController::ToggleSplit()
{
    if (!CanToggleSplit()) {
        return false;
    }

    const bool didChangeSplitState =
        mControlState.IsPlayerSplit() ? MergePlayer() : SplitPlayer();
    if (didChangeSplitState) {
        mSceneSystem.OnPlayerSplitMergeSucceeded();
    }
    return didChangeSplitState;
}

bool PlayerConfigurationController::CanToggleSplit() const
{
    const bool allowsPlayerSplitToggle =
        mSceneSystem.IsPlaying() ||
        mSceneSystem.IsWaitingForTutorialPlayerSplitMerge();
    if (!allowsPlayerSplitToggle || !CanChangeSoloConfiguration()) {
        return false;
    }

    return !mControlState.IsPlayerSplit() ||
           AreSplitPlayersCloseEnoughToMerge();
}

bool PlayerConfigurationController::CanChangeSoloConfiguration() const
{
    const bool isWaitingForTutorialConfigurationAction =
        mSceneSystem.IsWaitingForTutorialPlayerSwitch() ||
        mSceneSystem.IsWaitingForTutorialPlayerSplitMerge();
    const bool allowsPlayerConfigurationScene =
        mSceneSystem.IsPlaying() ||
        isWaitingForTutorialConfigurationAction;
    const bool allowsNormalPlayerInput =
        mCameraSystem.AllowsPlayerInput() &&
        !mSequenceSystem.LocksPlayerControl();
    const bool allowsPlayerConfigurationInput =
        isWaitingForTutorialConfigurationAction ||
        allowsNormalPlayerInput;

    return !mIsSecondPlayerJoined &&
           mWorld.GetPlayers().size() >= 2 &&
           allowsPlayerConfigurationScene &&
           !mPauseMenuController.IsOpen() &&
           allowsPlayerConfigurationInput;
}

bool PlayerConfigurationController::SplitPlayer()
{
    const std::vector<Player*>& players = mWorld.GetPlayers();
    if (!PlayerSplitService::ActivateSplit(players)) {
        return false;
    }

    mControlState.BeginPlayerSplit();
    SynchronizeSoloSplitResources(*players[0]);
    SelectControlledPlayer(1);
    return true;
}

bool PlayerConfigurationController::MergePlayer()
{
    if (!AreSplitPlayersCloseEnoughToMerge()) {
        return false;
    }

    return MergePlayerInto(mControlState.GetControlledPlayerIndex());
}

bool PlayerConfigurationController::AreSplitPlayersCloseEnoughToMerge() const
{
    return PlayerSplitService::ArePlayersCloseEnoughToMerge(
        mWorld.GetPlayers(), playerMergeMaximumDistanceWorldUnits);
}

bool PlayerConfigurationController::MergePlayerInto(int targetPlayerIndex)
{
    if (!PlayerSplitService::MergeIntoMainPlayer(
            mWorld.GetPlayers(), targetPlayerIndex)) {
        return false;
    }

    const std::optional<ControlledPlayerChange> selectionChange =
        mControlState.EndPlayerSplit();
    if (selectionChange) {
        mCameraSystem.SnapToControlledPlayer(
            selectionChange->previousPlayerIndex,
            selectionChange->currentPlayerIndex);
    }
    return true;
}

void PlayerConfigurationController::SelectControlledPlayer(int playerIndex)
{
    const std::optional<ControlledPlayerChange> selectionChange =
        mControlState.SelectControlledPlayer(playerIndex);
    if (selectionChange) {
        mCameraSystem.SnapToControlledPlayer(
            selectionChange->previousPlayerIndex,
            selectionChange->currentPlayerIndex);
    }
}

bool PlayerConfigurationController::SwitchControlledPlayer()
{
    if (!CanSwitchControlledPlayer()) {
        return false;
    }

    const std::vector<Player*>& players = mWorld.GetPlayers();
    const int previousIndex = mControlState.GetControlledPlayerIndex();
    int nextIndex = previousIndex;
    for (int offset = 1;
         offset <= static_cast<int>(players.size());
         ++offset) {
        const int candidateIndex =
            (previousIndex + offset) % static_cast<int>(players.size());
        Player* candidatePlayer =
            players[static_cast<std::size_t>(candidateIndex)];
        if (candidatePlayer && candidatePlayer->GetIsActive()) {
            nextIndex = candidateIndex;
            break;
        }
    }

    if (nextIndex != previousIndex) {
        SelectControlledPlayer(nextIndex);
        mSceneSystem.OnPlayerSwitchSucceeded();
        return true;
    }

    for (int candidateIndex = 0;
         candidateIndex < static_cast<int>(players.size());
         ++candidateIndex) {
        if (candidateIndex == previousIndex) {
            continue;
        }

        Player* waitingPlayer =
            players[static_cast<std::size_t>(candidateIndex)];
        if (!waitingPlayer || !waitingPlayer->CancelWaitingBoatRide()) {
            continue;
        }

        mControlState.CancelScheduledControlSwitch();
        SelectControlledPlayer(candidateIndex);
        mSceneSystem.OnPlayerSwitchSucceeded();
        return true;
    }

    return false;
}

bool PlayerConfigurationController::CanSwitchControlledPlayer() const
{
    const bool allowsPlayerSwitch =
        mSceneSystem.IsPlaying() ||
        mSceneSystem.IsWaitingForTutorialPlayerSwitch();
    if (!mControlState.IsPlayerSplit() ||
        !CanChangeSoloConfiguration() ||
        mWorld.GetPlayers().size() < 2 ||
        !allowsPlayerSwitch) {
        return false;
    }

    const std::vector<Player*>& players = mWorld.GetPlayers();
    for (int index = 0; index < static_cast<int>(players.size()); ++index) {
        if (index != mControlState.GetControlledPlayerIndex() &&
            players[static_cast<std::size_t>(index)] &&
            (players[static_cast<std::size_t>(index)]->GetIsActive() ||
             players[static_cast<std::size_t>(index)]->IsWaitingForBoat())) {
            return true;
        }
    }
    return false;
}

void PlayerConfigurationController::RequestControlSwitchAfterBoarding()
{
    if (!mIsSecondPlayerJoined && mControlState.IsPlayerSplit()) {
        mControlState.ScheduleControlSwitchAfterBoarding(
            boardingControlSwitchDelaySeconds);
    }
}

void PlayerConfigurationController::UpdatePendingControlSwitch(
    float deltaTime)
{
    const std::vector<Player*>& players = mWorld.GetPlayers();
    const int controlledPlayerIndex =
        mControlState.GetControlledPlayerIndex();
    const bool isControlledPlayerActive =
        controlledPlayerIndex >= 0 &&
        controlledPlayerIndex < static_cast<int>(players.size()) &&
        players[static_cast<std::size_t>(controlledPlayerIndex)] &&
        players[static_cast<std::size_t>(controlledPlayerIndex)]->GetIsActive();

    int activePlayerIndex = -1;
    if (!isControlledPlayerActive) {
        for (int index = 0; index < static_cast<int>(players.size()); ++index) {
            Player* player = players[static_cast<std::size_t>(index)];
            if (player && player->GetIsActive()) {
                activePlayerIndex = index;
                break;
            }
        }
    }

    const std::optional<ControlledPlayerChange> selectionChange =
        mControlState.UpdateScheduledControlSwitch(
            deltaTime,
            mIsSecondPlayerJoined,
            activePlayerIndex);
    if (selectionChange) {
        mCameraSystem.SnapToControlledPlayer(
            selectionChange->previousPlayerIndex,
            selectionChange->currentPlayerIndex);
    }
}

void PlayerConfigurationController::MergeSoloSplitAfterBoatArrival()
{
    if (!mIsSecondPlayerJoined && mControlState.IsPlayerSplit()) {
        MergePlayerInto(mControlState.GetControlledPlayerIndex());
    }
}

void PlayerConfigurationController::MergeSoloSplitBeforeRestart()
{
    if (!mIsSecondPlayerJoined && mControlState.IsPlayerSplit()) {
        MergePlayerInto(0);
    }
}

void PlayerConfigurationController::SynchronizeSoloSplitResources(
    const Player& sourcePlayer)
{
    if (mIsSecondPlayerJoined || !mControlState.IsPlayerSplit()) {
        return;
    }

    PlayerSplitService::SynchronizeSharedResources(
        mWorld.GetPlayers(), sourcePlayer);
}

bool PlayerConfigurationController::IsSecondPlayerJoined() const
{
    return mIsSecondPlayerJoined;
}

bool PlayerConfigurationController::IsPlayerSplit() const
{
    return mControlState.IsPlayerSplit();
}

int PlayerConfigurationController::GetControlledPlayerIndex() const
{
    return mControlState.GetControlledPlayerIndex();
}

Player* PlayerConfigurationController::GetMainPlayer() const
{
    if (!mIsSecondPlayerJoined) {
        if (Player* controlledPlayer = GetControlledPlayer()) {
            return controlledPlayer;
        }
    }

    return mWorld.GetMainPlayer();
}

Player* PlayerConfigurationController::GetControlledPlayer() const
{
    const std::vector<Player*>& players = mWorld.GetPlayers();
    const int controlledPlayerIndex =
        mControlState.GetControlledPlayerIndex();
    if (controlledPlayerIndex < 0 ||
        controlledPlayerIndex >= static_cast<int>(players.size())) {
        return players.empty() ? nullptr : players[0];
    }

    return players[static_cast<std::size_t>(controlledPlayerIndex)];
}
