#include "system/CameraSystem.h"
#include "system/camera/CameraProjection.h"

#include "actor/Boat.h"
#include "actor/Enemy.h"
#include "actor/Key.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Player.h"
#include "actor/Star.h"
#include "component/FocusComponent.h"
#include "system/InputSystem.h"
#include "system/SceneSystem.h"
#include "system/scene/TutorialController.h"
#include "system/tutorial/TutorialLibrary.h"
#include "world/GameWorld.h"
#include "world/Stage.h"

#include <GLFW/glfw3.h>
#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <utility>

CameraSystem::CameraSystem(CameraSystemDependencies dependencies)
    : mWorld(dependencies.world),
      mInput(dependencies.input),
      mScene(dependencies.scene),
      mReadControlState(std::move(dependencies.readControlState)),
      mFindPlacedActor(std::move(dependencies.findPlacedActor)),
      mBossDefeatSequence(dependencies.audio),
      mCollisionResolver(std::move(dependencies.physicsProvider)),
      mFocusCamera(mCollisionResolver),
      mPlayerCamera(mCollisionResolver),
      mPlayerCameraSettingsRepository("../assets/data/camera/player.yaml"),
      mCinematicLibrary("../assets/data/camera/cinematics.yaml")
{
    mPlayerCameraSettingsRepository.Load(mPlayerCameraSettings);
    mCinematicLibrary.Load();
}

