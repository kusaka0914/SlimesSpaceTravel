#include "CameraSystem.h"

#include "Game.h"
#include "actor/Enemy.h"
#include "actor/Key.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "component/FocusComponent.h"
#include "system/SceneSystem.h"

#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <glm/common.hpp>

CameraSystem::CameraSystem(Game* game)
    : mGame(game),
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
    if (!mGame || mCinematicCamera.IsPlaying()) {
        return;
    }

    if (mGame->GetIsFreeCameraMode()) {
        mDebugCamera.ProcessInput();
        return;
    }

    SDL_GameController* sdlController = mGame->GetSdlController();
    if (!sdlController) {
        mCameraStickX = 0.0f;
        return;
    }

    constexpr float deadZone = 0.25f;
    constexpr float scale = 1.0f / 32767.0f;

    mCameraStickX = SDL_GameControllerGetAxis(sdlController, SDL_CONTROLLER_AXIS_RIGHTX) * scale;

    if (std::abs(mCameraStickX) < deadZone) {
        mCameraStickX = 0.0f;
    }
}

void CameraSystem::Update(float deltaTime)
{
    UpdateCamera(deltaTime);
}

bool CameraSystem::PlayCinematic(std::string_view sequenceId)
{
    const CinematicSequence* sequence = mCinematicLibrary.Find(sequenceId);
    return sequence && mCinematicCamera.Play(*sequence);
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

float CameraSystem::GetFieldOfViewDegrees() const
{
    if (mCinematicCamera.IsPlaying()) {
        return glm::clamp(mCinematicCamera.GetPose().fieldOfViewDegrees, 10.0f, 120.0f);
    }

    if (mGame && mGame->GetIsFreeCameraMode()) {
        return mDebugCamera.GetFieldOfViewDegrees();
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

    if (mCinematicCamera.IsPlaying()) {
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

    mPlayerCamera.Update(mGame->GetPlayers(), yawDelta, mPlayerCameraSettings.upSmoothingSpeed,
                         mPlayerCameraSettings.targetSmoothingSpeed,
                         mPlayerCameraSettings.attackTargetSmoothingSpeed, deltaTime);
}

void CameraSystem::UpdateTalkCameraTransition(float deltaTime)
{
    SceneSystem* sceneSystem = mGame ? mGame->GetSceneSystem() : nullptr;

    Player* targetPlayer = nullptr;
    if (sceneSystem && sceneSystem->IsTalkWithNPC() && sceneSystem->GetTalkingPlayer()) {
        targetPlayer = sceneSystem->GetTalkingPlayer();
    } else if (mTalkCameraPreviewEnabled && mGame) {
        targetPlayer = mGame->GetMainPlayer();
    }

    if (targetPlayer) {
        if (mTalkCameraPlayer && mTalkCameraPlayer != targetPlayer) {
            mTalkCameraBlend = 0.0f;
        }

        mTalkCameraPlayer = targetPlayer;

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
    }
}

float CameraSystem::GetEasedTalkCameraBlend() const
{
    const float blend = glm::clamp(mTalkCameraBlend, 0.0f, 1.0f);
    return blend * blend * (3.0f - 2.0f * blend);
}

glm::mat4 CameraSystem::GetPlayerCameraView(Player* player, int playerIndex)
{
    const float talkBlend = player && player == mTalkCameraPlayer ? GetEasedTalkCameraBlend() : 0.0f;
    const float distance =
        glm::mix(mPlayerCameraSettings.distance, mPlayerCameraSettings.talkDistance, talkBlend);
    const float pitchDegrees =
        glm::mix(mPlayerCameraSettings.pitchDegrees, mPlayerCameraSettings.talkPitchDegrees, talkBlend);
    const float targetHeight =
        glm::mix(mPlayerCameraSettings.targetHeight, mPlayerCameraSettings.talkTargetHeight, talkBlend);

    return mPlayerCamera.GetView(
        player, playerIndex, distance, glm::radians(pitchDegrees), targetHeight);
}

std::vector<glm::mat4> CameraSystem::GetViews()
{
    std::vector<glm::mat4> views;

    if (!mGame) {
        return views;
    }

    if (mCinematicCamera.IsPlaying()) {
        views.emplace_back(mCinematicCamera.GetView());
        return views;
    }

    if (mGame->GetIsFreeCameraMode()) {
        views.emplace_back(mDebugCamera.GetView());
        return views;
    }

    const std::vector<Player*>& players = mGame->GetPlayers();
    if (players.empty() || !players[0]) {
        return views;
    }

    SceneSystem* sceneSystem = mGame->GetSceneSystem();
    if (sceneSystem && sceneSystem->IsOpening()) {
        views = mFocusCamera.GetOpeningViews();
        if (!views.empty()) {
            return views;
        }
    }

    Planet* currentPlanet = players[0]->GetCurrentPlanet();
    if (!currentPlanet) {
        return views;
    }

    const bool isPlayer2Joined = mGame->GetIsPlayer2Joined() && players.size() >= 2 && players[1];
    if (mTalkCameraPlayer && mTalkCameraBlend > 0.0f) {
        views.emplace_back(GetPlayerCameraView(players[0], 0));
        if (isPlayer2Joined) {
            views.emplace_back(GetPlayerCameraView(players[1], 1));
        }
        return views;
    }

    const std::vector<Boat*> boats = currentPlanet->GetBoats();
    if (!boats.empty()) {
        views = mFocusCamera.GetBoatFocusViews(boats);
        if (!views.empty()) {
            return views;
        }
    }

    Key* key = currentPlanet->GetKey();
    if (key) {
        FocusComponent* focusComponent = key->GetFocusComponent();
        if (focusComponent && focusComponent->GetFocusTimer() >= 0.0f) {
            views.emplace_back(mFocusCamera.GetFocusView(key));
            return views;
        }
    }

    if (sceneSystem && sceneSystem->IsStageClear()) {
        views.emplace_back(mPlayerCamera.GetView(players[0], 0, 6.0f, -1.0f, 1.0f, true));
        return views;
    }

    if (mIsTargetFocus) {
        Enemy* targetEnemy = FindBossEnemy(currentPlanet);
        if (targetEnemy) {
            views.emplace_back(mFocusCamera.GetTargetCameraView(targetEnemy));
            return views;
        }
    }

    views.emplace_back(GetPlayerCameraView(players[0], 0));

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

    if (mCinematicCamera.IsPlaying()) {
        return mCinematicCamera.GetPose().position;
    }

    if (mGame->GetIsFreeCameraMode()) {
        return mDebugCamera.GetCameraPos();
    }

    if (mTalkCameraPlayer && mTalkCameraBlend > 0.0f) {
        return mPlayerCamera.GetCameraPos(0);
    }

    if (mIsTargetFocus) {
        return mFocusCamera.GetCameraPos();
    }

    return mPlayerCamera.GetCameraPos(0);
}

glm::vec3 CameraSystem::GetPlayerCameraPos(int playerNum) const
{
    return mPlayerCamera.GetCameraPos(playerNum);
}

Enemy* CameraSystem::FindBossEnemy(Planet* planet) const
{
    if (!planet) {
        return nullptr;
    }

    const std::vector<Enemy*> enemies = planet->GetEnemies();
    for (Enemy* enemy : enemies) {
        if (!enemy || !enemy->GetIsBoss()) {
            continue;
        }

        return enemy;
    }

    return nullptr;
}
