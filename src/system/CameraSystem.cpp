#include "CameraSystem.h"
#include "Game.h"
#include "actor/Actor.h"
#include "actor/Boat.h"
#include "actor/Enemy.h"
#include "actor/Key.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "component/FocusComponent.h"
#include "system/PhysicsSystem.h"
#include "system/SceneSystem.h"
#include <btBulletDynamicsCommon.h>
#include <cmath>

CameraSystem::CameraSystem(Game* game)
    : mGame(game),
      mCameraPitch(-1.0f),
      mCameraStickX(0.0f),
      mCameraUpVec(0.0f, 1.0f, 0.0f),
      mCameraTargetPos(0.0f),
      mCameraPos(0.0f),
      mIsTargetFocus(false),
      mMoveForward(0.0f),
      mMoveRight(0.0f),
      mMoveUp(0.0f),
      mDebugCameraPitch(0.0f),
      mDebugCameraYaw(0.0f),
      mDebugYawInput(0.0f),
      mDebugPitchInput(0.0f)
{
}

void CameraSystem::ProcessInput()
{
    if (mGame->GetIsFreeCameraMode()) {
        GLFWwindow* window = mGame->GetWindow();

        mMoveForward = 0.0f;
        mMoveRight = 0.0f;
        mMoveUp = 0.0f;
        mDebugYawInput = 0.0f;
        mDebugPitchInput = 0.0f;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            mMoveForward += 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            mMoveForward -= 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            mMoveRight -= 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            mMoveRight += 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            mMoveUp += 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
            mMoveUp -= 1.0f;
        }

        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            mDebugYawInput += 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            mDebugYawInput -= 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
            mDebugPitchInput += 1.0f;
        }
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            mDebugPitchInput -= 1.0f;
        }

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

void CameraSystem::UpdateCamera(float deltaTime)
{
    if (mGame->GetIsFreeCameraMode()) {
        constexpr float rotateSpeed = 2.0f;

        mDebugCameraYaw += mDebugYawInput * rotateSpeed * deltaTime;
        mDebugCameraPitch += mDebugPitchInput * rotateSpeed * deltaTime;

        glm::vec3 forward;
        forward.x = std::cos(mDebugCameraPitch) * std::sin(mDebugCameraYaw);
        forward.y = std::sin(mDebugCameraPitch);
        forward.z = std::cos(mDebugCameraPitch) * std::cos(mDebugCameraYaw);
        forward = glm::normalize(forward);

        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

        constexpr float moveSpeed = 10.0f;

        mCameraPos += forward * mMoveForward * moveSpeed * deltaTime + right * mMoveRight * moveSpeed * deltaTime +
                      up * mMoveUp * moveSpeed * deltaTime;

        mCameraUpVec = up;
        mCameraTargetPos = mCameraPos + forward;

        return;
    }

    constexpr float cameraSensitivity = 2.5f;
    const float yawDelta = mCameraStickX * cameraSensitivity * deltaTime;

    std::vector<Player*> players = mGame->GetPlayers();
    if (players.empty()) {
        return;
    }

    ResizePlayerCameraState(players.size());

    if (players[0]) {
        players[0]->SetCameraYaw(yawDelta);
    }

    for (int i = 0; i < static_cast<int>(players.size()); i++) {
        UpdatePlayerCameraState(players[i], i, deltaTime);
    }
}