void CameraSystem::ProcessInput()
{
    const CameraControlState controls = mReadControlState();
    if (mCinematicCamera.IsActive()) {
        return;
    }

    if (controls.isEditorKeyboardInputCaptured) {
        mCameraStickX = 0.0f;
        mCameraStickY = 0.0f;
        mSecondControllerStickX = 0.0f;
        mSecondControllerStickY = 0.0f;
        mKeyboardYawInput = 0.0f;
        mKeyboardPitchInput = 0.0f;
        mWasAlignCameraPressed = true;
        mWasSecondControllerAlignCameraPressed = true;
        if (controls.isFreeCameraMode) {
            mDebugCamera.ProcessInput(mInput, controls.isUGCMode, controls.isEditorKeyboardInputCaptured);
        }
        return;
    }

    if (controls.isFreeCameraMode) {
        mDebugCamera.ProcessInput(mInput, controls.isUGCMode, controls.isEditorKeyboardInputCaptured);
        return;
    }

    SceneSystem& sceneSystem = mScene;
    const InputSystem& inputSystem = mInput;
    const bool hasPrimaryController = inputSystem.HasControllerInput(1);
    const bool hasSecondController = inputSystem.HasControllerInput(2);

    constexpr float deadZone = 0.25f;
    constexpr float scale = 1.0f / 32767.0f;

    mCameraStickX = hasPrimaryController ? inputSystem.GetControllerAxis(1, SDL_CONTROLLER_AXIS_RIGHTX) * scale : 0.0f;
    mCameraStickY = hasPrimaryController ? inputSystem.GetControllerAxis(1, SDL_CONTROLLER_AXIS_RIGHTY) * scale : 0.0f;

    if (std::abs(mCameraStickX) < deadZone) {
        mCameraStickX = 0.0f;
    }
    if (std::abs(mCameraStickY) < deadZone) {
        mCameraStickY = 0.0f;
    }
    mSecondControllerStickX =
        hasSecondController ? inputSystem.GetControllerAxis(2, SDL_CONTROLLER_AXIS_RIGHTX) * scale : 0.0f;
    mSecondControllerStickY =
        hasSecondController ? inputSystem.GetControllerAxis(2, SDL_CONTROLLER_AXIS_RIGHTY) * scale : 0.0f;
    if (std::abs(mSecondControllerStickX) < deadZone) {
        mSecondControllerStickX = 0.0f;
    }
    if (std::abs(mSecondControllerStickY) < deadZone) {
        mSecondControllerStickY = 0.0f;
    }

    mKeyboardYawInput = 0.0f;
    mKeyboardPitchInput = 0.0f;
    if (inputSystem.IsKeyPressed(GLFW_KEY_RIGHT)) {
        mKeyboardYawInput += 1.0f;
    }
    if (inputSystem.IsKeyPressed(GLFW_KEY_LEFT)) {
        mKeyboardYawInput -= 1.0f;
    }
    if (inputSystem.IsKeyPressed(GLFW_KEY_UP)) {
        mKeyboardPitchInput += 1.0f;
    }
    if (inputSystem.IsKeyPressed(GLFW_KEY_DOWN)) {
        mKeyboardPitchInput -= 1.0f;
    }

    const bool keyboardAlignCameraPressed = inputSystem.IsKeyPressed(GLFW_KEY_M);
    const bool controllerAlignCameraPressed =
        inputSystem.IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
    const bool secondControllerAlignCameraPressed =
        inputSystem.IsControllerButtonPressed(2, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
    const bool playerTwoUsesController = controls.isPlayer2Joined && controls.hasSecondController;
    const bool alignCameraPressed =
        controllerAlignCameraPressed || (keyboardAlignCameraPressed && !playerTwoUsesController);

    if (sceneSystem.IsPlaying() && alignCameraPressed && !mWasAlignCameraPressed) {
        int playerIndex = GetPrimaryPlayerIndex();
        if (controls.isPlayer2Joined) {
            playerIndex = (keyboardAlignCameraPressed && !playerTwoUsesController) ? 1 : 0;
        }
        const std::vector<Player*>& players = mWorld.GetPlayers();
        if (playerIndex >= 0 && playerIndex < static_cast<int>(players.size()) &&
            players[static_cast<std::size_t>(playerIndex)]) {
            if (!mCameraZones.HasActiveZone(playerIndex)) {
                mCameraZones.RequestReset(playerIndex);
                mPlayerCamera.AlignBehindPlayer(players[static_cast<std::size_t>(playerIndex)], playerIndex);
            }
        }
    }

    mWasAlignCameraPressed = alignCameraPressed;
    if (sceneSystem.IsPlaying() && secondControllerAlignCameraPressed && !mWasSecondControllerAlignCameraPressed &&
        playerTwoUsesController) {
        const std::vector<Player*>& players = mWorld.GetPlayers();
        if (players.size() >= 2 && players[1]) {
            if (!mCameraZones.HasActiveZone(1)) {
                mCameraZones.RequestReset(1);
                mPlayerCamera.AlignBehindPlayer(players[1], 1);
            }
        }
    }
    mWasSecondControllerAlignCameraPressed = secondControllerAlignCameraPressed;
}

void CameraSystem::Update(float deltaTime)
{
    UpdateShakeEffects(deltaTime);
    UpdateCamera(deltaTime);
}

void CameraSystem::UpdateShakeEffects(float deltaTime)
{
    for (CameraShakeEffect& shakeEffect : mPlayerShakeEffects) {
        shakeEffect.Update(deltaTime);
    }
}

void CameraSystem::StartAirStrongAttackHitShake(int playerNum)
{
    const int playerIndex = playerNum - 1;
    if (playerIndex < 0) {
        return;
    }

    const std::size_t shakeIndex = static_cast<std::size_t>(playerIndex);
    if (mPlayerShakeEffects.size() <= shakeIndex) {
        mPlayerShakeEffects.resize(shakeIndex + 1);
    }
    mPlayerShakeEffects[shakeIndex].TryStart(CameraShakePattern::AirStrongAttackHit);
}

void CameraSystem::StartPlayerDamagedShake(int playerNum)
{
    const int playerIndex = playerNum - 1;
    if (playerIndex < 0) {
        return;
    }

    const std::size_t shakeIndex = static_cast<std::size_t>(playerIndex);
    if (mPlayerShakeEffects.size() <= shakeIndex) {
        mPlayerShakeEffects.resize(shakeIndex + 1);
    }
    mPlayerShakeEffects[shakeIndex].TryStart(CameraShakePattern::PlayerDamaged);
}

bool CameraSystem::PlayCinematic(std::string_view sequenceId, bool shouldHoldFinalPose)
{
    const CinematicSequence* sequence = mCinematicLibrary.Find(sequenceId);
    return sequence && mCinematicCamera.Play(*sequence, shouldHoldFinalPose);
}

void CameraSystem::StopCinematic()
{
    mCinematicCamera.Stop();
}

bool CameraSystem::ReloadCinematicSequences()
{
    StopCinematic();
    return mCinematicLibrary.Load();
}

void CameraSystem::SetPlayerCameraSettings(PlayerCameraSettings settings)
{
    settings.Normalize();
    mPlayerCameraSettings = settings;
}

bool CameraSystem::SavePlayerCameraSettings() const
{
    return mPlayerCameraSettingsRepository.Save(mPlayerCameraSettings);
}

bool CameraSystem::ReloadPlayerCameraSettings()
{
    PlayerCameraSettings settings;
    if (!mPlayerCameraSettingsRepository.Load(settings)) {
        return false;
    }

    mPlayerCameraSettings = settings;
    return true;
}

void CameraSystem::ResetForStageChange()
{
    StopCinematic();
    StopBossDefeatSequence();

    mIsTargetFocus = false;
    mIsShowingRevealFocus = false;
    mRevealFocusActors.clear();
    mRevealFocusView = glm::mat4(1.0f);
    mWasAlignCameraPressed = false;
    mWasSecondControllerAlignCameraPressed = false;
    mCameraStickX = 0.0f;
    mCameraStickY = 0.0f;
    mSecondControllerStickX = 0.0f;
    mSecondControllerStickY = 0.0f;
    mKeyboardYawInput = 0.0f;
    mKeyboardPitchInput = 0.0f;
    mPlayerPitchOffsetsDegrees.clear();
    mAirSlamCameraStates.clear();
    mPlayerShakeEffects.clear();
    mPlayerCamera.Reset();
    mCameraZones.Reset(mWorld.GetPlayers());
    mCameraZonePreviewIndex = -1;
    mTalkCameraBlend = 0.0f;
    mTalkCameraPlayer = nullptr;
    mHasTalkCameraTarget = false;
    mTalkCameraTargetPos = glm::vec3(0.0f);
    mTalkPageFocusActor = nullptr;
    mTalkPageFocusBlend = 0.0f;
    mHasTalkPageFocusPose = false;
    mTalkPageFocusCameraPos = glm::vec3(0.0f);
    mTalkPageFocusTargetPos = glm::vec3(0.0f);
    mTalkPageFocusUpVec = glm::vec3(0.0f, 1.0f, 0.0f);
    mRenderedTalkPageCameraPos = glm::vec3(0.0f);
    mBoatRideCameraTarget = nullptr;
}

void CameraSystem::SnapBehindControlledPlayer()
{
    const CameraControlState controls = mReadControlState();
    Player* controlledPlayer = controls.controlledPlayer;
    if (!controlledPlayer) {
        return;
    }

    mPlayerCamera.SnapBehindPlayer(controlledPlayer, GetPrimaryPlayerIndex());
}

std::optional<CameraPose> CameraSystem::GetCurrentCameraPose()
{
    const auto controls = mReadControlState();
    if (controls.isFreeCameraMode) {
        return GetDebugCameraPose();
    }
    const auto views = GetViews();
    if (views.empty()) {
        return std::nullopt;
    }
    const int playerIndex = controls.controlledPlayerIndex;
    const std::size_t viewIndex = controls.isPlayer2Joined && views.size() > 1 && playerIndex >= 0 &&
                                          static_cast<std::size_t>(playerIndex) < views.size()
                                      ? static_cast<std::size_t>(playerIndex)
                                      : 0;
    const glm::mat4 inverseView = glm::inverse(views[viewIndex]);
    CameraPose pose;
    pose.position = glm::vec3(inverseView[3]);
    pose.target = pose.position - glm::vec3(inverseView[2]);
    pose.up = glm::vec3(inverseView[1]);
    pose.fieldOfViewDegrees = GetPlayerFieldOfViewDegrees(playerIndex);
    return pose;
}

float CameraSystem::GetFieldOfViewDegrees() const
{
    const CameraControlState controls = mReadControlState();
    if (mCinematicCamera.IsActive()) {
        return glm::clamp(mCinematicCamera.GetPose().fieldOfViewDegrees, 10.0f, 120.0f);
    }

    if (controls.isFreeCameraMode) {
        return mDebugCamera.GetFieldOfViewDegrees();
    }

    if (mBossDefeatSequence.IsActive()) {
        return mPlayerCameraSettings.bossDefeatFieldOfViewDegrees;
    }

    if (ResolveBoatRideCameraTarget()) {
        return mPlayerCameraSettings.boatRideFieldOfViewDegrees;
    }

    if (mScene.IsPlaying() && !controls.isUGCMode && !mIsTargetFocus && !mIsShowingRevealFocus &&
        mTalkCameraBlend <= 0.0f && mTalkPageFocusBlend <= 0.0f) {
        if (const auto pose = mCameraZones.GetPose(GetPrimaryPlayerIndex())) {
            return pose->fieldOfViewDegrees;
        }
    }

    const float normalFieldOfView = controls.isPlayer2Joined ? mPlayerCameraSettings.splitScreenFieldOfViewDegrees
                                                             : mPlayerCameraSettings.fieldOfViewDegrees;

    return glm::mix(normalFieldOfView, mPlayerCameraSettings.talkFieldOfViewDegrees, GetEasedTalkCameraBlend());
}

void CameraSystem::UpdateCamera(float deltaTime)
{
    ++mCameraFrameNumber;
    mCameraDeltaTime = std::max(0.0f, deltaTime);
    const CameraControlState controls = mReadControlState();
    UpdateTalkCameraTransition(deltaTime);
    mBossDefeatSequence.Update(deltaTime);
    UpdateAirSlamCameraStates(deltaTime);
    if ((!controls.isDebugEditorShowing || controls.isFreeCameraMode) && mCameraZonePreviewIndex >= 0) {
        SetCameraZonePreview(-1);
    }

    if (!mScene.IsPlaying() || controls.isFreeCameraMode || controls.isUGCMode || mCinematicCamera.IsActive() ||
        mBossDefeatSequence.IsActive() || mTalkCameraBlend > 0.0f || mTalkPageFocusBlend > 0.0f || mIsTargetFocus ||
        mIsShowingRevealFocus || ResolveBoatRideCameraTarget()) {
        // スイッチの注目演出はFocusingへ遷移する。非操作側を含め、復帰後の行き・帰りを保持する。
        const bool preservesDirection =
            (mScene.IsPlaying() || mScene.IsFocusing()) && !controls.isFreeCameraMode && !controls.isUGCMode;
        mCameraZones.Reset(mWorld.GetPlayers(), preservesDirection);
    }

    if (mCinematicCamera.IsActive()) {
        mCinematicCamera.Update(deltaTime);
        return;
    }

    if (controls.isFreeCameraMode) {
        mDebugCamera.Update(deltaTime, controls.isUGCMode);
        return;
    }

    UpdatePlayerPitchOffsets(deltaTime);

    const std::vector<Player*>& players = mWorld.GetPlayers();
    std::vector<float> yawDeltas(players.size(), 0.0f);
    const int primaryPlayerIndex = GetPrimaryPlayerIndex();
    const bool allowsManualYaw = mTalkCameraBlend <= 0.0f;
    if (allowsManualYaw) {
        const float yawSpeedRadiansPerSecond = mPlayerCameraSettings.yawSensitivity;
        const float safeDeltaTime = std::max(0.0f, deltaTime);

        if (controls.isPlayer2Joined) {
            if (!players.empty() && players[0] && controls.hasPrimaryController) {
                yawDeltas[0] = mCameraStickX * yawSpeedRadiansPerSecond * safeDeltaTime;
            }

            if (controls.hasSecondController) {
                if (yawDeltas.size() >= 2 && players[1]) {
                    yawDeltas[1] = mSecondControllerStickX * yawSpeedRadiansPerSecond * safeDeltaTime;
                }
            } else if (controls.hasPrimaryController) {
                if (yawDeltas.size() >= 2 && players[1]) {
                    yawDeltas[1] = mKeyboardYawInput * yawSpeedRadiansPerSecond * safeDeltaTime;
                }
            } else if (!players.empty() && players[0]) {
                yawDeltas[0] = mKeyboardYawInput * yawSpeedRadiansPerSecond * safeDeltaTime;
            }
        } else if (primaryPlayerIndex >= 0 && primaryPlayerIndex < static_cast<int>(yawDeltas.size())) {
            const float yawInput = controls.hasPrimaryController ? mCameraStickX : mKeyboardYawInput;
            yawDeltas[static_cast<std::size_t>(primaryPlayerIndex)] =
                yawInput * yawSpeedRadiansPerSecond * safeDeltaTime;
        }
    }

    SceneSystem& sceneSystem = mScene;
    const bool allowsMovementCameraAssist =
        sceneSystem.IsPlaying() && mTalkCameraBlend <= 0.0f && !mBossDefeatSequence.IsActive() && !mIsTargetFocus;
    for (std::size_t index = 0; index < yawDeltas.size(); ++index) {
        if (!mCameraZones.AllowsManualRotation(static_cast<int>(index))) {
            yawDeltas[index] = 0.0f;
        }
        if (std::abs(yawDeltas[index]) > 0.000001f) {
            mCameraZones.NotifyManualInput(static_cast<int>(index));
        }
    }
    mPlayerCamera.Update(players, yawDeltas, mPlayerCameraSettings, deltaTime, allowsMovementCameraAssist);
    UpdateTalkCameraAim();
    UpdateTalkPageFocus(deltaTime);
}

void CameraSystem::UpdateAirSlamCameraStates(float deltaTime)
{
    const std::vector<Player*>& players = mWorld.GetPlayers();
    mAirSlamCameraStates.resize(players.size());

    const float safeDeltaTime = std::max(0.0f, deltaTime);
    const float smoothingAmount = 1.0f - std::exp(-mPlayerCameraSettings.airSlamDistanceSmoothingSpeed * safeDeltaTime);

    for (std::size_t playerIndex = 0; playerIndex < players.size(); ++playerIndex) {
        AirSlamCameraState& cameraState = mAirSlamCameraStates[playerIndex];
        const Player* player = players[playerIndex];
        const bool isAirSlamAttacking = player && player->GetActionState() == PlayerActionState::AirSlamAttacking;

        if (isAirSlamAttacking) {
            cameraState.isWaitingForLanding = true;
            cameraState.returnDelayRemainingSeconds = 0.0f;
        } else if (cameraState.isWaitingForLanding && player && player->GetOnGround()) {
            cameraState.isWaitingForLanding = false;
            cameraState.returnDelayRemainingSeconds = mPlayerCameraSettings.airSlamReturnDelaySeconds;
        } else if (!cameraState.isWaitingForLanding) {
            cameraState.returnDelayRemainingSeconds =
                std::max(0.0f, cameraState.returnDelayRemainingSeconds - safeDeltaTime);
        }

        const bool shouldKeepPulledBack =
            isAirSlamAttacking || cameraState.isWaitingForLanding || cameraState.returnDelayRemainingSeconds > 0.0f;
        const float targetBlend = shouldKeepPulledBack ? 1.0f : 0.0f;
        cameraState.distanceBlend = glm::mix(cameraState.distanceBlend, targetBlend, smoothingAmount);

        const float targetPitchBlend = isAirSlamAttacking ? 1.0f : 0.0f;
        cameraState.pitchBlend = glm::mix(cameraState.pitchBlend, targetPitchBlend, smoothingAmount);

        if (std::abs(cameraState.distanceBlend - targetBlend) < 0.001f) {
            cameraState.distanceBlend = targetBlend;
        }
        if (std::abs(cameraState.pitchBlend - targetPitchBlend) < 0.001f) {
            cameraState.pitchBlend = targetPitchBlend;
        }
    }
}

void CameraSystem::UpdatePlayerPitchOffsets(float deltaTime)
{
    const CameraControlState controls = mReadControlState();
    const std::vector<Player*>& players = mWorld.GetPlayers();
    if (mPlayerPitchOffsetsDegrees.size() < players.size()) {
        mPlayerPitchOffsetsDegrees.resize(players.size(), 0.0f);
    }

    const std::vector<float> previousPitchOffsets = mPlayerPitchOffsetsDegrees;

    const bool allowsManualPitch = mTalkCameraBlend <= 0.0f && !mBossDefeatSequence.IsActive() && !mIsTargetFocus;

    if (allowsManualPitch) {
        const float pitchSpeed = mPlayerCameraSettings.pitchSensitivityDegrees * std::max(0.0f, deltaTime);

        if (controls.isPlayer2Joined) {
            if (!players.empty() && players[0] && controls.hasPrimaryController) {
                mPlayerPitchOffsetsDegrees[0] += -mCameraStickY * pitchSpeed;
            }

            int keyboardPlayerIndex = -1;
            if (controls.hasSecondController) {
                if (players.size() >= 2 && players[1]) {
                    mPlayerPitchOffsetsDegrees[1] += -mSecondControllerStickY * pitchSpeed;
                }
            } else if (controls.hasPrimaryController) {
                if (players.size() >= 2 && players[1]) {
                    keyboardPlayerIndex = 1;
                }
            } else if (!players.empty() && players[0]) {
                keyboardPlayerIndex = 0;
            }

            if (keyboardPlayerIndex >= 0) {
                mPlayerPitchOffsetsDegrees[static_cast<std::size_t>(keyboardPlayerIndex)] +=
                    mKeyboardPitchInput * pitchSpeed;
            }
        } else {
            const int controlledIndex = GetPrimaryPlayerIndex();
            if (controlledIndex >= 0 && controlledIndex < static_cast<int>(players.size()) &&
                players[static_cast<std::size_t>(controlledIndex)]) {
                if (controls.hasPrimaryController) {
                    mPlayerPitchOffsetsDegrees[static_cast<std::size_t>(controlledIndex)] +=
                        -mCameraStickY * pitchSpeed;
                } else {
                    mPlayerPitchOffsetsDegrees[static_cast<std::size_t>(controlledIndex)] +=
                        mKeyboardPitchInput * pitchSpeed;
                }
            }
        }
    }

    for (std::size_t index = 0; index < players.size(); ++index) {
        if (!mCameraZones.AllowsManualRotation(static_cast<int>(index))) {
            mPlayerPitchOffsetsDegrees[index] = previousPitchOffsets[index];
            continue;
        }
        if (mPlayerPitchOffsetsDegrees[index] != previousPitchOffsets[index]) {
            mPlayerCamera.NotifyManualInput(players[index], static_cast<int>(index), mPlayerCameraSettings);
            mCameraZones.NotifyManualInput(static_cast<int>(index));
        }
    }

    for (float& pitchOffset : mPlayerPitchOffsetsDegrees) {
        const float pitch = glm::clamp(mPlayerCameraSettings.pitchDegrees + pitchOffset,
                                       mPlayerCameraSettings.minPitchDegrees, mPlayerCameraSettings.maxPitchDegrees);
        pitchOffset = pitch - mPlayerCameraSettings.pitchDegrees;
    }
}

bool CameraSystem::AllowsPlayerInput() const
{
    const CameraControlState controls = mReadControlState();
    return !mBossDefeatSequence.IsActive() && !mCinematicCamera.IsActive() && !controls.isFreeCameraMode &&
           !mIsTargetFocus && !FindMovingBoat();
}

void CameraSystem::StartBossDefeatSequence(Enemy* boss, Star* star)
{
    BeginBossDefeatSequence(boss, star, false);
}

void CameraSystem::BeginBossDefeatSequence(Enemy* boss, Star* star, bool isPreview)
{
    const CameraControlState controls = mReadControlState();
    if (!boss) {
        return;
    }

    mBossDefeatSequence.Start(boss, star, isPreview);
    mCameraStickX = 0.0f;

    Player* player = controls.mainPlayer;
    const glm::vec3 targetPos = player ? player->GetPos() : boss->GetPos();
    const glm::vec3 up = player ? player->GetUpVec() : boss->GetUpVec();
    mFocusCamera.BeginTransition(mPlayerCamera.GetCameraPos(GetPrimaryPlayerIndex()), targetPos, up);
}

bool CameraSystem::PreviewBossDefeatSequence()
{
    const CameraControlState controls = mReadControlState();
    Player* player = controls.mainPlayer;
    Planet* planet = player ? player->GetCurrentPlanet() : nullptr;
    Enemy* boss = FindBossEnemy(planet);
    if (!boss) {
        return false;
    }

    BeginBossDefeatSequence(boss, planet->GetStar(), true);
    return true;
}

void CameraSystem::StopBossDefeatSequence()
{
    mBossDefeatSequence.Stop();
}

void CameraSystem::UpdateTalkCameraTransition(float deltaTime)
{
    const CameraControlState controls = mReadControlState();
    SceneSystem& sceneSystem = mScene;

    Player* targetPlayer = nullptr;
    NPC* targetNPC = nullptr;
    if (sceneSystem.IsTalkWithNPC() && sceneSystem.GetTalkingPlayer() && sceneSystem.GetTalkingNPC() &&
        sceneSystem.GetTalkingNPC()->ShouldUseTalkCamera()) {
        targetPlayer = sceneSystem.GetTalkingPlayer();
        targetNPC = sceneSystem.GetTalkingNPC();
    } else if (mIsTalkCameraPreviewEnabled) {
        targetPlayer = controls.mainPlayer;
    }

    if (targetPlayer) {
        if (mTalkCameraPlayer && mTalkCameraPlayer != targetPlayer) {
            mTalkCameraBlend = 0.0f;
        }

        mTalkCameraPlayer = targetPlayer;
        if (targetNPC) {
            mTalkCameraTargetPos = targetNPC->GetPos();
            mHasTalkCameraTarget = true;
        }

        const float duration = mPlayerCameraSettings.talkTransitionInDuration;
        mTalkCameraBlend =
            duration <= 0.0f ? 1.0f : std::min(1.0f, mTalkCameraBlend + std::max(0.0f, deltaTime) / duration);
        return;
    }

    const float duration = mPlayerCameraSettings.talkTransitionOutDuration;
    mTalkCameraBlend =
        duration <= 0.0f ? 0.0f : std::max(0.0f, mTalkCameraBlend - std::max(0.0f, deltaTime) / duration);

    if (mTalkCameraBlend <= 0.0f) {
        mTalkCameraPlayer = nullptr;
        mHasTalkCameraTarget = false;
    }
}

void CameraSystem::UpdateTalkCameraAim()
{
    if (!mTalkCameraPlayer || !mHasTalkCameraTarget || mTalkCameraBlend <= 0.0f) {
        return;
    }

    const std::vector<Player*>& players = mWorld.GetPlayers();
    for (int playerIndex = 0; playerIndex < static_cast<int>(players.size()); ++playerIndex) {
        if (players[playerIndex] != mTalkCameraPlayer) {
            continue;
        }

        mPlayerCamera.BlendBehindTarget(mTalkCameraPlayer, playerIndex, mTalkCameraTargetPos,
                                        GetEasedTalkCameraBlend());
        return;
    }
}

void CameraSystem::InheritSplitPlayerCameraDirection(int fromPlayerIndex, int toPlayerIndex)
{
    mCameraZones.InheritSplitDirection(fromPlayerIndex, toPlayerIndex, mWorld.GetPlayers());
}

void CameraSystem::SnapToControlledPlayer(int fromPlayerIndex, int toPlayerIndex)
{
    if (toPlayerIndex < 0) {
        return;
    }

    const std::vector<Player*>& players = mWorld.GetPlayers();
    if (toPlayerIndex >= static_cast<int>(players.size()) || !players[static_cast<std::size_t>(toPlayerIndex)]) {
        return;
    }

    mCameraZones.TransitionToPlayer(fromPlayerIndex, toPlayerIndex, players, true);
    mPlayerCamera.SnapToPlayer(players[static_cast<std::size_t>(toPlayerIndex)], toPlayerIndex);

    CopyPlayerPitchOffset(fromPlayerIndex, toPlayerIndex);
}

void CameraSystem::TransitionToControlledPlayer(int fromPlayerIndex, int toPlayerIndex)
{
    if (toPlayerIndex < 0) {
        return;
    }

    const std::vector<Player*>& players = mWorld.GetPlayers();
    if (toPlayerIndex >= static_cast<int>(players.size()) || !players[static_cast<std::size_t>(toPlayerIndex)]) {
        return;
    }

    mCameraZones.TransitionToPlayer(fromPlayerIndex, toPlayerIndex, players);
    mPlayerCamera.TransitionToPlayer(fromPlayerIndex, players[static_cast<std::size_t>(toPlayerIndex)], toPlayerIndex);

    CopyPlayerPitchOffset(fromPlayerIndex, toPlayerIndex);
}

void CameraSystem::CopyPlayerPitchOffset(int fromPlayerIndex, int toPlayerIndex)
{
    if (toPlayerIndex < 0) {
        return;
    }

    const int requiredPitchCount = std::max(fromPlayerIndex, toPlayerIndex) + 1;
    if (requiredPitchCount <= 0) {
        return;
    }

    if (mPlayerPitchOffsetsDegrees.size() < static_cast<std::size_t>(requiredPitchCount)) {
        mPlayerPitchOffsetsDegrees.resize(static_cast<std::size_t>(requiredPitchCount), 0.0f);
    }

    if (fromPlayerIndex >= 0) {
        mPlayerPitchOffsetsDegrees[static_cast<std::size_t>(toPlayerIndex)] =
            mPlayerPitchOffsetsDegrees[static_cast<std::size_t>(fromPlayerIndex)];
    }
}

void CameraSystem::UpdateTalkPageFocus(float deltaTime)
{
    Actor* desiredFocusActor = ResolveTalkPageFocusActor();
    if (desiredFocusActor && !desiredFocusActor->GetIsActive()) {
        desiredFocusActor = nullptr;
    }

    SceneSystem& sceneSystem = mScene;
    Player* talkingPlayer = sceneSystem.GetTalkingPlayer();

    const float safeDeltaTime = std::max(0.0f, deltaTime);
    if (desiredFocusActor) {
        if (!mHasTalkPageFocusPose) {
            glm::vec3 up = talkingPlayer ? talkingPlayer->GetUpVec() : desiredFocusActor->GetUpVec();
            if (glm::dot(up, up) < 0.000001f) {
                up = glm::vec3(0.0f, 1.0f, 0.0f);
            } else {
                up = glm::normalize(up);
            }

            int talkPlayerIndex = GetPrimaryPlayerIndex();
            const std::vector<Player*>& players = mWorld.GetPlayers();
            for (int playerIndex = 0; playerIndex < static_cast<int>(players.size()); ++playerIndex) {
                if (players[static_cast<std::size_t>(playerIndex)] == talkingPlayer) {
                    talkPlayerIndex = playerIndex;
                    break;
                }
            }

            mTalkPageFocusCameraPos = mPlayerCamera.GetCameraPos(talkPlayerIndex);
            if (glm::dot(mTalkPageFocusCameraPos, mTalkPageFocusCameraPos) < 0.000001f) {
                mTalkPageFocusCameraPos =
                    (talkingPlayer ? talkingPlayer->GetPos() : desiredFocusActor->GetPos()) + up * 3.0f;
            }
            mTalkPageFocusTargetPos = (talkingPlayer ? talkingPlayer->GetPos() : desiredFocusActor->GetPos()) +
                                      up * mPlayerCameraSettings.talkTargetHeight;
            mTalkPageFocusUpVec = up;
            mHasTalkPageFocusPose = true;
        }

        mTalkPageFocusActor = desiredFocusActor;

        const float duration = mPlayerCameraSettings.talkTransitionInDuration;
        mTalkPageFocusBlend = duration <= 0.0f ? 1.0f : std::min(1.0f, mTalkPageFocusBlend + safeDeltaTime / duration);

        glm::vec3 up = desiredFocusActor->GetUpVec();
        if (glm::dot(up, up) < 0.000001f) {
            up = glm::vec3(0.0f, 1.0f, 0.0f);
        } else {
            up = glm::normalize(up);
        }

        glm::vec3 back =
            (talkingPlayer ? talkingPlayer->GetPos() : desiredFocusActor->GetPos()) - desiredFocusActor->GetPos();
        back -= up * glm::dot(back, up);
        if (glm::dot(back, back) < 0.000001f) {
            back = glm::cross(up, glm::vec3(1.0f, 0.0f, 0.0f));
        }
        if (glm::dot(back, back) < 0.000001f) {
            back = glm::cross(up, glm::vec3(0.0f, 0.0f, 1.0f));
        }
        back = glm::normalize(back);

        const glm::vec3 desiredTargetPos = desiredFocusActor->GetPos() + up * mPlayerCameraSettings.talkTargetHeight;
        const glm::vec3 uncorrectedCameraPos = desiredTargetPos + back * mPlayerCameraSettings.talkDistance +
                                               up * (mPlayerCameraSettings.talkDistance * 0.35f);
        const glm::vec3 desiredCameraPos = mCollisionResolver.Resolve(desiredTargetPos, uncorrectedCameraPos);

        constexpr float focusPoseSmoothingSpeed = 7.0f;
        const float poseBlend = 1.0f - std::exp(-focusPoseSmoothingSpeed * safeDeltaTime);
        mTalkPageFocusCameraPos = glm::mix(mTalkPageFocusCameraPos, desiredCameraPos, poseBlend);
        mTalkPageFocusTargetPos = glm::mix(mTalkPageFocusTargetPos, desiredTargetPos, poseBlend);
        const glm::vec3 blendedUp = glm::mix(mTalkPageFocusUpVec, up, poseBlend);
        if (glm::dot(blendedUp, blendedUp) >= 0.000001f) {
            mTalkPageFocusUpVec = glm::normalize(blendedUp);
        }
        return;
    }

    mTalkPageFocusActor = nullptr;
    const float duration = mPlayerCameraSettings.talkTransitionOutDuration;
    mTalkPageFocusBlend = duration <= 0.0f ? 0.0f : std::max(0.0f, mTalkPageFocusBlend - safeDeltaTime / duration);

    if (mTalkPageFocusBlend <= 0.0f) {
        mHasTalkPageFocusPose = false;
    }
}

float CameraSystem::GetEasedTalkCameraBlend() const
{
    const float blend = glm::clamp(mTalkCameraBlend, 0.0f, 1.0f);
    return blend * blend * (3.0f - 2.0f * blend);
}

float CameraSystem::GetEasedTalkPageFocusBlend() const
{
    const float blend = glm::clamp(mTalkPageFocusBlend, 0.0f, 1.0f);
    return blend * blend * (3.0f - 2.0f * blend);
}

glm::mat4 CameraSystem::GetPlayerCameraView(Player* player, int playerIndex)
{
    const CameraControlState controls = mReadControlState();
    const float talkBlend = player && player == mTalkCameraPlayer ? GetEasedTalkCameraBlend() : 0.0f;
    const float airSlamDistanceBlend =
        playerIndex >= 0 && static_cast<std::size_t>(playerIndex) < mAirSlamCameraStates.size()
            ? mAirSlamCameraStates[static_cast<std::size_t>(playerIndex)].distanceBlend
            : 0.0f;
    const float airSlamDistanceMultiplier =
        glm::mix(1.0f, mPlayerCameraSettings.airSlamDistanceMultiplier, airSlamDistanceBlend);
    const float normalDistance = mPlayerCameraSettings.distance * airSlamDistanceMultiplier;
    const float distance = glm::mix(normalDistance, mPlayerCameraSettings.talkDistance, talkBlend);
    const float pitchOffset =
        playerIndex >= 0 && static_cast<std::size_t>(playerIndex) < mPlayerPitchOffsetsDegrees.size()
            ? mPlayerPitchOffsetsDegrees[static_cast<std::size_t>(playerIndex)]
            : 0.0f;
    const float normalPitchDegrees =
        glm::clamp(mPlayerCameraSettings.pitchDegrees + pitchOffset, mPlayerCameraSettings.minPitchDegrees,
                   mPlayerCameraSettings.maxPitchDegrees);
    const float airSlamPitchBlend =
        playerIndex >= 0 && static_cast<std::size_t>(playerIndex) < mAirSlamCameraStates.size()
            ? mAirSlamCameraStates[static_cast<std::size_t>(playerIndex)].pitchBlend
            : 0.0f;
    const float actionPitchDegrees =
        glm::mix(normalPitchDegrees, mPlayerCameraSettings.airSlamPitchDegrees, airSlamPitchBlend);
    const float pitchDegrees = glm::mix(actionPitchDegrees, mPlayerCameraSettings.talkPitchDegrees, talkBlend);
    const float normalTargetHeight =
        controls.isPlayer2Joined ? mPlayerCameraSettings.splitScreenTargetHeight : mPlayerCameraSettings.targetHeight;
    const float targetHeight = glm::mix(normalTargetHeight, mPlayerCameraSettings.talkTargetHeight, talkBlend);

    const float aspect = mViewportAspectRatio * (controls.isPlayer2Joined ? 2.0f : 1.0f);
    const float normalFov = controls.isPlayer2Joined ? mPlayerCameraSettings.splitScreenFieldOfViewDegrees
                                                     : mPlayerCameraSettings.fieldOfViewDegrees;
    const float clearanceRadius = CalculateCameraClearanceRadius(normalFov, aspect);
    const glm::mat4 view = mPlayerCamera.GetView(player, playerIndex, distance, glm::radians(pitchDegrees),
                                                 targetHeight, false, clearanceRadius);
    if (!player) {
        return ApplyPlayerShake(view, playerIndex);
    }
    const auto inverseView = glm::inverse(view);
    CameraPose normalPose;
    normalPose.position = glm::vec3(inverseView[3]);
    normalPose.target = normalPose.position - glm::vec3(inverseView[2]) * distance;
    normalPose.up = glm::vec3(inverseView[1]);
    normalPose.fieldOfViewDegrees = controls.isPlayer2Joined ? mPlayerCameraSettings.splitScreenFieldOfViewDegrees
                                                             : mPlayerCameraSettings.fieldOfViewDegrees;
    const bool allowsZones = mScene.IsPlaying() && mTalkCameraBlend <= 0.0f && mTalkPageFocusBlend <= 0.0f &&
                             !controls.isFreeCameraMode && !controls.isUGCMode && AllowsPlayerInput();
    const auto pose = mCameraZones.Apply(*player, playerIndex, normalPose, mWorld.GetCurrentStage(), mFindPlacedActor,
                                         mCollisionResolver, aspect, mCameraDeltaTime, mCameraFrameNumber, allowsZones,
                                         controls.isDebugEditorShowing ? mCameraZonePreviewIndex : -1);
    if (!mCameraZones.GetPose(playerIndex)) {
        return ApplyPlayerShake(view, playerIndex);
    }
    mPlayerCamera.AdoptZonePose(playerIndex, pose);
    const glm::vec3 forward = pose.target - pose.position;
    if (glm::dot(forward, forward) < 0.000001f) {
        return ApplyPlayerShake(view, playerIndex);
    }
    glm::vec3 up = pose.up;
    if (glm::length(glm::cross(forward, up)) < 0.0001f) {
        up = std::abs(glm::normalize(forward).y) < 0.9f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
    }
    return ApplyPlayerShake(glm::lookAt(pose.position, pose.target, up), playerIndex);
}

glm::mat4 CameraSystem::ApplyPlayerShake(const glm::mat4& view, int playerIndex) const
{
    if (playerIndex < 0 || static_cast<std::size_t>(playerIndex) >= mPlayerShakeEffects.size()) {
        return view;
    }

    const glm::vec2 localOffset = mPlayerShakeEffects[static_cast<std::size_t>(playerIndex)].GetLocalOffset();
    const glm::mat4 shakeTranslation = glm::translate(glm::mat4(1.0f), glm::vec3(-localOffset.x, -localOffset.y, 0.0f));
    return shakeTranslation * view;
}

glm::mat4 CameraSystem::GetTalkPageFocusView(Player* player, int playerIndex)
{
    const CameraControlState controls = mReadControlState();
    const glm::mat4 normalView = GetPlayerCameraView(player, playerIndex);
    if (!player || !mHasTalkPageFocusPose || mTalkPageFocusBlend <= 0.0f) {
        return normalView;
    }

    glm::vec3 normalUp = player->GetUpVec();
    if (glm::dot(normalUp, normalUp) < 0.000001f) {
        normalUp = glm::vec3(0.0f, 1.0f, 0.0f);
    } else {
        normalUp = glm::normalize(normalUp);
    }

    const glm::vec3 normalCameraPos = mPlayerCamera.GetCameraPos(playerIndex);
    const float talkCameraBlend = player == mTalkCameraPlayer ? GetEasedTalkCameraBlend() : 0.0f;
    const float normalTargetHeight =
        controls.isPlayer2Joined ? mPlayerCameraSettings.splitScreenTargetHeight : mPlayerCameraSettings.targetHeight;
    const float currentCameraTargetHeight =
        glm::mix(normalTargetHeight, mPlayerCameraSettings.talkTargetHeight, talkCameraBlend);
    const glm::vec3 normalTargetPos = player->GetPos() + normalUp * currentCameraTargetHeight;
    const float blend = GetEasedTalkPageFocusBlend();

    const glm::vec3 cameraPos = glm::mix(normalCameraPos, mTalkPageFocusCameraPos, blend);
    const glm::vec3 targetPos = glm::mix(normalTargetPos, mTalkPageFocusTargetPos, blend);
    glm::vec3 up = glm::mix(normalUp, mTalkPageFocusUpVec, blend);
    if (glm::dot(up, up) < 0.000001f) {
        up = normalUp;
    } else {
        up = glm::normalize(up);
    }

    mRenderedTalkPageCameraPos = cameraPos;
    return glm::lookAt(cameraPos, targetPos, up);
}

Actor* CameraSystem::ResolveTalkPageFocusActor() const
{
    SceneSystem& sceneSystem = mScene;
    if (sceneSystem.HasActiveTutorial()) {
        TutorialController* tutorialController = sceneSystem.GetTutorialController();
        const TutorialPage* page = tutorialController ? tutorialController->GetCurrentPage() : nullptr;
        if (!page || !page->focusTarget.IsValid()) {
            return nullptr;
        }

        return mFindPlacedActor ? mFindPlacedActor(page->focusTarget.sequenceName, page->focusTarget.yamlIndex)
                                : nullptr;
    }

    NPC* talkingNPC = sceneSystem.GetTalkingNPC();
    Player* talkingPlayer = sceneSystem.GetTalkingPlayer();
    if (!sceneSystem.IsTalkWithNPC() || !talkingNPC || !talkingPlayer) {
        return nullptr;
    }

    const int talkIndex = sceneSystem.GetTalkUIIndex();
    if (talkIndex < 0) {
        return nullptr;
    }

    const NPCTalkCameraFocusTarget* target =
        talkingNPC->GetResolvedTalkCameraFocusTarget(static_cast<std::size_t>(talkIndex));
    if (!target || !target->IsValid()) {
        return nullptr;
    }

    return mFindPlacedActor ? mFindPlacedActor(target->sequenceName, target->yamlIndex) : nullptr;
}

std::vector<glm::mat4> CameraSystem::GetViews()
{
    const CameraControlState controls = mReadControlState();
    std::vector<glm::mat4> views;
    mIsShowingRevealFocus = false;

    if (mCinematicCamera.IsActive()) {
        views.emplace_back(mCinematicCamera.GetView());
        return views;
    }

    if (controls.isFreeCameraMode) {
        views.emplace_back(mDebugCamera.GetView());
        return views;
    }

    if (mBossDefeatSequence.IsActive()) {
        Actor* focusActor = mBossDefeatSequence.GetCameraFocusActor();
        if (focusActor) {
            const bool isFocusingStar = dynamic_cast<Star*>(focusActor) != nullptr;
            views.emplace_back(
                mFocusCamera.GetCloseFocusView(focusActor,
                                               isFocusingStar ? mPlayerCameraSettings.bossDefeatStarDistance
                                                              : mPlayerCameraSettings.bossDefeatDistance,
                                               isFocusingStar ? mPlayerCameraSettings.bossDefeatStarCameraHeight
                                                              : mPlayerCameraSettings.bossDefeatCameraHeight,
                                               isFocusingStar ? mPlayerCameraSettings.bossDefeatStarTargetHeight
                                                              : mPlayerCameraSettings.bossDefeatTargetHeight,
                                               controls.mainPlayer));
        }

        if (!views.empty()) {
            return views;
        }
    }

    Boat* boatRideCameraTarget = ResolveBoatRideCameraTarget();
    if (boatRideCameraTarget) {
        if (mBoatRideCameraTarget != boatRideCameraTarget) {
            const int playerIndex = GetPrimaryPlayerIndex();
            glm::vec3 cameraPos = mPlayerCamera.GetCameraPos(playerIndex);
            glm::vec3 up = boatRideCameraTarget->GetUpVec();
            if (glm::dot(up, up) < 0.000001f) {
                up = glm::vec3(0.0f, 1.0f, 0.0f);
            } else {
                up = glm::normalize(up);
            }
            if (glm::dot(cameraPos, cameraPos) < 0.000001f) {
                cameraPos = boatRideCameraTarget->GetPos() + up * mPlayerCameraSettings.boatRideCameraHeight;
            }
            mFocusCamera.BeginTransition(
                cameraPos, boatRideCameraTarget->GetPos() + up * mPlayerCameraSettings.boatRideTargetHeight, up);
        }

        mBoatRideCameraTarget = boatRideCameraTarget;
        views.emplace_back(mFocusCamera.GetCloseFocusView(
            boatRideCameraTarget, mPlayerCameraSettings.boatRideDistance, mPlayerCameraSettings.boatRideCameraHeight,
            mPlayerCameraSettings.boatRideTargetHeight, controls.mainPlayer));
        return views;
    }
    mBoatRideCameraTarget = nullptr;

    const std::vector<Player*>& players = mWorld.GetPlayers();
    const int primaryPlayerIndex = GetPrimaryPlayerIndex();
    if (primaryPlayerIndex < 0 || primaryPlayerIndex >= static_cast<int>(players.size())) {
        return views;
    }
    Player* primaryPlayer = players[static_cast<std::size_t>(primaryPlayerIndex)];
    if (!primaryPlayer) {
        return views;
    }

    SceneSystem& sceneSystem = mScene;
    if (sceneSystem.IsOpening()) {
        OpeningCameraShot shot = OpeningCameraShot::None;
        if (sceneSystem.IsTalkWithMother()) {
            shot = OpeningCameraShot::Mother;
        } else if (sceneSystem.IsTalkWithDoctor()) {
            shot = OpeningCameraShot::Doctor;
        }
        views = mFocusCamera.GetOpeningViews(shot);
        if (!views.empty()) {
            return views;
        }
    }

    Planet* currentPlanet = primaryPlayer->GetCurrentPlanet();
    if (!currentPlanet) {
        return views;
    }

    const bool isPlayer2Joined = controls.isPlayer2Joined && players.size() >= 2 && players[1];
    if ((mTalkCameraPlayer && mTalkCameraBlend > 0.0f) || (mHasTalkPageFocusPose && mTalkPageFocusBlend > 0.0f)) {
        views.emplace_back(GetTalkPageFocusView(isPlayer2Joined ? players[0] : primaryPlayer,
                                                isPlayer2Joined ? 0 : primaryPlayerIndex));
        if (isPlayer2Joined) {
            views.emplace_back(GetPlayerCameraView(players[1], 1));
        }
        return views;
    }

    std::vector<Actor*> focusingActors = FindFocusingActors();
    if (!focusingActors.empty()) {
        mIsShowingRevealFocus = true;
        if (focusingActors != mRevealFocusActors) {
            mRevealFocusActors = focusingActors;
            mRevealFocusView = mFocusCamera.GetFocusView(focusingActors, mPlayerCamera.GetCameraPos(primaryPlayerIndex),
                                                         controls.mainPlayer);
        }
        views.emplace_back(mRevealFocusView);
        return views;
    }
    mRevealFocusActors.clear();

    if (sceneSystem.IsStageClear()) {
        views.emplace_back(mPlayerCamera.GetView(primaryPlayer, primaryPlayerIndex, 6.0f, -1.0f, 1.0f, true));
        return views;
    }

    if (mIsTargetFocus) {
        Enemy* targetEnemy = FindBossEnemy(currentPlanet);
        if (targetEnemy) {
            views.emplace_back(ApplyPlayerShake(mFocusCamera.GetTargetCameraView(targetEnemy, controls.mainPlayer),
                                                primaryPlayerIndex));
            return views;
        }
    }

    views.emplace_back(GetPlayerCameraView(primaryPlayer, primaryPlayerIndex));

    if (isPlayer2Joined) {
        views.emplace_back(GetPlayerCameraView(players[1], 1));
    }

    return views;
}

glm::vec3 CameraSystem::GetCameraPos() const
{
    const CameraControlState controls = mReadControlState();
    if (mCinematicCamera.IsActive()) {
        return mCinematicCamera.GetPose().position;
    }

    if (controls.isFreeCameraMode) {
        return mDebugCamera.GetCameraPos();
    }

    if (mHasTalkPageFocusPose && mTalkPageFocusBlend > 0.0f) {
        return mRenderedTalkPageCameraPos;
    }

    if (mTalkCameraPlayer && mTalkCameraBlend > 0.0f) {
        return mPlayerCamera.GetCameraPos(GetPrimaryPlayerIndex());
    }

    if (ResolveBoatRideCameraTarget()) {
        return mFocusCamera.GetCameraPos();
    }

    if (mIsTargetFocus) {
        return mFocusCamera.GetCameraPos();
    }

    if (mIsShowingRevealFocus) {
        return mFocusCamera.GetCameraPos();
    }

    return GetPlayerCameraPos(GetPrimaryPlayerIndex());
}

glm::vec3 CameraSystem::GetPlayerCameraPos(int playerIndex) const
{
    if (const auto pose = mCameraZones.GetPose(playerIndex)) {
        return pose->position;
    }
    return mPlayerCamera.GetCameraPos(playerIndex);
}

float CameraSystem::GetPlayerFieldOfViewDegrees(int playerIndex) const
{
    const auto controls = mReadControlState();
    if (mScene.IsPlaying() && !controls.isFreeCameraMode && !mCinematicCamera.IsActive() &&
        !mBossDefeatSequence.IsActive() && !mIsTargetFocus && !mIsShowingRevealFocus && mTalkCameraBlend <= 0.0f &&
        mTalkPageFocusBlend <= 0.0f && !ResolveBoatRideCameraTarget()) {
        if (const auto pose = mCameraZones.GetPose(playerIndex)) {
            return pose->fieldOfViewDegrees;
        }
        return controls.isPlayer2Joined ? mPlayerCameraSettings.splitScreenFieldOfViewDegrees
                                        : mPlayerCameraSettings.fieldOfViewDegrees;
    }
    return GetFieldOfViewDegrees();
}

void CameraSystem::SetCameraZonePreview(int yamlIndex)
{
    mCameraZonePreviewIndex = yamlIndex;
    mCameraZones.Reset(mWorld.GetPlayers());
}

int CameraSystem::GetPrimaryPlayerIndex() const
{
    const CameraControlState controls = mReadControlState();
    if (controls.isPlayer2Joined) {
        return 0;
    }

    const int controlledIndex = controls.controlledPlayerIndex;
    const std::vector<Player*>& players = mWorld.GetPlayers();
    if (controlledIndex < 0 || controlledIndex >= static_cast<int>(players.size())) {
        return 0;
    }

    return controlledIndex;
}

std::vector<Actor*> CameraSystem::FindFocusingActors() const
{
    std::vector<Actor*> focusingActors;
    Stage* stage = mWorld.GetCurrentStage();
    if (!stage) {
        return focusingActors;
    }

    for (Planet* planet : stage->GetPlanets()) {
        if (!planet) {
            continue;
        }

        Key* key = planet->GetKey();
        FocusComponent* keyFocusComponent = key ? key->GetFocusComponent() : nullptr;
        if (keyFocusComponent && keyFocusComponent->GetFocusTimer() > 0.0f) {
            focusingActors.emplace_back(key);
        }

        for (Boat* boat : planet->GetBoats()) {
            if (!boat) {
                continue;
            }

            FocusComponent* focusComponent = boat->GetFocusComponent();
            if (focusComponent && focusComponent->GetFocusTimer() > 0.0f) {
                focusingActors.emplace_back(boat);
            }
        }

        for (Platform* platform : planet->GetPlatforms()) {
            if (!platform) {
                continue;
            }

            FocusComponent* focusComponent = platform->GetFocusComponent();
            if (focusComponent && focusComponent->GetFocusTimer() > 0.0f) {
                focusingActors.emplace_back(platform);
            }
        }
    }
    return focusingActors;
}

bool CameraSystem::HasActiveRevealFocus() const
{
    return !FindFocusingActors().empty();
}

Boat* CameraSystem::FindMovingBoat() const
{
    Stage* stage = mWorld.GetCurrentStage();
    if (!stage) {
        return nullptr;
    }

    for (Planet* planet : stage->GetPlanets()) {
        if (!planet) {
            continue;
        }

        for (Boat* boat : planet->GetBoats()) {
            if (boat && boat->GetIsActive() && boat->GetIsMoving()) {
                return boat;
            }
        }
    }
    return nullptr;
}

Boat* CameraSystem::ResolveBoatRideCameraTarget() const
{
    const CameraControlState controls = mReadControlState();
    if (Boat* movingBoat = FindMovingBoat()) {
        return movingBoat;
    }

    const bool allowsEditorPreview = controls.isDebugEditorShowing && mIsBoatRideCameraPreviewEnabled;
    if (!allowsEditorPreview || !mWorld.GetCurrentStage()) {
        return nullptr;
    }

    for (Planet* planet : mWorld.GetCurrentStage()->GetPlanets()) {
        if (!planet) {
            continue;
        }

        for (Boat* boat : planet->GetBoats()) {
            if (boat && !boat->IsDebugDisabled()) {
                return boat;
            }
        }
    }
    return nullptr;
}

Enemy* CameraSystem::FindBossEnemy(Planet* planet) const
{
    if (!planet) {
        return nullptr;
    }

    const std::vector<Enemy*> enemies = planet->GetEnemies();
    for (Enemy* enemy : enemies) {
        if (!enemy || !enemy->GetIsActive() || !enemy->GetIsBoss()) {
            continue;
        }

        return enemy;
    }

    return nullptr;
}
