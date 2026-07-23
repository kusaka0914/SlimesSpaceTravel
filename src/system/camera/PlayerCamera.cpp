#include "system/camera/PlayerCamera.h"

#include "actor/Player.h"
#include "system/camera/CameraCollisionResolver.h"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

PlayerCamera::PlayerCamera(CameraCollisionResolver& collisionResolver)
    : mCollisionResolver(collisionResolver)
{
}

void PlayerCamera::Update(const std::vector<Player*>& players, float yawDelta, float upSmoothingSpeed,
                          float targetSmoothingSpeed, float deltaTime)
{
    if (players.empty()) {
        return;
    }

    ResizeState(players.size());

    if (players[0]) {
        players[0]->SetCameraYaw(yawDelta);
    }

    for (int i = 0; i < static_cast<int>(players.size()); ++i) {
        UpdateState(players[i], i, upSmoothingSpeed, targetSmoothingSpeed, deltaTime);
    }
}

glm::mat4 PlayerCamera::GetView(Player* player, int playerIndex, float cameraDistance, float cameraPitch,
                                float targetHeight, bool isFixed)
{
    if (!player) {
        return glm::mat4(1.0f);
    }

    ResizeState(playerIndex + 1);

    PlayerCameraState& state = mStates[playerIndex];

    glm::vec3 toPosX;
    glm::vec3 cameraDir;
    glm::vec3 lookAtOffset;

    if (isFixed) {
        const glm::vec3 facingForwardVec = player->GetFacingForwardVec();

        toPosX = glm::normalize(-facingForwardVec);
        cameraDir = glm::normalize(std::cos(-0.2f) * toPosX + std::sin(-0.2f) * state.upVec);

        lookAtOffset = glm::normalize(state.upVec) * 1.0f;
    } else {
        const glm::vec3 forwardVec = player->GetForwardVec();

        toPosX = glm::normalize(-forwardVec);
        cameraDir = glm::normalize(std::cos(cameraPitch) * toPosX + std::sin(cameraPitch) * state.upVec);

        lookAtOffset = glm::normalize(state.upVec) * targetHeight;
    }

    const glm::vec3 lookAtPos = state.targetPos + lookAtOffset;
    const glm::vec3 desiredCameraPos = state.targetPos - cameraDir * cameraDistance;

    state.cameraPos = mCollisionResolver.Resolve(lookAtPos, desiredCameraPos);

    return glm::lookAt(state.cameraPos, lookAtPos, state.upVec);
}

glm::vec3 PlayerCamera::GetCameraPos(int playerIndex) const
{
    if (playerIndex < 0 || playerIndex >= static_cast<int>(mStates.size())) {
        return glm::vec3(0.0f);
    }

    return mStates[playerIndex].cameraPos;
}

void PlayerCamera::ResizeState(std::size_t count)
{
    if (mStates.size() >= count) {
        return;
    }

    mStates.resize(count);
}

void PlayerCamera::UpdateState(Player* player, int playerIndex, float upSmoothingSpeed,
                               float targetSmoothingSpeed, float deltaTime)
{
    if (!player) {
        return;
    }

    ResizeState(playerIndex + 1);

    PlayerCameraState& state = mStates[playerIndex];

    const float upSmooth = 1.0f - std::exp(-upSmoothingSpeed * deltaTime);
    const float targetSmooth = 1.0f - std::exp(-targetSmoothingSpeed * deltaTime);

    state.upVec = glm::normalize(glm::mix(state.upVec, player->GetUpVec(), upSmooth));
    state.targetPos = glm::mix(state.targetPos, player->GetPos(), targetSmooth);
}
