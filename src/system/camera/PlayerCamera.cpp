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
                          float targetSmoothingSpeed, float attackTargetSmoothingSpeed, float deltaTime,
                          int yawPlayerIndex)
{
    if (players.empty()) {
        return;
    }

    ResizeState(players.size());

    if (yawPlayerIndex >= 0 &&
        yawPlayerIndex < static_cast<int>(players.size()) &&
        players[static_cast<std::size_t>(yawPlayerIndex)]) {
        // Manual camera input must always take priority over the automatic
        // align-behind transition. Otherwise UpdateCameraForward overwrites
        // the yaw every frame until the transition happens to complete.
        constexpr float manualYawEpsilon = 0.000001f;
        if (std::abs(yawDelta) > manualYawEpsilon) {
            mStates[static_cast<std::size_t>(yawPlayerIndex)].isAligningBehindPlayer = false;
        }
        players[static_cast<std::size_t>(yawPlayerIndex)]->SetCameraYaw(yawDelta);
    }

    for (int i = 0; i < static_cast<int>(players.size()); ++i) {
        UpdateState(players[i], i, upSmoothingSpeed, targetSmoothingSpeed, attackTargetSmoothingSpeed,
                    deltaTime);
    }
}

void PlayerCamera::Reset()
{
    mStates.clear();
}

void PlayerCamera::SnapToPlayer(
    Player* player,
    int playerIndex)
{
    if (!player || playerIndex < 0) {
        return;
    }

    ResizeState(static_cast<std::size_t>(playerIndex + 1));

    PlayerCameraState& state =
        mStates[static_cast<std::size_t>(playerIndex)];
    state.targetPos = player->GetPos();

    const glm::vec3 playerUpDirection = player->GetUpVec();
    const float upLengthSquared =
        glm::dot(playerUpDirection, playerUpDirection);
    state.upVec =
        upLengthSquared > directionEpsilonSquared
            ? playerUpDirection / std::sqrt(upLengthSquared)
            : glm::vec3(0.0f, 1.0f, 0.0f);

    glm::vec3 cameraForwardDirection;
    if (TryGetTangentDirection(
            player->GetForwardVec(),
            state.upVec,
            cameraForwardDirection)) {
        state.cameraForwardVec = cameraForwardDirection;
        state.hasCameraForward = true;
    } else {
        state.hasCameraForward = false;
    }

    state.hasAttackTargetForward = false;
    state.isAligningBehindPlayer = false;
}

void PlayerCamera::SnapBehindPlayer(Player* player, int playerIndex)
{
    if (!player || playerIndex < 0) {
        return;
    }

    ResizeState(static_cast<std::size_t>(playerIndex + 1));

    PlayerCameraState& state =
        mStates[static_cast<std::size_t>(playerIndex)];
    state.targetPos = player->GetPos();

    const glm::vec3 playerUpDirection = player->GetUpVec();
    const float upLengthSquared =
        glm::dot(playerUpDirection, playerUpDirection);
    state.upVec =
        upLengthSquared > directionEpsilonSquared
            ? playerUpDirection / std::sqrt(upLengthSquared)
            : glm::vec3(0.0f, 1.0f, 0.0f);

    glm::vec3 cameraForwardDirection;
    if (!TryGetTangentDirection(
            -player->GetFacingForwardVec(),
            state.upVec,
            cameraForwardDirection)) {
        state.hasCameraForward = false;
        state.hasAttackTargetForward = false;
        state.isAligningBehindPlayer = false;
        return;
    }

    state.cameraForwardVec = cameraForwardDirection;
    state.hasCameraForward = true;
    state.hasAttackTargetForward = false;
    state.isAligningBehindPlayer = false;
    player->SetCameraForwardDirection(
        cameraForwardDirection,
        state.upVec);
}

void PlayerCamera::AlignBehindPlayer(Player* player, int playerIndex)
{
    if (!player || playerIndex < 0) {
        return;
    }

    ResizeState(playerIndex + 1);

    PlayerCameraState& state = mStates[playerIndex];
    glm::vec3 cameraForward;
    if (!TryGetTangentDirection(-player->GetFacingForwardVec(), player->GetUpVec(), cameraForward)) {
        return;
    }

    state.alignTargetForwardVec = cameraForward;
    state.isAligningBehindPlayer = true;
    state.hasAttackTargetForward = false;
}

void PlayerCamera::BlendBehindTarget(Player* player, int playerIndex, const glm::vec3& targetPosition, float blend)
{
    if (!player || playerIndex < 0) {
        return;
    }

    ResizeState(playerIndex + 1);

    PlayerCameraState& state = mStates[playerIndex];

    glm::vec3 up = state.upVec;
    const float upLengthSquared = glm::dot(up, up);
    if (upLengthSquared <= directionEpsilonSquared) {
        return;
    }
    up /= std::sqrt(upLengthSquared);

    glm::vec3 currentForward;
    if (!TryGetTangentDirection(state.cameraForwardVec, up, currentForward) &&
        !TryGetTangentDirection(player->GetForwardVec(), up, currentForward)) {
        return;
    }

    // cameraForwardVec points from the player toward the camera, so place the
    // camera on the side opposite the conversation target.
    glm::vec3 targetForward;
    if (!TryGetTangentDirection(player->GetPos() - targetPosition, up, targetForward)) {
        return;
    }

    const float clampedBlend = glm::clamp(blend, 0.0f, 1.0f);
    const float dotValue = glm::clamp(glm::dot(currentForward, targetForward), -1.0f, 1.0f);
    const float signedAngle =
        std::atan2(glm::dot(glm::cross(currentForward, targetForward), up), dotValue);
    const float angle = signedAngle * clampedBlend;
    const glm::vec3 rotated =
        currentForward * std::cos(angle) + glm::cross(up, currentForward) * std::sin(angle);

    if (!TryGetTangentDirection(rotated, up, state.cameraForwardVec)) {
        state.cameraForwardVec = targetForward;
    }
    state.hasCameraForward = true;
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

    if (state.isAligningBehindPlayer) {
        // The player's up direction changes while moving over a sphere.
        // Keep both directions on the current tangent plane so the completion
        // dot product can still reach its threshold after that change.
        glm::vec3 currentForward;
        glm::vec3 alignTargetForward;
        if (!TryGetTangentDirection(state.cameraForwardVec, up, currentForward) ||
            !TryGetTangentDirection(state.alignTargetForwardVec, up, alignTargetForward)) {
            state.isAligningBehindPlayer = false;
            state.cameraForwardVec = normalForward;
            return;
        }

        state.cameraForwardVec = currentForward;
        state.alignTargetForwardVec = alignTargetForward;

        constexpr float alignSmoothingSpeed = 8.0f;
        state.cameraForwardVec = RotateTowards(
            state.cameraForwardVec, state.alignTargetForwardVec, up, alignSmoothingSpeed, deltaTime);
        player->SetCameraForwardDirection(state.cameraForwardVec, up);

        constexpr float alignmentCompleteDot = 0.9995f;
        if (glm::dot(state.cameraForwardVec, state.alignTargetForwardVec) >= alignmentCompleteDot) {
            state.cameraForwardVec = state.alignTargetForwardVec;
            player->SetCameraForwardDirection(state.cameraForwardVec, up);
            state.isAligningBehindPlayer = false;
        }
        return;
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
