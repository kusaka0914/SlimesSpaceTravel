#include "CameraSystem.h"

#include "Game.h"
#include "actor/Enemy.h"
#include "actor/Key.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "component/FocusComponent.h"
#include "system/SceneSystem.h"

#include <SDL.h>
#include <cmath>
#include <glm/common.hpp>

CameraSystem::CameraSystem(Game* game)
    : mGame(game),
      mCollisionResolver(game),
      mDebugCamera(game),
      mFocusCamera(game),
      mPlayerCamera(mCollisionResolver),
      mCinematicLibrary("../assets/data/camera/cinematics.yaml")
{
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

float CameraSystem::GetFieldOfViewDegrees() const
{
    if (mCinematicCamera.IsPlaying()) {
        return glm::clamp(mCinematicCamera.GetPose().fieldOfViewDegrees, 10.0f, 120.0f);
    }

    if (mGame && mGame->GetIsFreeCameraMode()) {
        return mDebugCamera.GetFieldOfViewDegrees();
    }

    return 60.0f;
}

void CameraSystem::UpdateCamera(float deltaTime)
{
    if (!mGame) {
        return;
    }

    if (mCinematicCamera.IsPlaying()) {
        mCinematicCamera.Update(deltaTime);
        return;
    }

    if (mGame->GetIsFreeCameraMode()) {
        mDebugCamera.Update(deltaTime);
        return;
    }

    constexpr float cameraSensitivity = 2.5f;
    const float yawDelta = mCameraStickX * cameraSensitivity * deltaTime;

    mPlayerCamera.Update(mGame->GetPlayers(), yawDelta, deltaTime);
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
        views.emplace_back(mPlayerCamera.GetView(players[0], 0, 6.0f, mCameraPitch, true));
        return views;
    }

    if (mIsTargetFocus) {
        Enemy* targetEnemy = FindBossEnemy(currentPlanet);
        if (targetEnemy) {
            views.emplace_back(mFocusCamera.GetTargetCameraView(targetEnemy));
            return views;
        }
    }

    views.emplace_back(mPlayerCamera.GetView(players[0], 0, 8.0f, mCameraPitch));

    const bool isPlayer2Joined = mGame->GetIsPlayer2Joined() && players.size() >= 2 && players[1];
    if (isPlayer2Joined) {
        views.emplace_back(mPlayerCamera.GetView(players[1], 1, 8.0f, mCameraPitch));
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
