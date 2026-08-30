#include "CameraSystem.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Boat.h"
#include "actor/Enemy.h"
#include "actor/Key.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Player.h"
#include "actor/Star.h"
#include "component/FocusComponent.h"
#include "system/ActorLoadSystem.h"
#include "system/AudioSystem.h"
#include "system/InputSystem.h"
#include "system/SceneSystem.h"
#include "system/scene/TutorialController.h"
#include "system/tutorial/TutorialLibrary.h"

#include <GLFW/glfw3.h>
#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>

CameraSystem::CameraSystem(Game* game)
    : mGame(game),
      mBossDefeatSequence(game),
      mCollisionResolver(game),
      mDebugCamera(game),
      mFocusCamera(game),
      mPlayerCamera(mCollisionResolver),
      mPlayerCameraSettingsRepository("../assets/data/camera/player.yaml"),
      mCinematicLibrary("../assets/data/camera/cinematics.yaml")
{
    mPlayerCameraSettingsRepository.Load(mPlayerCameraSettings);
    mCinematicLibrary.Load();
}

void CameraSystem::ProcessInput()
{
    if (!mGame || mCinematicCamera.IsActive()) {
        return;
    }

    if (mGame->IsEditorKeyboardInputCaptured()) {
        mCameraStickX = 0.0f;
        mCameraStickY = 0.0f;
        mKeyboardPitchInput = 0.0f;
        mAlignCameraPressedPrev = true;
        if (mGame->GetIsFreeCameraMode()) {
            mDebugCamera.ProcessInput();
        }
        return;
    }

    if (mGame->GetIsFreeCameraMode()) {
        mDebugCamera.ProcessInput();
        return;
    }

    SceneSystem* sceneSystem = mGame->GetSceneSystem();
    if (sceneSystem &&
        sceneSystem->IsWaitingForTutorialPlayerAction()) {
        mCameraStickX = 0.0f;
        mCameraStickY = 0.0f;
        mKeyboardPitchInput = 0.0f;
        mAlignCameraPressedPrev = false;
        return;
    }

    InputSystem* inputSystem = mGame->GetInputSystem();
    if (!inputSystem) {
        return;
    }
    const bool hasPrimaryController =
        inputSystem->HasControllerInput(1);
    const bool hasSecondController =
        inputSystem->HasControllerInput(2);

    constexpr float deadZone = 0.25f;
    constexpr float scale = 1.0f / 32767.0f;

    mCameraStickX =
        hasPrimaryController
            ? inputSystem->GetControllerAxis(1, SDL_CONTROLLER_AXIS_RIGHTX) * scale
            : 0.0f;
    mCameraStickY =
        hasPrimaryController
            ? inputSystem->GetControllerAxis(1, SDL_CONTROLLER_AXIS_RIGHTY) * scale
            : 0.0f;

    if (std::abs(mCameraStickX) < deadZone) {
        mCameraStickX = 0.0f;
    }
    if (std::abs(mCameraStickY) < deadZone) {
        mCameraStickY = 0.0f;
    }
    mSecondControllerStickX =
        hasSecondController
            ? inputSystem->GetControllerAxis(
                  2, SDL_CONTROLLER_AXIS_RIGHTX) * scale
            : 0.0f;
    mSecondControllerStickY =
        hasSecondController
            ? inputSystem->GetControllerAxis(
                  2, SDL_CONTROLLER_AXIS_RIGHTY) * scale
            : 0.0f;
    if (std::abs(mSecondControllerStickX) < deadZone) {
        mSecondControllerStickX = 0.0f;
    }
    if (std::abs(mSecondControllerStickY) < deadZone) {
        mSecondControllerStickY = 0.0f;
    }

    mKeyboardPitchInput = 0.0f;
    const bool acceptsKeyboardInput =
        !mGame->IsEditorKeyboardInputCaptured();
    if (acceptsKeyboardInput &&
        inputSystem->IsKeyPressed(GLFW_KEY_UP)) {
        mKeyboardPitchInput += 1.0f;
    }
    if (acceptsKeyboardInput &&
        inputSystem->IsKeyPressed(GLFW_KEY_DOWN)) {
        mKeyboardPitchInput -= 1.0f;
    }

    const bool keyboardAlignCameraPressed =
        acceptsKeyboardInput &&
        inputSystem->IsKeyPressed(GLFW_KEY_M);
    const bool controllerAlignCameraPressed =
        inputSystem->IsControllerButtonPressed(
            1, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
    const bool secondControllerAlignCameraPressed =
        inputSystem->IsControllerButtonPressed(
            2, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
    const bool playerTwoUsesController =
        mGame->GetIsPlayer2Joined() &&
        mGame->HasGameControllerForPlayer(2);
    const bool alignCameraPressed =
        controllerAlignCameraPressed ||
        (keyboardAlignCameraPressed && !playerTwoUsesController);

    if (sceneSystem && sceneSystem->IsPlaying() && alignCameraPressed && !mAlignCameraPressedPrev) {
        int playerIndex = GetPrimaryPlayerIndex();
        if (mGame->GetIsPlayer2Joined()) {


            playerIndex =
                (keyboardAlignCameraPressed && !playerTwoUsesController)
                    ? 1
                    : 0;
        }
        const std::vector<Player*>& players = mGame->GetPlayers();
        if (playerIndex >= 0 &&
            playerIndex < static_cast<int>(players.size()) &&
            players[static_cast<std::size_t>(playerIndex)]) {
            mPlayerCamera.AlignBehindPlayer(
                players[static_cast<std::size_t>(playerIndex)], playerIndex);
        }
    }

    mAlignCameraPressedPrev = alignCameraPressed;
    if (sceneSystem && sceneSystem->IsPlaying() &&
        secondControllerAlignCameraPressed &&
        !mSecondControllerAlignCameraPressedPrev && playerTwoUsesController) {
        const std::vector<Player*>& players = mGame->GetPlayers();
        if (players.size() >= 2 && players[1]) {
            mPlayerCamera.AlignBehindPlayer(players[1], 1);
        }
    }
    mSecondControllerAlignCameraPressedPrev =
        secondControllerAlignCameraPressed;
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
    mPlayerShakeEffects[shakeIndex].TryStart(
        CameraShakePattern::AirStrongAttackHit);
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
    mPlayerShakeEffects[shakeIndex].TryStart(
        CameraShakePattern::PlayerDamaged);
}

bool CameraSystem::PlayCinematic(
    std::string_view sequenceId,
    bool shouldHoldFinalPose)
{
    const CinematicSequence* sequence = mCinematicLibrary.Find(sequenceId);
    return sequence &&
           mCinematicCamera.Play(*sequence, shouldHoldFinalPose);
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
    mAlignCameraPressedPrev = false;
    mCameraStickX = 0.0f;
    mCameraStickY = 0.0f;
    mKeyboardPitchInput = 0.0f;
    mPlayerPitchOffsetsDegrees.clear();
    mPlayerShakeEffects.clear();
    mPlayerCamera.Reset();
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
    if (!mGame) {
        return;
    }

    Player* controlledPlayer = mGame->GetControlledPlayer();
    if (!controlledPlayer) {
        return;
    }

    mPlayerCamera.SnapBehindPlayer(
        controlledPlayer,
        GetPrimaryPlayerIndex());
}

float CameraSystem::GetFieldOfViewDegrees() const
{
    if (mCinematicCamera.IsActive()) {
        return glm::clamp(mCinematicCamera.GetPose().fieldOfViewDegrees, 10.0f, 120.0f);
    }

    if (mGame && mGame->GetIsFreeCameraMode()) {
        return mDebugCamera.GetFieldOfViewDegrees();
    }

    if (mBossDefeatSequence.IsActive()) {
        return mPlayerCameraSettings.bossDefeatFieldOfViewDegrees;
    }

    if (ResolveBoatRideCameraTarget()) {
        return mPlayerCameraSettings.boatRideFieldOfViewDegrees;
    }

    const float normalFieldOfView =
        mGame && mGame->GetIsPlayer2Joined()
            ? mPlayerCameraSettings.splitScreenFieldOfViewDegrees
            : mPlayerCameraSettings.fieldOfViewDegrees;

    return glm::mix(
        normalFieldOfView, mPlayerCameraSettings.talkFieldOfViewDegrees, GetEasedTalkCameraBlend());
}

void CameraSystem::UpdateCamera(float deltaTime)
{
    if (!mGame) {
        return;
    }

    UpdateTalkCameraTransition(deltaTime);
    mBossDefeatSequence.Update(deltaTime);

    if (mCinematicCamera.IsActive()) {
        mCinematicCamera.Update(deltaTime);
        return;
    }

    if (mGame->GetIsFreeCameraMode()) {
        mDebugCamera.Update(deltaTime);
        return;
    }

    const float yawDelta = mTalkCameraBlend > 0.0f
                               ? 0.0f
                               : mCameraStickX * mPlayerCameraSettings.yawSensitivity * deltaTime;

    UpdatePlayerPitchOffsets(deltaTime);

    const std::vector<Player*>& players = mGame->GetPlayers();
    std::vector<float> yawDeltas(players.size(), 0.0f);
    const int primaryPlayerIndex = GetPrimaryPlayerIndex();
    if (primaryPlayerIndex >= 0 &&
        primaryPlayerIndex < static_cast<int>(yawDeltas.size())) {
        yawDeltas[static_cast<std::size_t>(primaryPlayerIndex)] = yawDelta;
    }
    if (mGame->GetIsPlayer2Joined() &&
        mGame->HasGameControllerForPlayer(2) &&
        yawDeltas.size() >= 2) {
        yawDeltas[1] =
            mSecondControllerStickX *
            mPlayerCameraSettings.yawSensitivity * deltaTime;
    }

    SceneSystem* sceneSystem = mGame->GetSceneSystem();
    const bool allowsMovementCameraAssist =
        sceneSystem && sceneSystem->IsPlaying() &&
        mTalkCameraBlend <= 0.0f &&
        !mBossDefeatSequence.IsActive() &&
        !mIsTargetFocus;
    mPlayerCamera.Update(players, yawDeltas,
                         mPlayerCameraSettings, deltaTime,
                         allowsMovementCameraAssist);
    UpdateTalkCameraAim();
    UpdateTalkPageFocus(deltaTime);
}

void CameraSystem::UpdatePlayerPitchOffsets(float deltaTime)
{
    if (!mGame) {
        return;
    }

    const std::vector<Player*>& players = mGame->GetPlayers();
    if (mPlayerPitchOffsetsDegrees.size() < players.size()) {
        mPlayerPitchOffsetsDegrees.resize(players.size(), 0.0f);
    }

    const bool allowsManualPitch =
        mTalkCameraBlend <= 0.0f &&
        !mBossDefeatSequence.IsActive() &&
        !mIsTargetFocus;

    if (allowsManualPitch) {
        const float pitchSpeed =
            mPlayerCameraSettings.pitchSensitivityDegrees * std::max(0.0f, deltaTime);

        if (mGame->GetIsPlayer2Joined()) {
            if (!players.empty() && players[0] && mGame->IsGameControllerConnected()) {
                mPlayerPitchOffsetsDegrees[0] += -mCameraStickY * pitchSpeed;
            }

            int keyboardPlayerIndex = -1;
            if (mGame->HasGameControllerForPlayer(2)) {
                if (players.size() >= 2 && players[1]) {
                    mPlayerPitchOffsetsDegrees[1] +=
                        -mSecondControllerStickY * pitchSpeed;
                }
            } else if (mGame->IsGameControllerConnected()) {
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
            if (controlledIndex >= 0 &&
                controlledIndex < static_cast<int>(players.size()) &&
                players[static_cast<std::size_t>(controlledIndex)]) {
                if (mGame->IsGameControllerConnected()) {
                    mPlayerPitchOffsetsDegrees[static_cast<std::size_t>(controlledIndex)] +=
                        -mCameraStickY * pitchSpeed;
                } else {
                    mPlayerPitchOffsetsDegrees[static_cast<std::size_t>(controlledIndex)] +=
                        mKeyboardPitchInput * pitchSpeed;
                }
            }
        }
    }

    for (float& pitchOffset : mPlayerPitchOffsetsDegrees) {
        const float pitch = glm::clamp(
            mPlayerCameraSettings.pitchDegrees + pitchOffset,
            mPlayerCameraSettings.minPitchDegrees,
            mPlayerCameraSettings.maxPitchDegrees);
        pitchOffset = pitch - mPlayerCameraSettings.pitchDegrees;
    }
}

bool CameraSystem::AllowsPlayerInput() const
{
    return !mBossDefeatSequence.IsActive() && !mCinematicCamera.IsActive() &&
           !(mGame && mGame->GetIsFreeCameraMode()) && !mIsTargetFocus &&
           !FindMovingBoat();
}

void CameraSystem::StartBossDefeatSequence(Enemy* boss, Star* star)
{
    BeginBossDefeatSequence(boss, star, false);
}

void CameraSystem::BeginBossDefeatSequence(
    Enemy* boss,
    Star* star,
    bool isPreview)
{
    if (!boss) {
        return;
    }

    mBossDefeatSequence.Start(boss, star, isPreview);
    mCameraStickX = 0.0f;

    Player* player = mGame ? mGame->GetMainPlayer() : nullptr;
    const glm::vec3 targetPos = player ? player->GetPos() : boss->GetPos();
    const glm::vec3 up = player ? player->GetUpVec() : boss->GetUpVec();
    mFocusCamera.BeginTransition(
        mPlayerCamera.GetCameraPos(GetPrimaryPlayerIndex()),
        targetPos,
        up);
}

bool CameraSystem::PreviewBossDefeatSequence()
{
    Player* player = mGame ? mGame->GetMainPlayer() : nullptr;
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
    SceneSystem* sceneSystem = mGame ? mGame->GetSceneSystem() : nullptr;

    Player* targetPlayer = nullptr;
    NPC* targetNPC = nullptr;
    if (sceneSystem && sceneSystem->IsTalkWithNPC() &&
        sceneSystem->GetTalkingPlayer() &&
        sceneSystem->GetTalkingNPC() &&
        sceneSystem->GetTalkingNPC()->ShouldUseTalkCamera()) {
        targetPlayer = sceneSystem->GetTalkingPlayer();
        targetNPC = sceneSystem->GetTalkingNPC();
    } else if (mTalkCameraPreviewEnabled && mGame) {
        targetPlayer = mGame->GetMainPlayer();
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
    if (!mGame || !mTalkCameraPlayer || !mHasTalkCameraTarget || mTalkCameraBlend <= 0.0f) {
        return;
    }

    const std::vector<Player*>& players = mGame->GetPlayers();
    for (int playerIndex = 0; playerIndex < static_cast<int>(players.size()); ++playerIndex) {
        if (players[playerIndex] != mTalkCameraPlayer) {
            continue;
        }

        mPlayerCamera.BlendBehindTarget(
            mTalkCameraPlayer, playerIndex, mTalkCameraTargetPos, GetEasedTalkCameraBlend());
        return;
    }
}

void CameraSystem::SnapToControlledPlayer(
    int fromPlayerIndex,
    int toPlayerIndex)
{
    if (!mGame || toPlayerIndex < 0) {
        return;
    }

    const std::vector<Player*>& players =
        mGame->GetPlayers();
    if (toPlayerIndex >= static_cast<int>(players.size()) ||
        !players[static_cast<std::size_t>(toPlayerIndex)]) {
        return;
    }

    mPlayerCamera.SnapToPlayer(
        players[static_cast<std::size_t>(toPlayerIndex)],
        toPlayerIndex);

    CopyPlayerPitchOffset(fromPlayerIndex, toPlayerIndex);
}

void CameraSystem::TransitionToControlledPlayer(
    int fromPlayerIndex,
    int toPlayerIndex)
{
    if (!mGame || toPlayerIndex < 0) {
        return;
    }

    const std::vector<Player*>& players = mGame->GetPlayers();
    if (toPlayerIndex >= static_cast<int>(players.size()) ||
        !players[static_cast<std::size_t>(toPlayerIndex)]) {
        return;
    }

    mPlayerCamera.TransitionToPlayer(
        fromPlayerIndex,
        players[static_cast<std::size_t>(toPlayerIndex)],
        toPlayerIndex);

    CopyPlayerPitchOffset(fromPlayerIndex, toPlayerIndex);
}

void CameraSystem::CopyPlayerPitchOffset(
    int fromPlayerIndex,
    int toPlayerIndex)
{
    if (toPlayerIndex < 0) {
        return;
    }

    const int requiredPitchCount =
        std::max(fromPlayerIndex, toPlayerIndex) + 1;
    if (requiredPitchCount <= 0) {
        return;
    }

    if (mPlayerPitchOffsetsDegrees.size() <
        static_cast<std::size_t>(requiredPitchCount)) {
        mPlayerPitchOffsetsDegrees.resize(
            static_cast<std::size_t>(requiredPitchCount),
            0.0f);
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

    SceneSystem* sceneSystem =
        mGame ? mGame->GetSceneSystem() : nullptr;
    Player* talkingPlayer =
        sceneSystem ? sceneSystem->GetTalkingPlayer() : nullptr;

    const float safeDeltaTime = std::max(0.0f, deltaTime);
    if (desiredFocusActor) {
        if (!mHasTalkPageFocusPose) {
            glm::vec3 up =
                talkingPlayer
                    ? talkingPlayer->GetUpVec()
                    : desiredFocusActor->GetUpVec();
            if (glm::dot(up, up) < 0.000001f) {
                up = glm::vec3(0.0f, 1.0f, 0.0f);
            } else {
                up = glm::normalize(up);
            }

            int talkPlayerIndex = GetPrimaryPlayerIndex();
            const std::vector<Player*>& players = mGame->GetPlayers();
            for (int playerIndex = 0;
                 playerIndex < static_cast<int>(players.size());
                 ++playerIndex) {
                if (players[static_cast<std::size_t>(playerIndex)] ==
                    talkingPlayer) {
                    talkPlayerIndex = playerIndex;
                    break;
                }
            }

            mTalkPageFocusCameraPos =
                mPlayerCamera.GetCameraPos(talkPlayerIndex);
            if (glm::dot(mTalkPageFocusCameraPos, mTalkPageFocusCameraPos) < 0.000001f) {
                mTalkPageFocusCameraPos =
                    (talkingPlayer
                         ? talkingPlayer->GetPos()
                         : desiredFocusActor->GetPos()) +
                    up * 3.0f;
            }
            mTalkPageFocusTargetPos =
                (talkingPlayer
                     ? talkingPlayer->GetPos()
                     : desiredFocusActor->GetPos()) +
                up * mPlayerCameraSettings.talkTargetHeight;
            mTalkPageFocusUpVec = up;
            mHasTalkPageFocusPose = true;
        }

        mTalkPageFocusActor = desiredFocusActor;

        const float duration = mPlayerCameraSettings.talkTransitionInDuration;
        mTalkPageFocusBlend =
            duration <= 0.0f
                ? 1.0f
                : std::min(1.0f, mTalkPageFocusBlend + safeDeltaTime / duration);

        glm::vec3 up = desiredFocusActor->GetUpVec();
        if (glm::dot(up, up) < 0.000001f) {
            up = glm::vec3(0.0f, 1.0f, 0.0f);
        } else {
            up = glm::normalize(up);
        }

        glm::vec3 back =
            (talkingPlayer
                 ? talkingPlayer->GetPos()
                 : desiredFocusActor->GetPos()) -
            desiredFocusActor->GetPos();
        back -= up * glm::dot(back, up);
        if (glm::dot(back, back) < 0.000001f) {
            back = glm::cross(up, glm::vec3(1.0f, 0.0f, 0.0f));
        }
        if (glm::dot(back, back) < 0.000001f) {
            back = glm::cross(up, glm::vec3(0.0f, 0.0f, 1.0f));
        }
        back = glm::normalize(back);

        const glm::vec3 desiredTargetPos =
            desiredFocusActor->GetPos() + up * mPlayerCameraSettings.talkTargetHeight;
        const glm::vec3 uncorrectedCameraPos =
            desiredTargetPos +
            back * mPlayerCameraSettings.talkDistance +
            up * (mPlayerCameraSettings.talkDistance * 0.35f);
        const glm::vec3 desiredCameraPos =
            mCollisionResolver.Resolve(desiredTargetPos, uncorrectedCameraPos);

        constexpr float focusPoseSmoothingSpeed = 7.0f;
        const float poseBlend =
            1.0f - std::exp(-focusPoseSmoothingSpeed * safeDeltaTime);
        mTalkPageFocusCameraPos =
            glm::mix(mTalkPageFocusCameraPos, desiredCameraPos, poseBlend);
        mTalkPageFocusTargetPos =
            glm::mix(mTalkPageFocusTargetPos, desiredTargetPos, poseBlend);
        const glm::vec3 blendedUp = glm::mix(mTalkPageFocusUpVec, up, poseBlend);
        if (glm::dot(blendedUp, blendedUp) >= 0.000001f) {
            mTalkPageFocusUpVec = glm::normalize(blendedUp);
        }
        return;
    }

    mTalkPageFocusActor = nullptr;
    const float duration = mPlayerCameraSettings.talkTransitionOutDuration;
    mTalkPageFocusBlend =
        duration <= 0.0f
            ? 0.0f
            : std::max(0.0f, mTalkPageFocusBlend - safeDeltaTime / duration);

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
    const float talkBlend = player && player == mTalkCameraPlayer ? GetEasedTalkCameraBlend() : 0.0f;
    const float distance =
        glm::mix(mPlayerCameraSettings.distance, mPlayerCameraSettings.talkDistance, talkBlend);
    const float pitchOffset =
        playerIndex >= 0 &&
        static_cast<std::size_t>(playerIndex) < mPlayerPitchOffsetsDegrees.size()
            ? mPlayerPitchOffsetsDegrees[static_cast<std::size_t>(playerIndex)]
            : 0.0f;
    const float normalPitchDegrees = glm::clamp(
        mPlayerCameraSettings.pitchDegrees + pitchOffset,
        mPlayerCameraSettings.minPitchDegrees,
        mPlayerCameraSettings.maxPitchDegrees);
    const float pitchDegrees =
        glm::mix(normalPitchDegrees, mPlayerCameraSettings.talkPitchDegrees, talkBlend);
    const float normalTargetHeight =
        mGame && mGame->GetIsPlayer2Joined()
            ? mPlayerCameraSettings.splitScreenTargetHeight
            : mPlayerCameraSettings.targetHeight;
    const float targetHeight =
        glm::mix(normalTargetHeight, mPlayerCameraSettings.talkTargetHeight, talkBlend);

    const glm::mat4 view = mPlayerCamera.GetView(
        player, playerIndex, distance, glm::radians(pitchDegrees), targetHeight);
    return ApplyPlayerShake(view, playerIndex);
}

glm::mat4 CameraSystem::ApplyPlayerShake(
    const glm::mat4& view,
    int playerIndex) const
{
    if (playerIndex < 0 ||
        static_cast<std::size_t>(playerIndex) >=
            mPlayerShakeEffects.size()) {
        return view;
    }

    const glm::vec2 localOffset =
        mPlayerShakeEffects[static_cast<std::size_t>(playerIndex)]
            .GetLocalOffset();
    const glm::mat4 shakeTranslation = glm::translate(
        glm::mat4(1.0f),
        glm::vec3(-localOffset.x, -localOffset.y, 0.0f));
    return shakeTranslation * view;
}

glm::mat4 CameraSystem::GetTalkPageFocusView(Player* player, int playerIndex)
{
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
    const float talkCameraBlend =
        player == mTalkCameraPlayer
            ? GetEasedTalkCameraBlend()
            : 0.0f;
    const float normalTargetHeight =
        mGame && mGame->GetIsPlayer2Joined()
            ? mPlayerCameraSettings.splitScreenTargetHeight
            : mPlayerCameraSettings.targetHeight;
    const float currentCameraTargetHeight =
        glm::mix(
            normalTargetHeight,
            mPlayerCameraSettings.talkTargetHeight,
            talkCameraBlend);
    const glm::vec3 normalTargetPos =
        player->GetPos() + normalUp * currentCameraTargetHeight;
    const float blend = GetEasedTalkPageFocusBlend();

    const glm::vec3 cameraPos =
        glm::mix(normalCameraPos, mTalkPageFocusCameraPos, blend);
    const glm::vec3 targetPos =
        glm::mix(normalTargetPos, mTalkPageFocusTargetPos, blend);
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
    if (!mGame) {
        return nullptr;
    }

    SceneSystem* sceneSystem = mGame->GetSceneSystem();
    if (sceneSystem && sceneSystem->HasActiveTutorial()) {
        TutorialController* tutorialController =
            sceneSystem->GetTutorialController();
        const TutorialPage* page =
            tutorialController
                ? tutorialController->GetCurrentPage()
                : nullptr;
        if (!page || !page->focusTarget.IsValid()) {
            return nullptr;
        }

        ActorLoadSystem* actorLoadSystem =
            mGame->GetActorLoadSystem();
        Stage* stage = mGame->GetCurrentStage();
        if (!actorLoadSystem || !stage) {
            return nullptr;
        }
        return actorLoadSystem->GetActorLocator().FindPlacedActor(
            *stage,
            page->focusTarget.sequenceName,
            page->focusTarget.yamlIndex);
    }

    NPC* talkingNPC = sceneSystem ? sceneSystem->GetTalkingNPC() : nullptr;
    Player* talkingPlayer =
        sceneSystem ? sceneSystem->GetTalkingPlayer() : nullptr;
    if (!sceneSystem || !sceneSystem->IsTalkWithNPC() ||
        !talkingNPC || !talkingPlayer) {
        return nullptr;
    }

    const int talkIndex = sceneSystem->GetTalkUIIndex();
    if (talkIndex < 0) {
        return nullptr;
    }

    const NPCTalkCameraFocusTarget* target =
        talkingNPC->GetResolvedTalkCameraFocusTarget(
            static_cast<std::size_t>(talkIndex));
    if (!target || !target->IsValid()) {
        return nullptr;
    }

    ActorLoadSystem* actorLoadSystem = mGame->GetActorLoadSystem();
    Stage* stage = mGame->GetCurrentStage();
    if (!actorLoadSystem || !stage) {
        return nullptr;
    }
    return actorLoadSystem->GetActorLocator().FindPlacedActor(
        *stage,
        target->sequenceName,
        target->yamlIndex);
}

std::vector<glm::mat4> CameraSystem::GetViews()
{
    std::vector<glm::mat4> views;

    if (!mGame) {
        return views;
    }

    if (mCinematicCamera.IsActive()) {
        views.emplace_back(mCinematicCamera.GetView());
        return views;
    }

    if (mGame->GetIsFreeCameraMode()) {
        views.emplace_back(mDebugCamera.GetView());
        return views;
    }

    if (mBossDefeatSequence.IsActive()) {
        Actor* focusActor = mBossDefeatSequence.GetCameraFocusActor();
        if (focusActor) {
            const bool isFocusingStar = dynamic_cast<Star*>(focusActor) != nullptr;
            views.emplace_back(mFocusCamera.GetCloseFocusView(
                focusActor,
                isFocusingStar
                    ? mPlayerCameraSettings.bossDefeatStarDistance
                    : mPlayerCameraSettings.bossDefeatDistance,
                isFocusingStar
                    ? mPlayerCameraSettings.bossDefeatStarCameraHeight
                    : mPlayerCameraSettings.bossDefeatCameraHeight,
                isFocusingStar
                    ? mPlayerCameraSettings.bossDefeatStarTargetHeight
                    : mPlayerCameraSettings.bossDefeatTargetHeight));
        }

        if (!views.empty()) {
            return views;
        }
    }

    Boat* boatRideCameraTarget = ResolveBoatRideCameraTarget();
    if (boatRideCameraTarget) {
        if (mBoatRideCameraTarget != boatRideCameraTarget) {
            const int playerIndex = GetPrimaryPlayerIndex();
            glm::vec3 cameraPos =
                mPlayerCamera.GetCameraPos(playerIndex);
            glm::vec3 up = boatRideCameraTarget->GetUpVec();
            if (glm::dot(up, up) < 0.000001f) {
                up = glm::vec3(0.0f, 1.0f, 0.0f);
            } else {
                up = glm::normalize(up);
            }
            if (glm::dot(cameraPos, cameraPos) < 0.000001f) {
                cameraPos =
                    boatRideCameraTarget->GetPos() +
                    up * mPlayerCameraSettings.boatRideCameraHeight;
            }
            mFocusCamera.BeginTransition(
                cameraPos,
                boatRideCameraTarget->GetPos() +
                    up * mPlayerCameraSettings.boatRideTargetHeight,
                up);
        }

        mBoatRideCameraTarget = boatRideCameraTarget;
        views.emplace_back(
            mFocusCamera.GetCloseFocusView(
                boatRideCameraTarget,
                mPlayerCameraSettings.boatRideDistance,
                mPlayerCameraSettings.boatRideCameraHeight,
                mPlayerCameraSettings.boatRideTargetHeight));
        return views;
    }
    mBoatRideCameraTarget = nullptr;

    const std::vector<Player*>& players = mGame->GetPlayers();
    const int primaryPlayerIndex = GetPrimaryPlayerIndex();
    if (primaryPlayerIndex < 0 ||
        primaryPlayerIndex >= static_cast<int>(players.size())) {
        return views;
    }
    Player* primaryPlayer =
        players[static_cast<std::size_t>(primaryPlayerIndex)];
    if (!primaryPlayer) {
        return views;
    }

    SceneSystem* sceneSystem = mGame->GetSceneSystem();
    if (sceneSystem && sceneSystem->IsOpening()) {
        views = mFocusCamera.GetOpeningViews();
        if (!views.empty()) {
            return views;
        }
    }

    Planet* currentPlanet = primaryPlayer->GetCurrentPlanet();
    if (!currentPlanet) {
        return views;
    }

    const bool isPlayer2Joined = mGame->GetIsPlayer2Joined() && players.size() >= 2 && players[1];
    if ((mTalkCameraPlayer && mTalkCameraBlend > 0.0f) ||
        (mHasTalkPageFocusPose && mTalkPageFocusBlend > 0.0f)) {
        views.emplace_back(
            GetTalkPageFocusView(
                isPlayer2Joined ? players[0] : primaryPlayer,
                isPlayer2Joined ? 0 : primaryPlayerIndex));
        if (isPlayer2Joined) {
            views.emplace_back(GetPlayerCameraView(players[1], 1));
        }
        return views;
    }

    if (Actor* focusingActor = FindFocusingActor()) {
        views.emplace_back(mFocusCamera.GetFocusView(focusingActor));
        return views;
    }

    Key* key = currentPlanet->GetKey();
    if (key) {
        FocusComponent* focusComponent = key->GetFocusComponent();
        if (focusComponent &&
            focusComponent->GetFocusTimer() >= 0.0f) {
            views.emplace_back(mFocusCamera.GetFocusView(key));
            return views;
        }
    }

    if (sceneSystem && sceneSystem->IsStageClear()) {
        views.emplace_back(
            mPlayerCamera.GetView(
                primaryPlayer,
                primaryPlayerIndex,
                6.0f,
                -1.0f,
                1.0f,
                true));
        return views;
    }

    if (mIsTargetFocus) {
        Enemy* targetEnemy = FindBossEnemy(currentPlanet);
        if (targetEnemy) {
            views.emplace_back(
                ApplyPlayerShake(
                    mFocusCamera.GetTargetCameraView(targetEnemy),
                    primaryPlayerIndex));
            return views;
        }
    }

    views.emplace_back(
        GetPlayerCameraView(primaryPlayer, primaryPlayerIndex));

    if (isPlayer2Joined) {
        views.emplace_back(GetPlayerCameraView(players[1], 1));
    }

    return views;
}

glm::vec3 CameraSystem::GetCameraPos() const
{
    if (!mGame) {
        return glm::vec3(0.0f);
    }

    if (mCinematicCamera.IsActive()) {
        return mCinematicCamera.GetPose().position;
    }

    if (mGame->GetIsFreeCameraMode()) {
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

    return mPlayerCamera.GetCameraPos(GetPrimaryPlayerIndex());
}

glm::vec3 CameraSystem::GetPlayerCameraPos(int playerNum) const
{
    return mPlayerCamera.GetCameraPos(playerNum);
}

int CameraSystem::GetPrimaryPlayerIndex() const
{
    if (!mGame || mGame->GetIsPlayer2Joined()) {
        return 0;
    }

    const int controlledIndex = mGame->GetControlledPlayerIndex();
    const std::vector<Player*>& players = mGame->GetPlayers();
    if (controlledIndex < 0 ||
        controlledIndex >= static_cast<int>(players.size())) {
        return 0;
    }

    return controlledIndex;
}

Actor* CameraSystem::FindFocusingActor() const
{
    Stage* stage = mGame ? mGame->GetCurrentStage() : nullptr;
    if (!stage) {
        return nullptr;
    }

    for (Planet* planet : stage->GetPlanets()) {
        if (!planet) {
            continue;
        }

        for (Boat* boat : planet->GetBoats()) {
            if (!boat) {
                continue;
            }

            FocusComponent* focusComponent =
                boat->GetFocusComponent();
            if (focusComponent &&
                focusComponent->GetFocusTimer() >= 0.0f) {
                return boat;
            }
        }

        for (Platform* platform : planet->GetPlatforms()) {
            if (!platform) {
                continue;
            }

            FocusComponent* focusComponent =
                platform->GetFocusComponent();
            if (focusComponent &&
                focusComponent->GetFocusTimer() >= 0.0f) {
                return platform;
            }
        }

    }
    return nullptr;
}

Boat* CameraSystem::FindMovingBoat() const
{
    Stage* stage = mGame ? mGame->GetCurrentStage() : nullptr;
    if (!stage) {
        return nullptr;
    }

    for (Planet* planet : stage->GetPlanets()) {
        if (!planet) {
            continue;
        }

        for (Boat* boat : planet->GetBoats()) {
            if (boat && boat->GetIsActive() &&
                boat->GetIsMoving()) {
                return boat;
            }
        }
    }
    return nullptr;
}

Boat* CameraSystem::ResolveBoatRideCameraTarget() const
{
    if (Boat* movingBoat = FindMovingBoat()) {
        return movingBoat;
    }

    const bool allowsEditorPreview =
        mGame && mGame->GetIsDebugEditorShowing() &&
        mBoatRideCameraPreviewEnabled;
    if (!allowsEditorPreview || !mGame->GetCurrentStage()) {
        return nullptr;
    }

    for (Planet* planet : mGame->GetCurrentStage()->GetPlanets()) {
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
        if (!enemy || !enemy->GetIsActive() ||
            !enemy->GetIsBoss()) {
            continue;
        }

        return enemy;
    }

    return nullptr;
}
