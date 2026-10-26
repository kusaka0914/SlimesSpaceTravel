#include <GL/glew.h>

#include "system/PlayerConfigurationController.h"

#include "actor/Planet.h"
#include "actor/Player.h"
#include "system/CameraSystem.h"
#include "system/GameProgressController.h"
#include "system/GamepadRumbleService.h"
#include "system/PauseMenuController.h"
#include "system/PhysicsSystem.h"
#include "system/PlayerSplitService.h"
#include "system/SceneSystem.h"
#include "system/actor_loader/StagePlayerLoader.h"
#include "system/sequence/SequenceSystem.h"
#include "world/GameWorld.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
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
    return clampedProgress * clampedProgress * (3.0f - 2.0f * clampedProgress);
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
    const float splitDirectionLengthSquared = glm::dot(splitDirection, splitDirection);
    if (splitDirectionLengthSquared <= 0.000001f) {
        splitDirection = glm::cross(upDirection, mainPlayer.GetFacingForwardVec());
    }

    const float fallbackLengthSquared = glm::dot(splitDirection, splitDirection);
    return fallbackLengthSquared > 0.000001f ? splitDirection / std::sqrt(fallbackLengthSquared)
                                             : glm::vec3(1.0f, 0.0f, 0.0f);
}

glm::vec3 CalculateSplitSurfacePosition(const Player& mainPlayer, const glm::vec3& splitDirection, float easedProgress)
{
    const glm::vec3 mainPosition = mainPlayer.GetPos();
    Planet* planet = mainPlayer.GetCurrentPlanet();
    if (!planet) {
        return mainPosition + splitDirection * splitPlayerSeparationWorldUnits * easedProgress;
    }

    const glm::vec3 candidatePosition = mainPosition + splitDirection * splitPlayerSeparationWorldUnits * easedProgress;
    if (planet->GetPlanetShape() != Planet::PlanetShape::Normal) {
        const Planet::EllipseSurfaceProjection mainSurface = planet->CalculateEllipseSurfaceProjection(mainPosition);
        const Planet::EllipseSurfaceProjection candidateSurface =
            planet->CalculateEllipseSurfaceProjection(candidatePosition);

        // 横方向へ分離しても、元のプレイヤーの惑星表面からの高さを維持する。
        const float mainSurfaceOffset = glm::dot(mainPosition - mainSurface.position, mainSurface.outwardNormal);
        return candidateSurface.position + candidateSurface.outwardNormal * mainSurfaceOffset;
    }

    const glm::vec3 mainRadialDirection = mainPosition - planet->GetPos();
    const float mainCenterDistance = glm::length(mainRadialDirection);
    if (mainCenterDistance <= 0.000001f) {
        return candidatePosition;
    }

    const glm::vec3 candidateRadialDirection =
        mainRadialDirection + splitDirection * splitPlayerSeparationWorldUnits * easedProgress;
    const float candidateRadialLength = glm::length(candidateRadialDirection);
    if (candidateRadialLength <= 0.000001f) {
        return candidatePosition;
    }

    return planet->GetPos() + candidateRadialDirection / candidateRadialLength * mainCenterDistance;
}
} // namespace

PlayerConfigurationController::PlayerConfigurationController(const PlayerConfigurationDependencies& dependencies)
    : mWorld(dependencies.world),
      mPlayerLoader(dependencies.playerLoader),
      mSceneSystem(dependencies.sceneSystem),
      mCameraSystem(dependencies.cameraSystem),
      mSequenceSystem(dependencies.sequenceSystem),
      mProgressController(dependencies.progressController),
      mGamepadService(dependencies.gamepadService),
      mPauseMenuController(dependencies.pauseMenuController),
      mPhysicsSystem(dependencies.physicsSystem),
      mAllowsKeyboardOnlyTwoPlayer(dependencies.allowsKeyboardOnlyTwoPlayer)
{
}

void PlayerConfigurationController::Reset()
{
    mRuntimeState.mControlState.Reset();
    mRuntimeState.mSplitMergeTransition = {};
    mRuntimeState.mIsSplitMergeButtonHeld = false;
    mRuntimeState.mIsMergeGuideRequested = false;
    mRuntimeState.mSplitGuardState.Reset();
}