glm::mat4 CameraSystem::GetPlayerView(Player* player, int playerIndex, float cameraDistance, bool isFixed)
{
    if (!player) {
        return glm::mat4(1.0f);
    }

    ResizePlayerCameraState(playerIndex + 1);

    PlayerCameraState& playerCameraState = mPlayerCameraStates[playerIndex];

    glm::vec3 toPosX;
    glm::vec3 cameraDir;
    glm::vec3 lookAtOffset;

    if (isFixed) {
        glm::vec3 facingForwardVec = player->GetFacingForwardVec();

        toPosX = glm::normalize(-facingForwardVec);
        cameraDir = glm::normalize(std::cos(-0.2f) * toPosX + std::sin(-0.2f) * playerCameraState.upVec);

        lookAtOffset = glm::normalize(playerCameraState.upVec) * 1.0f;
    } else {
        glm::vec3 forwardVec = player->GetForwardVec();

        toPosX = glm::normalize(-forwardVec);
        cameraDir = glm::normalize(std::cos(mCameraPitch) * toPosX + std::sin(mCameraPitch) * playerCameraState.upVec);

        lookAtOffset = glm::normalize(playerCameraState.upVec) * 1.5f;
    }

    glm::vec3 lookAtPos = playerCameraState.targetPos + lookAtOffset;
    glm::vec3 desiredCameraPos = playerCameraState.targetPos - cameraDir * cameraDistance;

    playerCameraState.cameraPos = ResolveCameraCollision(lookAtPos, desiredCameraPos);

    return glm::lookAt(playerCameraState.cameraPos, lookAtPos, playerCameraState.upVec);
}

glm::vec3 CameraSystem::ResolveCameraCollision(const glm::vec3& targetPos, const glm::vec3& desiredCameraPos) const
{
    btDiscreteDynamicsWorld* bulletWorld = mGame->GetPhysicsSystem()->GetBulletWorld();
    if (!bulletWorld) {
        return desiredCameraPos;
    }

    glm::vec3 from = targetPos;
    glm::vec3 to = desiredCameraPos;

    btCollisionWorld::ClosestRayResultCallback cb(btVector3(from.x, from.y, from.z), btVector3(to.x, to.y, to.z));

    bulletWorld->rayTest(cb.m_rayFromWorld, cb.m_rayToWorld, cb);

    if (!cb.hasHit()) {
        return desiredCameraPos;
    }

    glm::vec3 hitPos(cb.m_hitPointWorld.x(), cb.m_hitPointWorld.y(), cb.m_hitPointWorld.z());

    glm::vec3 dir = desiredCameraPos - targetPos;
    if (glm::length(dir) < 1e-5f) {
        return desiredCameraPos;
    }

    dir = glm::normalize(dir);

    constexpr float cameraCollisionMargin = 0.3f;

    return hitPos - dir * cameraCollisionMargin;
}

glm::mat4 CameraSystem::GetTargetCameraView(Actor* targetActor)
{
    if (!targetActor) {
        return glm::mat4(1.0f);
    }

    std::vector<Player*> players = mGame->GetPlayers();
    if (players.empty() || !players[0]) {
        return glm::mat4(1.0f);
    }

    Player* player = players[0];

    const glm::vec3 playerPos = player->GetPos();
    const glm::vec3 targetPos = targetActor->GetPos();

    constexpr float cameraLerp = 0.12f;

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
    constexpr float cameraHeight = 4.0f;

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
    if (!focusActor) {
        return glm::mat4(1.0f);
    }

    const glm::vec3 upVec = focusActor->GetUpVec();
    glm::vec3 baseLeft = glm::cross(upVec, glm::vec3(0.0f, 0.0f, 1.0f));

    if (glm::length(baseLeft) < 0.01f) {
        baseLeft = glm::normalize(glm::cross(upVec, glm::vec3(0.0f, 1.0f, 0.0f)));
    } else {
        baseLeft = glm::normalize(baseLeft);
    }

    const glm::vec3 forwardVec =
        glm::normalize(glm::cross(baseLeft, upVec) * std::cos(0.6f) - std::sin(0.6f) * baseLeft);

    const glm::vec3 back = glm::normalize(-forwardVec);
    const glm::vec3 cameraDir = glm::normalize(std::cos(-0.5f) * back + std::sin(-0.5f) * upVec);

    const glm::vec3 ownerPos = focusActor->GetPos();

    constexpr float cameraDistance = 15.0f;
    const glm::vec3 cameraPos = ownerPos - cameraDir * cameraDistance;

    return glm::lookAt(cameraPos, ownerPos, upVec);
}

