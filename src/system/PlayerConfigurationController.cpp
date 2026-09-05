#include <GL/glew.h>

#include "system/PlayerConfigurationController.h"

#include "actor/Planet.h"
#include "actor/Player.h"
#include "system/CameraSystem.h"
#include "system/GameWorld.h"
#include "system/GameProgressController.h"
#include "system/GamepadRumbleService.h"
#include "system/PauseMenuController.h"
#include "system/PhysicsSystem.h"
#include "system/PlayerSplitService.h"
#include "system/SceneSystem.h"
#include "system/sequence/SequenceSystem.h"
#include "system/actor_loader/StagePlayerLoader.h"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <optional>
#include <vector>

#include <glm/gtc/constants.hpp>

namespace {
constexpr float playerMergeMaximumDistanceWorldUnits = 1.25f;
constexpr float boardingControlSwitchDelaySeconds = 0.5f;
constexpr float splitMergeTransitionDurationSeconds = 0.4f;
constexpr float splitPlayerInitialScaleMultiplier = 0.2f;
constexpr float splitPlayerSeparationWorldUnits = 1.0f;
constexpr float splitPlayerArcHeightWorldUnits = 0.45f;
constexpr float splitPositionMaximumCorrectionWorldUnits = 0.15f;
constexpr float splitPositionMinimumWalkableNormalUpDot = 0.65f;
constexpr int splitCollisionSweepSegmentCount = 8;

float CalculateSmoothstep(float progress)
{
    const float clampedProgress = glm::clamp(progress, 0.0f, 1.0f);
    return clampedProgress * clampedProgress *
           (3.0f - 2.0f * clampedProgress);
}

glm::vec3 CalculateUpDirection(const Player& mainPlayer)
{
    glm::vec3 upDirection = mainPlayer.GetUpVec();
    const float upLengthSquared = glm::dot(upDirection, upDirection);
    if (upLengthSquared <= 0.000001f) {
        upDirection = glm::vec3(0.0f, 1.0f, 0.0f);
    } else {
        upDirection /= std::sqrt(upLengthSquared);
    }
    return upDirection;
}

glm::vec3 CalculateSplitDirection(const Player& mainPlayer)
{
    const glm::vec3 upDirection = CalculateUpDirection(mainPlayer);
    glm::vec3 splitDirection = mainPlayer.GetRightVec();
    splitDirection -= upDirection * glm::dot(splitDirection, upDirection);
    const float splitDirectionLengthSquared =
        glm::dot(splitDirection, splitDirection);
    if (splitDirectionLengthSquared <= 0.000001f) {
        splitDirection = glm::cross(upDirection, mainPlayer.GetFacingForwardVec());
    }

    const float fallbackLengthSquared = glm::dot(splitDirection, splitDirection);
    return fallbackLengthSquared > 0.000001f
        ? splitDirection / std::sqrt(fallbackLengthSquared)
        : glm::vec3(1.0f, 0.0f, 0.0f);
}

glm::vec3 CalculateSplitSurfacePosition(
    const Player& mainPlayer,
    const glm::vec3& splitDirection,
    float easedProgress)
{
    const glm::vec3 mainPosition = mainPlayer.GetPos();
    Planet* planet = mainPlayer.GetCurrentPlanet();
    if (!planet) {
        return mainPosition +
               splitDirection *
                   splitPlayerSeparationWorldUnits * easedProgress;
    }

    const glm::vec3 candidatePosition =
        mainPosition +
        splitDirection *
            splitPlayerSeparationWorldUnits * easedProgress;
    if (planet->GetPlanetShape() != Planet::PlanetShape::Normal) {
        const Planet::EllipseSurfaceProjection mainSurface =
            planet->CalculateEllipseSurfaceProjection(mainPosition);
        const Planet::EllipseSurfaceProjection candidateSurface =
            planet->CalculateEllipseSurfaceProjection(candidatePosition);
        const float mainSurfaceOffset = glm::dot(
            mainPosition - mainSurface.position,
            mainSurface.outwardNormal);
        return candidateSurface.position +
               candidateSurface.outwardNormal * mainSurfaceOffset;
    }

    const glm::vec3 mainRadialDirection =
        mainPosition - planet->GetPos();
    const float mainCenterDistance = glm::length(mainRadialDirection);
    if (mainCenterDistance <= 0.000001f) {
        return candidatePosition;
    }

    const glm::vec3 candidateRadialDirection =
        mainRadialDirection +
        splitDirection *
            splitPlayerSeparationWorldUnits * easedProgress;
    const float candidateRadialLength = glm::length(candidateRadialDirection);
    if (candidateRadialLength <= 0.000001f) {
        return candidatePosition;
    }

    return planet->GetPos() +
           candidateRadialDirection / candidateRadialLength *
               mainCenterDistance;
}
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
      mPauseMenuController(dependencies.pauseMenuController),
      mPhysicsSystem(dependencies.physicsSystem)
{
}

void PlayerConfigurationController::Reset()
{
    mControlState.Reset();
    mSplitMergeTransition = {};
    mIsSplitMergeButtonHeld = false;
    mIsMergeGuideRequested = false;
    mSplitGuardState.Reset();
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
    if (!CanStartSplitMergeInput()) {
        return false;
    }

    if (!mControlState.IsPlayerSplit()) {
        return BeginSoloSplitTransition();
    }

    if (AreSplitPlayersCloseEnoughToMerge()) {
        return BeginSoloMergeTransition();
    }

    mIsMergeGuideRequested = true;
    return true;
}

bool PlayerConfigurationController::CanToggleSplit() const
{
    if (!CanStartSplitMergeInput()) {
        return false;
    }

    return !mControlState.IsPlayerSplit() ||
           AreSplitPlayersCloseEnoughToMerge();
}

bool PlayerConfigurationController::CanStartSplitMergeInput() const
{
    const bool allowsPlayerSplitToggle =
        mSceneSystem.IsPlaying() ||
        mSceneSystem.IsWaitingForTutorialPlayerSplitMerge();
    if (!allowsPlayerSplitToggle ||
        IsSplitMergeTransitionActive() ||
        !CanChangeSoloConfiguration()) {
        return false;
    }

    const std::vector<Player*>& players = mWorld.GetPlayers();
    if (mControlState.IsPlayerSplit()) {
        return players.size() >= 2 &&
               players[0] && players[0]->GetIsActive() &&
               players[1] && players[1]->GetIsActive();
    }

    if (players.empty() || !players[0]) {
        return false;
    }

    glm::vec3 splitDirection;
    return TryResolveSplitDirection(*players[0], splitDirection);
}

void PlayerConfigurationController::SetSplitMergeButtonHeld(bool isHeld)
{
    mIsSplitMergeButtonHeld = isHeld;
    if (!isHeld) {
        mIsMergeGuideRequested = false;
    }
}

bool PlayerConfigurationController::TryResolveMergeGuide(
    const Player*& targetPlayer,
    float& radiusWorldUnits) const
{
    targetPlayer = nullptr;
    radiusWorldUnits = 0.0f;
    if (!mIsMergeGuideRequested ||
        !mIsSplitMergeButtonHeld ||
        !mControlState.IsPlayerSplit() ||
        IsSplitMergeTransitionActive() ||
        !CanChangeSoloConfiguration()) {
        return false;
    }

    targetPlayer = FindMergeGuideTargetPlayer();
    if (!targetPlayer) {
        return false;
    }

    radiusWorldUnits = playerMergeMaximumDistanceWorldUnits;
    return true;
}

bool PlayerConfigurationController::TryConsumeSplitGuard(
    const Player& damagedPlayer)
{
    if (mIsSecondPlayerJoined ||
        !mControlState.IsPlayerSplit() ||
        GetControlledPlayer() == &damagedPlayer) {
        return false;
    }

    const std::vector<Player*>& players = mWorld.GetPlayers();
    const bool isActiveSplitPlayer =
        std::find(players.begin(), players.end(), &damagedPlayer) !=
            players.end() &&
        damagedPlayer.GetIsActive();
    return isActiveSplitPlayer && mSplitGuardState.ConsumeOne();
}

int PlayerConfigurationController::GetSplitGuardCount() const
{
    return mSplitGuardState.GetCount();
}

int PlayerConfigurationController::GetMaximumSplitGuardCount() const
{
    return PlayerSplitGuardState::MaximumGuardCount;
}

void PlayerConfigurationController::UpdateSplitMergeTransition(
    float deltaTime)
{
    mSplitGuardState.Update(deltaTime);
    UpdatePendingSoloMergeRequest();
    UpdateMergeRecall(deltaTime);
    if (!IsSplitMergeTransitionActive()) {
        return;
    }

    mSplitMergeTransition.elapsedSeconds += std::max(0.0f, deltaTime);
    const float progress = glm::clamp(
        mSplitMergeTransition.elapsedSeconds /
            splitMergeTransitionDurationSeconds,
        0.0f,
        1.0f);

    if (mSplitMergeTransition.kind ==
        SplitMergeTransitionKind::Splitting) {
        UpdateSoloSplitTransition(progress);
        if (progress >= 1.0f) {
            CompleteSoloSplitTransition();
        }
        return;
    }

    UpdateSoloMergeTransition(progress);
    if (progress >= 1.0f) {
        CompleteSoloMergeTransition();
    }
}

void PlayerConfigurationController::UpdateMergeRecall(float deltaTime)
{
    if (!mIsMergeGuideRequested ||
        !mIsSplitMergeButtonHeld ||
        IsSplitMergeTransitionActive()) {
        return;
    }

    Player* controlledPlayer = GetControlledPlayer();
    Player* recalledPlayer = FindMergeGuideTargetPlayer();
    if (!controlledPlayer ||
        !recalledPlayer ||
        !controlledPlayer->GetCurrentPlanet() ||
        controlledPlayer->GetCurrentPlanet() !=
            recalledPlayer->GetCurrentPlanet()) {
        return;
    }

    recalledPlayer->MoveTowardForMergeRecall(
        controlledPlayer->GetPos(),
        deltaTime);
}

bool PlayerConfigurationController::IsSplitMergeTransitionActive() const
{
    return mSplitMergeTransition.kind != SplitMergeTransitionKind::None;
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
           !IsSplitMergeTransitionActive() &&
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

bool PlayerConfigurationController::BeginSoloSplitTransition()
{
    const std::vector<Player*>& players = mWorld.GetPlayers();
    if (players.size() < 2 || !players[0] || !players[1]) {
        return false;
    }

    const glm::vec3 mainPlayerStartScale = players[0]->GetScale();
    glm::vec3 splitDirection;
    if (!TryResolveSplitDirection(*players[0], splitDirection)) {
        return false;
    }
    if (!PlayerSplitService::ActivateSplit(players)) {
        return false;
    }

    Player& mainPlayer = *players[0];
    Player& splitPlayer = *players[1];
    mainPlayer.SetScale(mainPlayerStartScale);
    const glm::vec3 finalSplitScale = splitPlayer.GetScale();
    splitPlayer.SetScale(
        finalSplitScale * splitPlayerInitialScaleMultiplier);
    splitPlayer.SetVelocity(glm::vec3(0.0f));
    splitPlayer.SetShouldJudgeLanding(false);

    mControlState.BeginPlayerSplit();
    SynchronizeSoloSplitResources(mainPlayer);
    mSplitMergeTransition = {
        .kind = SplitMergeTransitionKind::Splitting,
        .elapsedSeconds = 0.0f,
        .splitPlayerStartPosition = mainPlayer.GetPos(),
        .splitDirection = splitDirection,
        .splitPlayerStartScale = splitPlayer.GetScale(),
        .mainPlayerStartScale = mainPlayerStartScale,
    };
    return true;
}

bool PlayerConfigurationController::TryResolveSplitDirection(
    Player& mainPlayer,
    glm::vec3& splitDirection) const
{
    const glm::vec3 preferredDirection =
        CalculateSplitDirection(mainPlayer);
    if (IsSplitDirectionClear(mainPlayer, preferredDirection)) {
        splitDirection = preferredDirection;
        return true;
    }

    const glm::vec3 oppositeDirection = -preferredDirection;
    if (!IsSplitDirectionClear(mainPlayer, oppositeDirection)) {
        return false;
    }

    splitDirection = oppositeDirection;
    return true;
}

bool PlayerConfigurationController::IsSplitDirectionClear(
    Player& mainPlayer,
    const glm::vec3& splitDirection) const
{
    const glm::vec3 startPosition = mainPlayer.GetPos();
    const glm::vec3 upDirection = CalculateUpDirection(mainPlayer);
    glm::vec3 previousSweepPosition = startPosition;
    for (int segmentIndex = 1;
         segmentIndex <= splitCollisionSweepSegmentCount;
         ++segmentIndex) {
        const float progress =
            static_cast<float>(segmentIndex) /
            static_cast<float>(splitCollisionSweepSegmentCount);
        const float easedProgress = CalculateSmoothstep(progress);
        const float arcHeight =
            std::sin(glm::pi<float>() * progress) *
            splitPlayerArcHeightWorldUnits;
        const glm::vec3 sweepPosition =
            CalculateSplitSurfacePosition(
                mainPlayer,
                splitDirection,
                easedProgress) +
            upDirection * arcHeight;
        if (mPhysicsSystem.DoesActorSweepHitBlockingStage(
                mainPlayer,
                previousSweepPosition,
                sweepPosition)) {
            return false;
        }
        previousSweepPosition = sweepPosition;
    }

    const glm::vec3 desiredPosition =
        CalculateSplitSurfacePosition(
            mainPlayer,
            splitDirection,
            1.0f);
    if (glm::length(desiredPosition - startPosition) <= 0.000001f) {
        return false;
    }

    const ActorMovementCollisionResult collisionResult =
        mPhysicsSystem.ResolveMovementCollision(
            &mainPlayer,
            glm::vec3(0.0f),
            desiredPosition);
    const float correctionDistance = glm::length(
        collisionResult.resolvedPosition - desiredPosition);
    const float blockingNormalUpDot = glm::dot(
        collisionResult.blockingNormal,
        upDirection);
    const bool didHitNonWalkableSurface =
        collisionResult.didHitStage &&
        blockingNormalUpDot <
            splitPositionMinimumWalkableNormalUpDot;
    return !didHitNonWalkableSurface &&
           !collisionResult.hasUnresolvedStageOverlap &&
           correctionDistance <=
               splitPositionMaximumCorrectionWorldUnits;
}

bool PlayerConfigurationController::BeginSoloMergeTransition()
{
    if (!AreSplitPlayersCloseEnoughToMerge()) {
        return false;
    }

    const std::vector<Player*>& players = mWorld.GetPlayers();
    if (players.size() < 2 || !players[0] || !players[1]) {
        return false;
    }

    Player& mainPlayer = *players[0];
    Player& splitPlayer = *players[1];
    mIsMergeGuideRequested = false;
    mainPlayer.SetVelocity(glm::vec3(0.0f));
    splitPlayer.SetVelocity(glm::vec3(0.0f));
    splitPlayer.SetShouldJudgeLanding(false);
    mSplitMergeTransition = {
        .kind = SplitMergeTransitionKind::Merging,
        .elapsedSeconds = 0.0f,
        .splitPlayerStartPosition = splitPlayer.GetPos(),
        .splitPlayerStartScale = splitPlayer.GetScale(),
        .mainPlayerStartScale = mainPlayer.GetScale(),
    };
    return true;
}

void PlayerConfigurationController::UpdatePendingSoloMergeRequest()
{
    if (!mIsMergeGuideRequested) {
        return;
    }

    if (!mIsSplitMergeButtonHeld ||
        !mControlState.IsPlayerSplit() ||
        !CanChangeSoloConfiguration() ||
        !FindMergeGuideTargetPlayer()) {
        mIsMergeGuideRequested = false;
        return;
    }

    if (AreSplitPlayersCloseEnoughToMerge()) {
        mIsMergeGuideRequested = false;
        BeginSoloMergeTransition();
    }
}

Player* PlayerConfigurationController::FindMergeGuideTargetPlayer() const
{
    const std::vector<Player*>& players = mWorld.GetPlayers();
    if (players.size() < 2) {
        return nullptr;
    }

    const int controlledPlayerIndex =
        mControlState.GetControlledPlayerIndex();
    if (controlledPlayerIndex < 0 || controlledPlayerIndex >= 2) {
        return nullptr;
    }

    const int targetPlayerIndex =
        controlledPlayerIndex == 0 ? 1 : 0;
    Player* targetPlayer =
        players[static_cast<std::size_t>(targetPlayerIndex)];
    if (!targetPlayer || !targetPlayer->GetIsActive()) {
        return nullptr;
    }
    return targetPlayer;
}

void PlayerConfigurationController::UpdateSoloSplitTransition(
    float progress)
{
    const std::vector<Player*>& players = mWorld.GetPlayers();
    if (players.size() < 2 || !players[0] || !players[1]) {
        mSplitMergeTransition = {};
        return;
    }

    Player& mainPlayer = *players[0];
    Player& splitPlayer = *players[1];
    const float easedProgress = CalculateSmoothstep(progress);
    const glm::vec3 upDirection = CalculateUpDirection(mainPlayer);
    const glm::vec3 splitDirection =
        mSplitMergeTransition.splitDirection;
    const float arcHeight =
        std::sin(glm::pi<float>() * progress) *
        splitPlayerArcHeightWorldUnits;
    splitPlayer.SetPos(
        CalculateSplitSurfacePosition(
            mainPlayer,
            splitDirection,
            easedProgress) +
        upDirection * arcHeight);

    const glm::vec3 finalSplitScale =
        splitPlayer.GetBaseScale() * Player::SplitBodyScaleMultiplier;
    splitPlayer.SetScale(glm::mix(
        mSplitMergeTransition.splitPlayerStartScale,
        finalSplitScale,
        easedProgress));
    const glm::vec3 finalMainPlayerScale =
        mainPlayer.GetBaseScale() * Player::SplitBodyScaleMultiplier;
    mainPlayer.SetScale(glm::mix(
        mSplitMergeTransition.mainPlayerStartScale,
        finalMainPlayerScale,
        easedProgress));
}

void PlayerConfigurationController::UpdateSoloMergeTransition(
    float progress)
{
    const std::vector<Player*>& players = mWorld.GetPlayers();
    if (players.size() < 2 || !players[0] || !players[1]) {
        mSplitMergeTransition = {};
        return;
    }

    Player& mainPlayer = *players[0];
    Player& splitPlayer = *players[1];
    const float easedProgress = CalculateSmoothstep(progress);
    const glm::vec3 upDirection = CalculateUpDirection(mainPlayer);
    const float arcHeight =
        std::sin(glm::pi<float>() * progress) *
        splitPlayerArcHeightWorldUnits * 0.5f;
    splitPlayer.SetPos(
        glm::mix(
            mSplitMergeTransition.splitPlayerStartPosition,
            mainPlayer.GetPos(),
            easedProgress) +
        upDirection * arcHeight);

    const glm::vec3 hiddenSplitScale =
        splitPlayer.GetBaseScale() *
        Player::SplitBodyScaleMultiplier *
        splitPlayerInitialScaleMultiplier;
    splitPlayer.SetScale(glm::mix(
        mSplitMergeTransition.splitPlayerStartScale,
        hiddenSplitScale,
        easedProgress));
    mainPlayer.SetScale(glm::mix(
        mSplitMergeTransition.mainPlayerStartScale,
        mainPlayer.GetBaseScale(),
        easedProgress));
}

void PlayerConfigurationController::CompleteSoloSplitTransition()
{
    const std::vector<Player*>& players = mWorld.GetPlayers();
    if (players.size() < 2 || !players[0] || !players[1]) {
        mSplitMergeTransition = {};
        return;
    }

    players[1]->SetShouldJudgeLanding(true);
    mSplitMergeTransition = {};
    SelectControlledPlayer(
        1,
        ControlledPlayerCameraTransition::Smooth);
    mSceneSystem.OnPlayerSplitMergeSucceeded();
}

void PlayerConfigurationController::CompleteSoloMergeTransition()
{
    mSplitMergeTransition = {};
    if (MergePlayerInto(
            0,
            ControlledPlayerCameraTransition::Smooth)) {
        mSceneSystem.OnPlayerSplitMergeSucceeded();
    }
}

bool PlayerConfigurationController::AreSplitPlayersCloseEnoughToMerge() const
{
    return PlayerSplitService::ArePlayersCloseEnoughToMerge(
        mWorld.GetPlayers(), playerMergeMaximumDistanceWorldUnits);
}

bool PlayerConfigurationController::MergePlayerInto(
    int targetPlayerIndex,
    ControlledPlayerCameraTransition cameraTransition)
{
    mIsMergeGuideRequested = false;
    if (!PlayerSplitService::MergeIntoMainPlayer(
            mWorld.GetPlayers(), targetPlayerIndex)) {
        return false;
    }

    const std::optional<ControlledPlayerChange> selectionChange =
        mControlState.EndPlayerSplit();
    if (selectionChange) {
        if (cameraTransition == ControlledPlayerCameraTransition::Smooth) {
            mCameraSystem.TransitionToControlledPlayer(
                selectionChange->previousPlayerIndex,
                selectionChange->currentPlayerIndex);
        } else {
            mCameraSystem.SnapToControlledPlayer(
                selectionChange->previousPlayerIndex,
                selectionChange->currentPlayerIndex);
        }
    }
    return true;
}

void PlayerConfigurationController::SelectControlledPlayer(
    int playerIndex,
    ControlledPlayerCameraTransition cameraTransition)
{
    const std::optional<ControlledPlayerChange> selectionChange =
        mControlState.SelectControlledPlayer(playerIndex);
    if (selectionChange) {
        if (cameraTransition == ControlledPlayerCameraTransition::Smooth) {
            mCameraSystem.TransitionToControlledPlayer(
                selectionChange->previousPlayerIndex,
                selectionChange->currentPlayerIndex);
        } else {
            mCameraSystem.SnapToControlledPlayer(
                selectionChange->previousPlayerIndex,
                selectionChange->currentPlayerIndex);
        }
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