void PlayerConfigurationController::ConfigureAddedPlayer(Player& player)
{
    const bool isSoloClone = !mRuntimeState.mIsSecondPlayerJoined && !mRuntimeState.mControlState.IsPlayerSplit() &&
                             !mWorld.GetPlayers().empty();
    if (isSoloClone) {
        player.SetIsActive(false);
        player.SetControlLocked(true);
    }

    if (mRuntimeState.mIsSecondPlayerJoined) {
        player.SetSplitForm(true);
        player.SetControlLocked(false);
        player.SetIsActive(true);
    }
}

void PlayerConfigurationController::SynchronizeAfterStageReload()
{
    if (mRuntimeState.mIsSecondPlayerJoined) {
        PlayerSplitService::SynchronizeSecondPlayerAfterStageReload(mWorld.GetPlayers());
    }
}

void PlayerConfigurationController::JoinSecondPlayer()
{
    const bool canJoinWithoutController = mAllowsKeyboardOnlyTwoPlayer && !mGamepadService.IsConnected();
    if (mRuntimeState.mIsSecondPlayerJoined || (!mGamepadService.IsConnected() && !canJoinWithoutController)) {
        return;
    }

    if (mWorld.GetPlayers().size() < 2 && !mPlayerLoader.CreatePlayerFromCurrentStage(2)) {
        return;
    }

    if (!SplitPlayer()) {
        return;
    }

    mRuntimeState.mIsSecondPlayerJoined = true;
    SelectControlledPlayer(0);
}

void PlayerConfigurationController::ReturnToSinglePlayer()
{
    if (!mRuntimeState.mIsSecondPlayerJoined) {
        return;
    }

    mRuntimeState.mIsSecondPlayerJoined = false;
    MergePlayerInto(0);
}

bool PlayerConfigurationController::CanStartTwoPlayerFromPauseMenu() const
{
    return mRuntimeState.mIsSecondPlayerJoined ||
           (mProgressController.IsStageCleared(1) && (mGamepadService.IsConnected() || mAllowsKeyboardOnlyTwoPlayer));
}

PlayerConfigurationController::SplitToggleResult PlayerConfigurationController::ToggleSplit()
{
    const auto result = CheckSplitMergeInput();
    if (result != SplitToggleResult::Accepted) {
        return result;
    }

    if (!mRuntimeState.mControlState.IsPlayerSplit()) {
        return BeginSoloSplitTransition();
    }

    if (AreSplitPlayersCloseEnoughToMerge()) {
        return BeginSoloMergeTransition() ? SplitToggleResult::Accepted : SplitToggleResult::Unavailable;
    }

    mRuntimeState.mIsMergeGuideRequested = true;
    return SplitToggleResult::MergeTooFar;
}

bool PlayerConfigurationController::CanToggleSplit() const
{
    if (CheckSplitMergeInput() != SplitToggleResult::Accepted) {
        return false;
    }

    return !mRuntimeState.mControlState.IsPlayerSplit() || AreSplitPlayersCloseEnoughToMerge();
}

PlayerConfigurationController::SplitToggleResult PlayerConfigurationController::CheckSplitMergeInput() const
{
    const bool allowsPlayerSplitToggle =
        mSceneSystem.IsPlaying() || mSceneSystem.IsWaitingForTutorialPlayerSplitMerge();
    if (!allowsPlayerSplitToggle || IsSplitMergeTransitionActive() || !CanChangeSoloConfiguration()) {
        return SplitToggleResult::Unavailable;
    }

    const std::vector<Player*>& players = mWorld.GetPlayers();
    if (mRuntimeState.mControlState.IsPlayerSplit()) {
        const bool hasActiveSplitPlayers =
            players.size() >= 2 && players[0] && players[0]->GetIsActive() && players[1] && players[1]->GetIsActive();
        return hasActiveSplitPlayers ? SplitToggleResult::Accepted : SplitToggleResult::Unavailable;
    }

    if (players.size() < 2 || !players[0] || !players[1]) {
        return SplitToggleResult::Unavailable;
    }

    glm::vec3 splitDirection;
    bool wasBlockedByObstacle = false;
    if (TryResolveSplitDirection(*players[0], splitDirection, wasBlockedByObstacle)) {
        return SplitToggleResult::Accepted;
    }
    return wasBlockedByObstacle ? SplitToggleResult::BlockedByObstacle : SplitToggleResult::Unavailable;
}

