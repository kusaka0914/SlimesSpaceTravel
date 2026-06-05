#include "CameraSystem.h"
#include "Game.h"
#include "actor/Actor.h"
#include "actor/Boat.h"
#include "actor/Enemy.h"
#include "actor/Key.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "component/FocusComponent.h"
#include "system/SceneSystem.h"
#include <iostream>

CameraSystem::CameraSystem(Game* game)
    : mGame(game),
      mCameraYaw(0.0f),
      mCameraPitch(-1.2f),
      mCameraStickY(0.0f),
      mCameraStickX(0.0f),
      mCameraUpVec(0.0f, 1.0f, 0.0f),
      mCameraTargetPos(0.0f),
      mCameraPos(0.0f),
      mIsTargetFocus(false)
{
}

void CameraSystem::ProcessInput()
{
    SDL_GameController* sdlController = mGame->GetSdlController();
    constexpr float deadZone = 0.25f;
    constexpr float scale =
        1.0f / 32767.0f; // SDL_GameControllerGetAxisの範囲が32767までで、scaleをかけて1.0f以内に抑えるため

    mCameraStickX = SDL_GameControllerGetAxis(sdlController, SDL_CONTROLLER_AXIS_RIGHTX) * scale;

    if (std::abs(mCameraStickY) < deadZone)
        mCameraStickY = 0.0f;
    if (std::abs(mCameraStickX) < deadZone)
        mCameraStickX = 0.0f;
}

void CameraSystem::Update(float deltaTime)
{
    UpdateCamera(deltaTime);
}

void CameraSystem::UpdateCamera(float deltaTime)
{
    constexpr float cameraSensitivity = 2.5f;

    const float yawDelta = mCameraStickX * cameraSensitivity * deltaTime;
    const float upSmooth = 1.0f - std::exp(-8.0f * deltaTime);
    const float targetSmooth = 1.0f - std::exp(-10.0f * deltaTime);

    std::vector<Player*> players = mGame->GetPlayers();
    players[0]->SetCameraYaw(yawDelta);
    for (auto player : players) {
        mCameraUpVec = glm::normalize(glm::mix(mCameraUpVec, player->GetUpVec(), upSmooth));
        mCameraTargetPos = glm::mix(mCameraTargetPos, player->GetPos(), targetSmooth);
    }
}

glm::mat4 CameraSystem::GetPlayerView(Player* player, float cameraDistance, bool isFixed)
{
    glm::vec3 toPosX;
    glm::vec3 cameraDir;

    if (isFixed) {
        glm::vec3 facingForwardVec = player->GetFacingForwardVec();
        toPosX = glm::normalize(-facingForwardVec);
        cameraDir = glm::normalize(std::cos(-0.2f) * toPosX + std::sin(-0.2f) * mCameraUpVec);
        mCameraPos = mCameraTargetPos - cameraDir * cameraDistance;
        glm::vec3 offset = glm::normalize(mCameraUpVec) * 1.0f;
        return glm::lookAt(mCameraPos, mCameraTargetPos + offset, mCameraUpVec);
    }

    glm::vec3 forwardVec = player->GetForwardVec();
    toPosX = glm::normalize(-forwardVec);
    cameraDir = glm::normalize(std::cos(mCameraPitch) * toPosX + std::sin(mCameraPitch) * mCameraUpVec);
    mCameraPos = mCameraTargetPos - cameraDir * cameraDistance;
    return glm::lookAt(mCameraPos, mCameraTargetPos, mCameraUpVec);
}

glm::mat4 CameraSystem::GetTargetCameraView(Actor* targetActor)
{
    Player* player = mGame->GetPlayers()[0];

    const glm::vec3 playerPos = player->GetPos();
    const glm::vec3 targetPos = targetActor->GetPos();

    const float cameraLerp = 0.12f;

    glm::vec3 up = glm::normalize(player->GetUpVec());

    const glm::vec3 center = glm::mix(playerPos, targetPos, 0.5f);

    glm::vec3 targetToPlayer = playerPos - targetPos;

    targetToPlayer -= up * glm::dot(targetToPlayer, up);

    if (glm::length(targetToPlayer) < 0.001f) {
        targetToPlayer = -player->GetForwardVec();
        targetToPlayer -= up * glm::dot(targetToPlayer, up);
    }

    if (glm::length(targetToPlayer) < 0.001f) {
        targetToPlayer = glm::cross(up, glm::vec3(1.0f, 0.0f, 0.0f));

        if (glm::length(targetToPlayer) < 0.001f) {
            targetToPlayer = glm::cross(up, glm::vec3(0.0f, 0.0f, 1.0f));
        }
    }

    const glm::vec3 backDir = glm::normalize(targetToPlayer);

    const float targetDistance = glm::length(playerPos - targetPos);
    const float cameraDistance = glm::clamp(targetDistance + 6.0f, 8.0f, 16.0f);
    const float cameraHeight = 4.0f;

    const glm::vec3 desiredCameraPos = center + backDir * cameraDistance + up * cameraHeight;

    const glm::vec3 lookAtBase = glm::mix(playerPos, targetPos, 0.75f);
    const glm::vec3 desiredLookAt = lookAtBase + up * 1.5f;

    mCameraPos = glm::mix(mCameraPos, desiredCameraPos, cameraLerp);
    mCameraTargetPos = glm::mix(mCameraTargetPos, desiredLookAt, cameraLerp);

    mCameraUpVec = glm::normalize(glm::mix(mCameraUpVec, up, cameraLerp));

    return glm::lookAt(mCameraPos, mCameraTargetPos, mCameraUpVec);
}