glm::mat4 CameraSystem::GetDebugCameraView()
{
    return glm::lookAt(mCameraPos, mCameraTargetPos, mCameraUpVec);
}

std::vector<glm::mat4> CameraSystem::GetViews()
{
    std::vector<glm::mat4> views;

    if (mGame->GetIsFreeCameraMode()) {
        views.emplace_back(GetDebugCameraView());
        return views;
    }

    std::vector<Player*> players = mGame->GetPlayers();
    if (players.empty() || !players[0]) {
        return views;
    }

    if (mGame->GetSceneSystem()->IsOpening()) {
        views = GetOpeningViews();
        if (!views.empty()) {
            return views;
        }
    }

    Planet* currentPlanet = players[0]->GetCurrentPlanet();
    if (!currentPlanet) {
        return views;
    }

    std::vector<Boat*> boats = currentPlanet->GetBoats();
    if (!boats.empty()) {
        views = GetBoatFocusViews(boats);
        if (!views.empty()) {
            return views;
        }
    }

    Key* key = currentPlanet->GetKey();
    if (key) {
        FocusComponent* focusComponent = key->GetFocusComponent();
        if (focusComponent && focusComponent->GetFocusTimer() >= 0.0f) {
            views.emplace_back(GetFocusView(key));
            return views;
        }
    }

    if (mGame->GetSceneSystem()->IsStageClear()) {
        views.emplace_back(GetPlayerView(players[0], 0, 6.0f, true));
        return views;
    }

    if (mIsTargetFocus) {
        Enemy* targetEnemy = nullptr;

        std::vector<Enemy*> enemies = currentPlanet->GetEnemies();
        for (Enemy* enemy : enemies) {
            if (!enemy || !enemy->GetIsBoss()) {
                continue;
            }

            targetEnemy = enemy;
            break;
        }

        if (targetEnemy) {
            views.emplace_back(GetTargetCameraView(targetEnemy));
            return views;
        }
    }

    views.emplace_back(GetPlayerView(players[0], 0, 8.0f));

    const bool isPlayer2Joined = mGame->GetIsPlayer2Joined() && players.size() >= 2 && players[1];
    if (isPlayer2Joined) {
        views.emplace_back(GetPlayerView(players[1], 1, 8.0f));
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

std::vector<glm::mat4> CameraSystem::GetBoatFocusViews(const std::vector<Boat*>& boats) const
{
    std::vector<glm::mat4> views;

    for (Boat* boat : boats) {
        if (!boat) {
            continue;
        }

        FocusComponent* focusComponent = boat->GetFocusComponent();
        if (!focusComponent || focusComponent->GetFocusTimer() < 0.0f) {
            continue;
        }

        views.emplace_back(GetFocusView(boat));
        return views;
    }

    return views;
}

void CameraSystem::ResizePlayerCameraState(std::size_t count)
{
    if (mPlayerCameraStates.size() >= count) {
        return;
    }

    mPlayerCameraStates.resize(count);
}

void CameraSystem::UpdatePlayerCameraState(Player* player, int playerIndex, float deltaTime)
{
    if (!player) {
        return;
    }

    ResizePlayerCameraState(playerIndex + 1);

    PlayerCameraState& playerCameraState = mPlayerCameraStates[playerIndex];

    const float upSmooth = 1.0f - std::exp(-8.0f * deltaTime);
    const float targetSmooth = 1.0f - std::exp(-10.0f * deltaTime);

    playerCameraState.upVec = glm::normalize(glm::mix(playerCameraState.upVec, player->GetUpVec(), upSmooth));

    playerCameraState.targetPos = glm::mix(playerCameraState.targetPos, player->GetPos(), targetSmooth);
}