void PlayerConfigurationController::SetSplitMergeButtonHeld(bool isHeld)
{
    mRuntimeState.mIsSplitMergeButtonHeld = isHeld;
    if (!isHeld) {
        mRuntimeState.mIsMergeGuideRequested = false;
    }
}

bool PlayerConfigurationController::TryResolveMergeGuide(const Player*& targetPlayer, float& radiusWorldUnits) const
{
    targetPlayer = nullptr;
    radiusWorldUnits = 0.0f;
    if (!mRuntimeState.mIsMergeGuideRequested || !mRuntimeState.mIsSplitMergeButtonHeld ||
        !mRuntimeState.mControlState.IsPlayerSplit() || IsSplitMergeTransitionActive() ||
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

bool PlayerConfigurationController::TryConsumeSplitGuard(const Player& damagedPlayer)
{
    if (mRuntimeState.mIsSecondPlayerJoined || !mRuntimeState.mControlState.IsPlayerSplit() ||
        GetControlledPlayer() == &damagedPlayer) {
        return false;
    }

    const std::vector<Player*>& players = mWorld.GetPlayers();
    const bool isActiveSplitPlayer =
        std::find(players.begin(), players.end(), &damagedPlayer) != players.end() && damagedPlayer.GetIsActive();
    return isActiveSplitPlayer && mRuntimeState.mSplitGuardState.ConsumeOne();
}

int PlayerConfigurationController::GetSplitGuardCount() const
{
    return mRuntimeState.mSplitGuardState.GetCount();
}

int PlayerConfigurationController::GetMaximumSplitGuardCount() const
{
    return PlayerSplitGuardState::MaximumGuardCount;
}

void PlayerConfigurationController::UpdateSplitMergeTransition(float deltaTime)
{
    mRuntimeState.mSplitGuardState.Update(deltaTime);
    UpdatePendingSoloMergeRequest();
    UpdateMergeRecall(deltaTime);
    if (!IsSplitMergeTransitionActive()) {
        return;
    }

    mRuntimeState.mSplitMergeTransition.elapsedSeconds += std::max(0.0f, deltaTime);
    const float progress = glm::clamp(
        mRuntimeState.mSplitMergeTransition.elapsedSeconds / splitMergeTransitionDurationSeconds, 0.0f, 1.0f);

    if (mRuntimeState.mSplitMergeTransition.kind == SplitMergeTransitionKind::Splitting) {
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
    if (!mRuntimeState.mIsMergeGuideRequested || !mRuntimeState.mIsSplitMergeButtonHeld ||
        IsSplitMergeTransitionActive()) {
        return;
    }

    Player* controlledPlayer = GetControlledPlayer();
    Player* recalledPlayer = FindMergeGuideTargetPlayer();
    if (!controlledPlayer || !recalledPlayer || !controlledPlayer->GetCurrentPlanet() ||
        controlledPlayer->GetCurrentPlanet() != recalledPlayer->GetCurrentPlanet()) {
        return;
    }

    recalledPlayer->MoveTowardForMergeRecall(controlledPlayer->GetPos(), deltaTime);
}

bool PlayerConfigurationController::IsSplitMergeTransitionActive() const
{
    return mRuntimeState.mSplitMergeTransition.kind != SplitMergeTransitionKind::None;
}

bool PlayerConfigurationController::CanChangeSoloConfiguration() const
{
    const bool isWaitingForTutorialConfigurationAction =
        mSceneSystem.IsWaitingForTutorialPlayerSwitch() || mSceneSystem.IsWaitingForTutorialPlayerSplitMerge();
    const bool allowsPlayerConfigurationScene = mSceneSystem.IsPlaying() || isWaitingForTutorialConfigurationAction;
    const bool allowsNormalPlayerInput = mCameraSystem.AllowsPlayerInput() && !mSequenceSystem.LocksPlayerControl();
    const bool allowsPlayerConfigurationInput = isWaitingForTutorialConfigurationAction || allowsNormalPlayerInput;

    return !mRuntimeState.mIsSecondPlayerJoined && !IsSplitMergeTransitionActive() && mWorld.GetPlayers().size() >= 2 &&
           allowsPlayerConfigurationScene && !mPauseMenuController.IsOpen() && allowsPlayerConfigurationInput;
}

bool PlayerConfigurationController::SplitPlayer()
{
    const std::vector<Player*>& players = mWorld.GetPlayers();
    if (!PlayerSplitService::ActivateSplit(players)) {
        return false;
    }

    mRuntimeState.mControlState.BeginPlayerSplit();
    SynchronizeSoloSplitResources(*players[0]);
    SelectControlledPlayer(1);
    return true;
}

PlayerConfigurationController::SplitToggleResult PlayerConfigurationController::BeginSoloSplitTransition()
{
    const std::vector<Player*>& players = mWorld.GetPlayers();
    if (players.size() < 2 || !players[0] || !players[1]) {
        return SplitToggleResult::Unavailable;
    }

    const glm::vec3 mainPlayerStartScale = players[0]->GetScale();
    glm::vec3 splitDirection;
    bool wasBlockedByObstacle = false;
    if (!TryResolveSplitDirection(*players[0], splitDirection, wasBlockedByObstacle)) {
        return wasBlockedByObstacle ? SplitToggleResult::BlockedByObstacle : SplitToggleResult::Unavailable;
    }
    if (!PlayerSplitService::ActivateSplit(players)) {
        return SplitToggleResult::Unavailable;
    }

    Player& mainPlayer = *players[0];
    Player& splitPlayer = *players[1];
    mainPlayer.SetScale(mainPlayerStartScale);
    const glm::vec3 finalSplitScale = splitPlayer.GetScale();
    splitPlayer.SetScale(finalSplitScale * splitPlayerInitialScaleMultiplier);
    splitPlayer.SetVelocity(glm::vec3(0.0f));
    splitPlayer.SetShouldJudgeLanding(false);

    mRuntimeState.mControlState.BeginPlayerSplit();
    SynchronizeSoloSplitResources(mainPlayer);
    mRuntimeState.mSplitMergeTransition = {
        .kind = SplitMergeTransitionKind::Splitting,
        .elapsedSeconds = 0.0f,
        .splitPlayerStartPosition = mainPlayer.GetPos(),
        .splitDirection = splitDirection,
        .splitPlayerStartScale = splitPlayer.GetScale(),
        .mainPlayerStartScale = mainPlayerStartScale,
    };
    return SplitToggleResult::Accepted;
}

bool PlayerConfigurationController::TryResolveSplitDirection(Player& mainPlayer, glm::vec3& splitDirection,
                                                             bool& wasBlockedByObstacle) const
{
    wasBlockedByObstacle = false;
    bool preferredWasBlocked = false;
    const glm::vec3 preferredDirection = CalculateSplitDirection(mainPlayer);
    if (IsSplitDirectionClear(mainPlayer, preferredDirection, preferredWasBlocked)) {
        splitDirection = preferredDirection;
        return true;
    }

    const glm::vec3 oppositeDirection = -preferredDirection;
    bool oppositeWasBlocked = false;
    if (!IsSplitDirectionClear(mainPlayer, oppositeDirection, oppositeWasBlocked)) {
        wasBlockedByObstacle = preferredWasBlocked || oppositeWasBlocked;
        return false;
    }

    splitDirection = oppositeDirection;
    return true;
}

bool PlayerConfigurationController::IsSplitDirectionClear(Player& mainPlayer, const glm::vec3& splitDirection,
                                                          bool& wasBlockedByObstacle) const
{
    wasBlockedByObstacle = false;
    const glm::vec3 startPosition = mainPlayer.GetPos();
    const glm::vec3 upDirection = CalculateUpDirection(mainPlayer);
    glm::vec3 previousSweepPosition = startPosition;
    for (int segmentIndex = 1; segmentIndex <= splitCollisionSweepSegmentCount; ++segmentIndex) {
        const float progress = static_cast<float>(segmentIndex) / static_cast<float>(splitCollisionSweepSegmentCount);
        const float easedProgress = CalculateSmoothstep(progress);
        const float arcHeight = std::sin(glm::pi<float>() * progress) * splitPlayerArcHeightWorldUnits;
        const glm::vec3 sweepPosition =
            CalculateSplitSurfacePosition(mainPlayer, splitDirection, easedProgress) + upDirection * arcHeight;
        if (mPhysicsSystem.DoesActorSweepHitBlockingStage(mainPlayer, previousSweepPosition, sweepPosition)) {
            wasBlockedByObstacle = true;
            return false;
        }
        previousSweepPosition = sweepPosition;
    }

    const glm::vec3 desiredPosition = CalculateSplitSurfacePosition(mainPlayer, splitDirection, 1.0f);
    if (glm::length(desiredPosition - startPosition) <= 0.000001f) {
        return false;
    }

    const ActorMovementCollisionResult collisionResult =
        mPhysicsSystem.ResolveMovementCollision(&mainPlayer, glm::vec3(0.0f), desiredPosition);
    const float correctionDistance = glm::length(collisionResult.resolvedPosition - desiredPosition);
    const float blockingNormalUpDot = glm::dot(collisionResult.blockingNormal, upDirection);
    const bool didHitNonWalkableSurface =
        collisionResult.didHitStage && blockingNormalUpDot < splitPositionMinimumWalkableNormalUpDot;
    wasBlockedByObstacle =
        didHitNonWalkableSurface || collisionResult.hasUnresolvedStageOverlap ||
        (std::isfinite(correctionDistance) && correctionDistance > splitPositionMaximumCorrectionWorldUnits);
    return !didHitNonWalkableSurface && !collisionResult.hasUnresolvedStageOverlap &&
           correctionDistance <= splitPositionMaximumCorrectionWorldUnits;
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
    mRuntimeState.mIsMergeGuideRequested = false;
    mainPlayer.SetVelocity(glm::vec3(0.0f));
    splitPlayer.SetVelocity(glm::vec3(0.0f));
    splitPlayer.SetShouldJudgeLanding(false);
    mRuntimeState.mSplitMergeTransition = {
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
    if (!mRuntimeState.mIsMergeGuideRequested) {
        return;
    }

    if (!mRuntimeState.mIsSplitMergeButtonHeld || !mRuntimeState.mControlState.IsPlayerSplit() ||
        !CanChangeSoloConfiguration() || !FindMergeGuideTargetPlayer()) {
        mRuntimeState.mIsMergeGuideRequested = false;
        return;
    }

    if (AreSplitPlayersCloseEnoughToMerge()) {
        mRuntimeState.mIsMergeGuideRequested = false;
        BeginSoloMergeTransition();
    }
}

Player* PlayerConfigurationController::FindMergeGuideTargetPlayer() const
{
    const std::vector<Player*>& players = mWorld.GetPlayers();
    if (players.size() < 2) {
        return nullptr;
    }

    const int controlledPlayerIndex = mRuntimeState.mControlState.GetControlledPlayerIndex();
    if (controlledPlayerIndex < 0 || controlledPlayerIndex >= 2) {
        return nullptr;
    }

    const int targetPlayerIndex = controlledPlayerIndex == 0 ? 1 : 0;
    Player* targetPlayer = players[static_cast<std::size_t>(targetPlayerIndex)];
    if (!targetPlayer || !targetPlayer->GetIsActive()) {
        return nullptr;
    }
    return targetPlayer;
}

void PlayerConfigurationController::UpdateSoloSplitTransition(float progress)
{
    const std::vector<Player*>& players = mWorld.GetPlayers();
    if (players.size() < 2 || !players[0] || !players[1]) {
        mRuntimeState.mSplitMergeTransition = {};
        return;
    }

    Player& mainPlayer = *players[0];
    Player& splitPlayer = *players[1];
    const float easedProgress = CalculateSmoothstep(progress);
    const glm::vec3 upDirection = CalculateUpDirection(mainPlayer);
    const glm::vec3 splitDirection = mRuntimeState.mSplitMergeTransition.splitDirection;
    const float arcHeight = std::sin(glm::pi<float>() * progress) * splitPlayerArcHeightWorldUnits;
    splitPlayer.SetPos(CalculateSplitSurfacePosition(mainPlayer, splitDirection, easedProgress) +
                       upDirection * arcHeight);

    const glm::vec3 finalSplitScale = splitPlayer.GetBaseScale() * Player::SplitBodyScaleMultiplier;
    splitPlayer.SetScale(
        glm::mix(mRuntimeState.mSplitMergeTransition.splitPlayerStartScale, finalSplitScale, easedProgress));
    const glm::vec3 finalMainPlayerScale = mainPlayer.GetBaseScale() * Player::SplitBodyScaleMultiplier;
    mainPlayer.SetScale(
        glm::mix(mRuntimeState.mSplitMergeTransition.mainPlayerStartScale, finalMainPlayerScale, easedProgress));
}

void PlayerConfigurationController::UpdateSoloMergeTransition(float progress)
{
    const std::vector<Player*>& players = mWorld.GetPlayers();
    if (players.size() < 2 || !players[0] || !players[1]) {
        mRuntimeState.mSplitMergeTransition = {};
        return;
    }

    Player& mainPlayer = *players[0];
    Player& splitPlayer = *players[1];
    const float easedProgress = CalculateSmoothstep(progress);
    const glm::vec3 upDirection = CalculateUpDirection(mainPlayer);
    const float arcHeight = std::sin(glm::pi<float>() * progress) * splitPlayerArcHeightWorldUnits * 0.5f;
    splitPlayer.SetPos(
        glm::mix(mRuntimeState.mSplitMergeTransition.splitPlayerStartPosition, mainPlayer.GetPos(), easedProgress) +
        upDirection * arcHeight);

    const glm::vec3 hiddenSplitScale =
        splitPlayer.GetBaseScale() * Player::SplitBodyScaleMultiplier * splitPlayerInitialScaleMultiplier;
    splitPlayer.SetScale(
        glm::mix(mRuntimeState.mSplitMergeTransition.splitPlayerStartScale, hiddenSplitScale, easedProgress));
    mainPlayer.SetScale(
        glm::mix(mRuntimeState.mSplitMergeTransition.mainPlayerStartScale, mainPlayer.GetBaseScale(), easedProgress));
}

void PlayerConfigurationController::CompleteSoloSplitTransition()
{
    const std::vector<Player*>& players = mWorld.GetPlayers();
    if (players.size() < 2 || !players[0] || !players[1]) {
        mRuntimeState.mSplitMergeTransition = {};
        return;
    }

    players[1]->SetShouldJudgeLanding(true);
    mRuntimeState.mSplitMergeTransition = {};
    SelectControlledPlayer(1, ControlledPlayerCameraTransition::Smooth);
    mSceneSystem.OnPlayerSplitMergeSucceeded();
}

void PlayerConfigurationController::CompleteSoloMergeTransition()
{
    mRuntimeState.mSplitMergeTransition = {};
    if (MergePlayerInto(0, ControlledPlayerCameraTransition::Smooth)) {
        mSceneSystem.OnPlayerSplitMergeSucceeded();
    }
}

bool PlayerConfigurationController::AreSplitPlayersCloseEnoughToMerge() const
{
    return PlayerSplitService::ArePlayersCloseEnoughToMerge(mWorld.GetPlayers(), playerMergeMaximumDistanceWorldUnits);
}

bool PlayerConfigurationController::MergePlayerInto(int targetPlayerIndex,
                                                    ControlledPlayerCameraTransition cameraTransition)
{
    mRuntimeState.mIsMergeGuideRequested = false;
    if (!PlayerSplitService::MergeIntoMainPlayer(mWorld.GetPlayers(), targetPlayerIndex)) {
        return false;
    }

    const std::optional<ControlledPlayerChange> selectionChange = mRuntimeState.mControlState.EndPlayerSplit();
    if (selectionChange) {
        if (cameraTransition == ControlledPlayerCameraTransition::Smooth) {
            mCameraSystem.TransitionToControlledPlayer(selectionChange->previousPlayerIndex,
                                                       selectionChange->currentPlayerIndex);
        } else {
            mCameraSystem.SnapToControlledPlayer(selectionChange->previousPlayerIndex,
                                                 selectionChange->currentPlayerIndex);
        }
    }
    return true;
}

void PlayerConfigurationController::SelectControlledPlayer(int playerIndex,
                                                           ControlledPlayerCameraTransition cameraTransition)
{
    const std::optional<ControlledPlayerChange> selectionChange =
        mRuntimeState.mControlState.SelectControlledPlayer(playerIndex);
    if (selectionChange) {
        if (cameraTransition == ControlledPlayerCameraTransition::Smooth) {
            mCameraSystem.TransitionToControlledPlayer(selectionChange->previousPlayerIndex,
                                                       selectionChange->currentPlayerIndex);
        } else {
            mCameraSystem.SnapToControlledPlayer(selectionChange->previousPlayerIndex,
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
    const int previousIndex = mRuntimeState.mControlState.GetControlledPlayerIndex();
    int nextIndex = previousIndex;
    for (int offset = 1; offset <= static_cast<int>(players.size()); ++offset) {
        const int candidateIndex = (previousIndex + offset) % static_cast<int>(players.size());
        Player* candidatePlayer = players[static_cast<std::size_t>(candidateIndex)];
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

    for (int candidateIndex = 0; candidateIndex < static_cast<int>(players.size()); ++candidateIndex) {
        if (candidateIndex == previousIndex) {
            continue;
        }

        Player* waitingPlayer = players[static_cast<std::size_t>(candidateIndex)];
        if (!waitingPlayer || !waitingPlayer->CancelWaitingBoatRide()) {
            continue;
        }

        mRuntimeState.mControlState.CancelScheduledControlSwitch();
        SelectControlledPlayer(candidateIndex);
        mSceneSystem.OnPlayerSwitchSucceeded();
        return true;
    }

    return false;
}

bool PlayerConfigurationController::CanSwitchControlledPlayer() const
{
    const bool allowsPlayerSwitch = mSceneSystem.IsPlaying() || mSceneSystem.IsWaitingForTutorialPlayerSwitch();
    if (!mRuntimeState.mControlState.IsPlayerSplit() || !CanChangeSoloConfiguration() ||
        mWorld.GetPlayers().size() < 2 || !allowsPlayerSwitch) {
        return false;
    }

    const std::vector<Player*>& players = mWorld.GetPlayers();
    for (int index = 0; index < static_cast<int>(players.size()); ++index) {
        if (index != mRuntimeState.mControlState.GetControlledPlayerIndex() &&
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
    if (!mRuntimeState.mIsSecondPlayerJoined && mRuntimeState.mControlState.IsPlayerSplit()) {
        mRuntimeState.mControlState.ScheduleControlSwitchAfterBoarding(boardingControlSwitchDelaySeconds);
    }
}

void PlayerConfigurationController::UpdatePendingControlSwitch(float deltaTime)
{
    const std::vector<Player*>& players = mWorld.GetPlayers();
    const int controlledPlayerIndex = mRuntimeState.mControlState.GetControlledPlayerIndex();
    const bool isControlledPlayerActive = controlledPlayerIndex >= 0 &&
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
        mRuntimeState.mControlState.UpdateScheduledControlSwitch(deltaTime, mRuntimeState.mIsSecondPlayerJoined,
                                                                 activePlayerIndex);
    if (selectionChange) {
        mCameraSystem.SnapToControlledPlayer(selectionChange->previousPlayerIndex, selectionChange->currentPlayerIndex);
    }
}

void PlayerConfigurationController::MergeSoloSplitAfterBoatArrival()
{
    if (!mRuntimeState.mIsSecondPlayerJoined && mRuntimeState.mControlState.IsPlayerSplit()) {
        MergePlayerInto(mRuntimeState.mControlState.GetControlledPlayerIndex());
    }
}

void PlayerConfigurationController::MergeSoloSplitBeforeRestart()
{
    if (!mRuntimeState.mIsSecondPlayerJoined && mRuntimeState.mControlState.IsPlayerSplit()) {
        MergePlayerInto(0);
    }
}

void PlayerConfigurationController::SynchronizeSoloSplitResources(const Player& sourcePlayer)
{
    if (mRuntimeState.mIsSecondPlayerJoined || !mRuntimeState.mControlState.IsPlayerSplit()) {
        return;
    }

    PlayerSplitService::SynchronizeSharedResources(mWorld.GetPlayers(), sourcePlayer);
}

bool PlayerConfigurationController::IsSecondPlayerJoined() const
{
    return mRuntimeState.mIsSecondPlayerJoined;
}

bool PlayerConfigurationController::IsPlayerSplit() const
{
    return mRuntimeState.mControlState.IsPlayerSplit();
}

int PlayerConfigurationController::GetControlledPlayerIndex() const
{
    return mRuntimeState.mControlState.GetControlledPlayerIndex();
}

Player* PlayerConfigurationController::GetMainPlayer() const
{
    if (!mRuntimeState.mIsSecondPlayerJoined) {
        if (Player* controlledPlayer = GetControlledPlayer()) {
            return controlledPlayer;
        }
    }

    return mWorld.GetMainPlayer();
}

Player* PlayerConfigurationController::GetControlledPlayer() const
{
    const std::vector<Player*>& players = mWorld.GetPlayers();
    const int controlledPlayerIndex = mRuntimeState.mControlState.GetControlledPlayerIndex();
    if (controlledPlayerIndex < 0 || controlledPlayerIndex >= static_cast<int>(players.size())) {
        return players.empty() ? nullptr : players[0];
    }

    return players[static_cast<std::size_t>(controlledPlayerIndex)];
}