glm::mat4 CameraSystem::GetFocusView(Actor* focusActor) const
{
    const glm::vec3 upVec = focusActor->GetUpVec();
    glm::vec3 baseLeft = glm::cross(upVec, glm::vec3(0, 0, 1));

    if (glm::length(baseLeft) < 0.01f) {
        baseLeft = glm::normalize(glm::cross(upVec, glm::vec3(0, 1, 0)));
    } else
        baseLeft = glm::normalize(baseLeft);

    const glm::vec3 forwardVec =
        glm::normalize(glm::cross(baseLeft, upVec) * std::cos(0.6f) - std::sin(0.6f) * baseLeft);
    const glm::vec3 back = glm::normalize(-forwardVec);
    const glm::vec3 cameraDir = glm::normalize(std::cos(-0.5f) * back + std::sin(-0.5f) * upVec);

    const glm::vec3 ownerPos = focusActor->GetPos();
    constexpr float cameraDistance = 15.0f;
    const glm::vec3 cameraPos = ownerPos - cameraDir * cameraDistance;

    return glm::lookAt(cameraPos, ownerPos, upVec);
}

std::vector<glm::mat4> CameraSystem::GetViews()
{
    std::vector<glm::mat4> views;
    if (mGame->GetSceneSystem()->IsOpening()) {
        views = GetOpeningViews();
        if (!views.empty()) {
            return views;
        }
    }

    Planet* currentPlanet = mGame->GetPlayers()[0]->GetCurrentPlanet();
    std::vector<Boat*> boats = mGame->GetPlayers()[0]->GetCurrentPlanet()->GetBoats();
    if (!boats.empty()) {
        views = GetBoatFocusViews(boats);
        if (!views.empty()) {
            return views;
        }
    }

    Key* key = currentPlanet->GetKey();
    if (key) {
        if (key->GetFocusComponent()->GetFocusTimer() >= 0.0f) {
            glm::mat4 keyFocusView = GetFocusView(key);
            views.emplace_back(keyFocusView);
            return views;
        }
    }

    if (mGame->GetSceneSystem()->IsStageClear()) {
        glm::mat4 playerFocusView = GetPlayerView(mGame->GetPlayers()[0], 4.0f, true);
        views.emplace_back(playerFocusView);
        return views;
    }

    if (mIsTargetFocus) {
        Enemy* targetEnemy;
        std::vector<Enemy*> enemies = mGame->GetPlayers()[0]->GetCurrentPlanet()->GetEnemies();
        for (auto enemy : enemies) {
            if (!enemy->GetIsBoss()) {
                continue;
            }

            targetEnemy = enemy;
            break;
        }
        views.emplace_back(GetTargetCameraView(targetEnemy));
    }

    glm::mat4 playerView = GetPlayerView(mGame->GetPlayers()[0], 10.0f);
    views.emplace_back(playerView);

    bool isPlayer2Joined = mGame->GetIsPlayer2Joined();
    if (isPlayer2Joined) {
        glm::mat4 player2View = GetPlayerView(mGame->GetPlayers()[1], 10.0f);
        views.emplace_back(player2View);
    }

    return views;
}

std::vector<glm::mat4> CameraSystem::GetOpeningViews() const
{
    std::vector<glm::mat4> views;
    if (mGame->GetSceneSystem()->IsTalkWithMother()) {
        glm::mat4 talkWithMotherView =
            glm::lookAt(glm::vec3(-2.0f, 4.0f, -2.0f), glm::vec3(4.0f, 2.0f, 4.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        views.emplace_back(talkWithMotherView);
        return views;
    }

    if (mGame->GetSceneSystem()->IsTalkWithDoctor()) {
        glm::mat4 talkWithDoctorView =
            glm::lookAt(glm::vec3(3.0f, 4.0f, 1.0f), glm::vec3(-4.0f, 2.0f, -4.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        views.emplace_back(talkWithDoctorView);
        return views;
    }

    return views;
}

std::vector<glm::mat4> CameraSystem::GetBoatFocusViews(std::vector<Boat*> boats) const
{
    std::vector<glm::mat4> views;
    for (auto boat : boats) {
        if (boat->GetFocusComponent()->GetFocusTimer() < 0.0f) {
            continue;
        }

        glm::mat4 view = mGame->GetCameraSystem()->GetFocusView(boat);
        views.emplace_back(view);
        return views;
    }
    return views;
}