#include "system/camera/FocusCamera.h"

#include "Game.h"
#include "actor/Actor.h"
#include "actor/Boat.h"
#include "actor/Player.h"
#include "component/FocusComponent.h"
#include "system/SceneSystem.h"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

FocusCamera::FocusCamera(Game* game)
    : mGame(game)
{
}

glm::mat4 FocusCamera::GetFocusView(Actor* focusActor) const
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

glm::mat4 FocusCamera::GetTargetCameraView(Actor* targetActor)
{
    if (!mGame || !targetActor) {
        return glm::mat4(1.0f);
    }

    const std::vector<Player*>& players = mGame->GetPlayers();
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

std::vector<glm::mat4> FocusCamera::GetOpeningViews() const
{
    std::vector<glm::mat4> views;

    if (!mGame || !mGame->GetSceneSystem()) {
        return views;
    }

    if (mGame->GetSceneSystem()->IsTalkWithMother()) {
        const glm::mat4 talkWithMotherView =
            glm::lookAt(glm::vec3(-2.0f, 4.0f, -2.0f), glm::vec3(4.0f, 2.0f, 4.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        views.emplace_back(talkWithMotherView);
        return views;
    }

    if (mGame->GetSceneSystem()->IsTalkWithDoctor()) {
        const glm::mat4 talkWithDoctorView =
            glm::lookAt(glm::vec3(3.0f, 4.0f, 1.0f), glm::vec3(-4.0f, 2.0f, -4.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        views.emplace_back(talkWithDoctorView);
        return views;
    }

    return views;
}

std::vector<glm::mat4> FocusCamera::GetBoatFocusViews(const std::vector<Boat*>& boats) const
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
