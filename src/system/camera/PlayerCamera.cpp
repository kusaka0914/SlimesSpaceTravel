#include "system/camera/PlayerCamera.h"

#include "actor/Enemy.h"
#include "actor/Player.h"
#include "system/camera/CameraCollisionResolver.h"

#include <algorithm>
#include <cmath>
#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace {
constexpr float directionEpsilonSquared = 0.000001f;

bool TryGetTangentDirection(const glm::vec3& direction, const glm::vec3& up, glm::vec3& tangentDirection)
{
    const glm::vec3 projectedDirection = direction - up * glm::dot(direction, up);
    const float lengthSquared = glm::dot(projectedDirection, projectedDirection);
    if (lengthSquared <= directionEpsilonSquared) {
        return false;
    }

    tangentDirection = projectedDirection / std::sqrt(lengthSquared);
    return true;
}

glm::vec3 RotateTowards(const glm::vec3& current, const glm::vec3& desired, const glm::vec3& up,
                        float smoothingSpeed, float deltaTime)
{
    const float dot = glm::clamp(glm::dot(current, desired), -1.0f, 1.0f);
    const float signedAngle = std::atan2(glm::dot(glm::cross(current, desired), up), dot);
    const float blend = 1.0f - std::exp(-std::max(0.0f, smoothingSpeed) * std::max(0.0f, deltaTime));
    const float angle = signedAngle * blend;

    const glm::vec3 rotated =
        current * std::cos(angle) + glm::cross(up, current) * std::sin(angle);

    glm::vec3 tangentDirection;
    return TryGetTangentDirection(rotated, up, tangentDirection) ? tangentDirection : desired;
}
} // namespace

PlayerCamera::PlayerCamera(CameraCollisionResolver& collisionResolver)
    : mCollisionResolver(collisionResolver)
{
}

void PlayerCamera::Update(const std::vector<Player*>& players, float yawDelta, float upSmoothingSpeed,
                          float targetSmoothingSpeed, float attackTargetSmoothingSpeed, float deltaTime)
{
    if (players.empty()) {
        return;
    }

    ResizeState(players.size());

    if (players[0]) {
        players[0]->SetCameraYaw(yawDelta);
    }

    for (int i = 0; i < static_cast<int>(players.size()); ++i) {
        UpdateState(players[i], i, upSmoothingSpeed, targetSmoothingSpeed, attackTargetSmoothingSpeed,
                    deltaTime);
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
        const glm::vec3 forwardVec =
            state.hasCameraForward ? state.cameraForwardVec : player->GetForwardVec();

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
                               float targetSmoothingSpeed, float attackTargetSmoothingSpeed, float deltaTime)
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

    UpdateCameraForward(player, state, attackTargetSmoothingSpeed, deltaTime);
}

void PlayerCamera::UpdateCameraForward(Player* player, PlayerCameraState& state,
                                       float attackTargetSmoothingSpeed, float deltaTime)
{
    glm::vec3 up = state.upVec;
    const float upLengthSquared = glm::dot(up, up);
    if (upLengthSquared <= directionEpsilonSquared) {
        up = glm::vec3(0.0f, 1.0f, 0.0f);
    } else {
        up /= std::sqrt(upLengthSquared);
    }

    glm::vec3 normalForward;
    if (!TryGetTangentDirection(player->GetForwardVec(), up, normalForward)) {
        if (!TryGetTangentDirection(player->GetFacingForwardVec(), up, normalForward)) {
            return;
        }
    }

    if (!state.hasCameraForward) {
        state.cameraForwardVec = normalForward;
        state.hasCameraForward = true;
    }

    Enemy* attackTarget = player->GetAttackDirectionTarget();
    const bool hasValidAttackTarget =
        attackTarget && attackTarget->GetIsActive() && attackTarget->IsAlive() &&
        !attackTarget->GetIsDead() && attackTarget->GetCurrentPlanet() == player->GetCurrentPlanet();

    glm::vec3 targetForward;
    // This vector points from the player toward the camera, so the enemy direction is inverted.
    if (hasValidAttackTarget &&
        TryGetTangentDirection(player->GetPos() - attackTarget->GetPos(), up, targetForward)) {
        state.attackTargetForwardVec = targetForward;
        state.hasAttackTargetForward = true;
    }

    const bool shouldAssistAttack = state.hasAttackTargetForward && player->IsAttacking();
    if (!player->IsAttacking() && !hasValidAttackTarget && state.hasAttackTargetForward) {
        state.hasAttackTargetForward = false;
    }

    if (shouldAssistAttack) {
        state.cameraForwardVec = RotateTowards(
            state.cameraForwardVec, state.attackTargetForwardVec, up, attackTargetSmoothingSpeed, deltaTime);
        player->SetCameraForwardDirection(state.cameraForwardVec, up);
        return;
    }

    state.cameraForwardVec = normalForward;
}
