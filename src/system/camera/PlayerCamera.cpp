#include "system/camera/PlayerCamera.h"

#include "actor/Enemy.h"
#include "actor/Player.h"
#include "system/camera/CameraCollisionResolver.h"
#include "system/camera/PlayerCameraSettings.h"

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

glm::vec3 RotateBetween(const glm::vec3& start, const glm::vec3& target,
                        const glm::vec3& up, float progress)
{
    const float dot = glm::clamp(glm::dot(start, target), -1.0f, 1.0f);
    const float signedAngle = std::atan2(glm::dot(glm::cross(start, target), up), dot);
    const float angle = signedAngle * glm::clamp(progress, 0.0f, 1.0f);

    const glm::vec3 rotated =
        start * std::cos(angle) + glm::cross(up, start) * std::sin(angle);

    glm::vec3 tangentDirection;
    return TryGetTangentDirection(rotated, up, tangentDirection) ? tangentDirection : target;
}

glm::vec3 RotateAroundUp(
    const glm::vec3& direction,
    const glm::vec3& up,
    float angleRadians)
{
    const glm::vec3 rotated =
        direction * std::cos(angleRadians) +
        glm::cross(up, direction) * std::sin(angleRadians);

    glm::vec3 tangentDirection;
    return TryGetTangentDirection(rotated, up, tangentDirection)
        ? tangentDirection
        : direction;
}

glm::vec3 RotateTowards(const glm::vec3& current, const glm::vec3& desired, const glm::vec3& up,
                        float smoothingSpeed, float deltaTime)
{
    const float blend = 1.0f - std::exp(
        -std::max(0.0f, smoothingSpeed) * std::max(0.0f, deltaTime));
    return RotateBetween(current, desired, up, blend);
}

void ResetAutoFollowInputTracking(PlayerCameraState& state)
{
    state.lateralInputHoldSeconds = 0.0f;
    state.trackedLateralInputSign = 0.0f;
    state.hasTriggeredAutoFollowForCurrentLateralInput = false;
}

void CancelAutomaticFollow(PlayerCameraState& state)
{
    state.isAutoFollowingBehindPlayer = false;
    state.isSurfaceTraversalAutoAligning = false;
    state.autoFollowElapsedSeconds = 0.0f;
    state.autoFollowStartOffsetRadians = 0.0f;
    ResetAutoFollowInputTracking(state);
}

bool StartAutomaticFollowBehindPlayer(
    Player& player,
    PlayerCameraState& state)
{
    glm::vec3 startForward;
    glm::vec3 targetForward;
    if (!TryGetTangentDirection(
            state.cameraForwardVec,
            state.upVec,
            startForward) ||
        !TryGetTangentDirection(
            -player.GetFacingForwardVec(),
            state.upVec,
            targetForward)) {
        return false;
    }

    const float targetToStartDot = glm::clamp(
        glm::dot(targetForward, startForward), -1.0f, 1.0f);
    state.autoFollowStartForwardVec = startForward;
    state.autoFollowTargetForwardVec = targetForward;
    state.autoFollowStartOffsetRadians = std::atan2(
        glm::dot(glm::cross(targetForward, startForward), state.upVec),
        targetToStartDot);
    state.autoFollowElapsedSeconds = 0.0f;
    state.isAutoFollowingBehindPlayer = true;
    return true;
}

void ResetSurfaceTraversalTracking(PlayerCameraState& state)
{
    state.hasSurfaceTraversalStartUp = false;
    state.hasAutoAlignedForCurrentSurfaceTraversal = false;
}
}

PlayerCamera::PlayerCamera(CameraCollisionResolver& collisionResolver)
    : mCollisionResolver(collisionResolver)
{
}

void PlayerCamera::Update(const std::vector<Player*>& players,
                          const std::vector<float>& yawDeltas,
                          const PlayerCameraSettings& settings, float deltaTime,
                          bool allowsMovementCameraAssist)
{
    if (players.empty()) {
        return;
    }

    ResizeState(players.size());

    for (std::size_t playerIndex = 0; playerIndex < players.size(); ++playerIndex) {
        Player* player = players[playerIndex];
        if (!player) {
            continue;
        }

        const float yawDelta =
            playerIndex < yawDeltas.size() ? yawDeltas[playerIndex] : 0.0f;
        PlayerCameraState& state = mStates[playerIndex];
        constexpr float manualYawEpsilon = 0.000001f;
        const float appliedManualYawMagnitude = std::max(
            std::abs(yawDelta), std::abs(player->GetCameraYaw()));
        if (appliedManualYawMagnitude > manualYawEpsilon) {
            state.isAligningBehindPlayer = false;
            CancelAutomaticFollow(state);
            state.autoFollowDelayRemainingSeconds =
                settings.autoFollowDelayAfterManualInputSeconds;
        }

        player->SetCameraYaw(yawDelta);
        UpdateState(player, static_cast<int>(playerIndex), settings,
                    deltaTime, allowsMovementCameraAssist);
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
    CancelAutomaticFollow(state);
    state.isBackwardFacingFramingActive = false;
    state.autoFollowDelayRemainingSeconds = 0.0f;
    state.backwardMovementHoldSeconds = 0.0f;
    ResetSurfaceTraversalTracking(state);
    state.backwardFacingLookAheadDistance = 0.0f;
}

void PlayerCamera::TransitionToPlayer(
    int fromPlayerIndex,
    Player* player,
    int toPlayerIndex)
{
    if (!player || toPlayerIndex < 0) {
        return;
    }

    if (fromPlayerIndex < 0 ||
        fromPlayerIndex >= static_cast<int>(mStates.size()) ||
        fromPlayerIndex == toPlayerIndex) {
        SnapToPlayer(player, toPlayerIndex);
        return;
    }

    const PlayerCameraState previousState =
        mStates[static_cast<std::size_t>(fromPlayerIndex)];
    ResizeState(static_cast<std::size_t>(toPlayerIndex + 1));

    PlayerCameraState& state =
        mStates[static_cast<std::size_t>(toPlayerIndex)];
    state = previousState;
    state.hasAttackTargetForward = false;
    state.isAligningBehindPlayer = false;
    CancelAutomaticFollow(state);
    ResetSurfaceTraversalTracking(state);

    if (state.hasCameraForward) {
        player->SetCameraForwardDirection(
            state.cameraForwardVec,
            state.upVec);
    }
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
        CancelAutomaticFollow(state);
        state.isBackwardFacingFramingActive = false;
        state.autoFollowDelayRemainingSeconds = 0.0f;
        state.backwardMovementHoldSeconds = 0.0f;
        ResetSurfaceTraversalTracking(state);
        state.backwardFacingLookAheadDistance = 0.0f;
        return;
    }

    state.cameraForwardVec = cameraForwardDirection;
    state.hasCameraForward = true;
    state.hasAttackTargetForward = false;
    state.isAligningBehindPlayer = false;
    CancelAutomaticFollow(state);
    state.isBackwardFacingFramingActive = false;
    state.autoFollowDelayRemainingSeconds = 0.0f;
    state.backwardMovementHoldSeconds = 0.0f;
    ResetSurfaceTraversalTracking(state);
    state.backwardFacingLookAheadDistance = 0.0f;
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

    CancelAutomaticFollow(state);
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

        constexpr float maximumLookAheadFractionOfHorizontalCameraDistance = 0.8f;
        const float horizontalCameraDistance =
            cameraDistance * std::abs(glm::dot(cameraDir, toPosX));
        const float maximumLookAheadDistance =
            horizontalCameraDistance * maximumLookAheadFractionOfHorizontalCameraDistance;
        const float lookAheadDistance = std::min(
            state.backwardFacingLookAheadDistance,
            maximumLookAheadDistance);
        lookAtOffset =
            glm::normalize(state.upVec) * targetHeight +
            glm::normalize(forwardVec) * lookAheadDistance;
    }

    const glm::vec3 lookAtPos = state.targetPos + lookAtOffset;
    const glm::vec3 desiredCameraPos =
        state.targetPos - cameraDir * cameraDistance;

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

void PlayerCamera::UpdateState(Player* player, int playerIndex,
                               const PlayerCameraSettings& settings, float deltaTime,
                               bool allowsMovementCameraAssist)
{
    if (!player) {
        return;
    }

    ResizeState(playerIndex + 1);

    PlayerCameraState& state = mStates[playerIndex];

    if (player->ConsumeCameraAutoAlignCancellationRequest()) {
        CancelAutomaticFollow(state);
        ResetSurfaceTraversalTracking(state);
    }

    const float upSmooth = 1.0f - std::exp(-settings.upSmoothingSpeed * deltaTime);
    const float targetSmooth = 1.0f - std::exp(-settings.targetSmoothingSpeed * deltaTime);

    state.upVec = glm::normalize(glm::mix(state.upVec, player->GetUpVec(), upSmooth));
    state.targetPos = glm::mix(state.targetPos, player->GetPos(), targetSmooth);

    state.autoFollowDelayRemainingSeconds = std::max(
        0.0f, state.autoFollowDelayRemainingSeconds - std::max(0.0f, deltaTime));

    UpdateAutoFollowRequest(
        player, state, settings, deltaTime, allowsMovementCameraAssist);

    UpdateCameraForward(player, state, settings.attackTargetSmoothingSpeed,
                        settings.autoFollowRotationDurationSeconds, deltaTime);
    UpdateBackwardFacingFraming(
        player, state, settings, deltaTime, allowsMovementCameraAssist);
    UpdateSurfaceTraversalAutoAlign(
        player, state, settings, allowsMovementCameraAssist);
}

void PlayerCamera::UpdateAutoFollowRequest(
    Player* player,
    PlayerCameraState& state,
    const PlayerCameraSettings& settings,
    float deltaTime,
    bool allowsMovementCameraAssist)
{
    const float lateralInput = player->GetMoveLeftInput();
    const bool hasLateralInput =
        std::abs(lateralInput) >= settings.autoFollowMinimumLateralInput;
    const float backwardInput = std::max(0.0f, player->GetMoveForwardInput());
    const bool hasStrongBackwardInput =
        backwardInput > settings.autoFollowMaximumBackwardInput;
    const bool isAutoFollowBlocked =
        !allowsMovementCameraAssist ||
        state.autoFollowDelayRemainingSeconds > 0.0f ||
        (hasStrongBackwardInput &&
         !state.isSurfaceTraversalAutoAligning);

    if (isAutoFollowBlocked) {
        CancelAutomaticFollow(state);
        return;
    }

    if (state.isSurfaceTraversalAutoAligning) {
        return;
    }

    if (!hasLateralInput) {
        ResetAutoFollowInputTracking(state);
        return;
    }

    float lateralInputSign = 1.0f;
    if (lateralInput < 0.0f) {
        lateralInputSign = -1.0f;
    }

    const bool didLateralDirectionChange =
        state.trackedLateralInputSign != lateralInputSign;
    if (didLateralDirectionChange) {
        CancelAutomaticFollow(state);
        state.trackedLateralInputSign = lateralInputSign;
    }

    if (state.hasTriggeredAutoFollowForCurrentLateralInput) {
        return;
    }

    state.lateralInputHoldSeconds += std::max(0.0f, deltaTime);
    if (state.lateralInputHoldSeconds < settings.autoFollowDelaySeconds) {
        return;
    }

    if (!StartAutomaticFollowBehindPlayer(*player, state)) {
        return;
    }

    state.isSurfaceTraversalAutoAligning = false;
    state.hasTriggeredAutoFollowForCurrentLateralInput = true;
}

void PlayerCamera::UpdateCameraForward(Player* player, PlayerCameraState& state,
                                       float attackTargetSmoothingSpeed,
                                       float autoFollowRotationDurationSeconds,
                                       float deltaTime)
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
        state.isAutoFollowingBehindPlayer = false;
        state.isSurfaceTraversalAutoAligning = false;
        state.autoFollowElapsedSeconds = 0.0f;
        state.cameraForwardVec = RotateTowards(
            state.cameraForwardVec, state.attackTargetForwardVec, up, attackTargetSmoothingSpeed, deltaTime);
        player->SetCameraForwardDirection(state.cameraForwardVec, up);
        return;
    }

    if (state.isAutoFollowingBehindPlayer) {
        state.autoFollowElapsedSeconds += std::max(0.0f, deltaTime);
        const float rotationProgress = glm::clamp(
            state.autoFollowElapsedSeconds /
                std::max(0.05f, autoFollowRotationDurationSeconds),
            0.0f,
            1.0f);
        const float easedRotationProgress =
            rotationProgress * rotationProgress *
            (3.0f - 2.0f * rotationProgress);

        if (!state.isSurfaceTraversalAutoAligning) {
            glm::vec3 autoFollowStartForward;
            glm::vec3 autoFollowTargetForward;
            if (!TryGetTangentDirection(
                    state.autoFollowStartForwardVec,
                    up,
                    autoFollowStartForward) ||
                !TryGetTangentDirection(
                    state.autoFollowTargetForwardVec,
                    up,
                    autoFollowTargetForward)) {
                CancelAutomaticFollow(state);
                state.cameraForwardVec = normalForward;
                return;
            }

            state.cameraForwardVec = RotateBetween(
                autoFollowStartForward,
                autoFollowTargetForward,
                up,
                easedRotationProgress);
            player->SetCameraForwardDirection(state.cameraForwardVec, up);

            if (rotationProgress >= 1.0f) {
                state.cameraForwardVec = autoFollowTargetForward;
                player->SetCameraForwardDirection(state.cameraForwardVec, up);
                state.isAutoFollowingBehindPlayer = false;
                state.autoFollowElapsedSeconds = 0.0f;
                state.autoFollowStartOffsetRadians = 0.0f;
            }
            return;
        }

        glm::vec3 currentBehindPlayerForward;
        if (!TryGetTangentDirection(
                -player->GetFacingForwardVec(),
                up,
                currentBehindPlayerForward)) {
            state.isAutoFollowingBehindPlayer = false;
            state.isSurfaceTraversalAutoAligning = false;
            state.autoFollowElapsedSeconds = 0.0f;
            state.autoFollowStartOffsetRadians = 0.0f;
            state.cameraForwardVec = normalForward;
            return;
        }

        const float remainingOffsetRadians =
            state.autoFollowStartOffsetRadians *
            (1.0f - easedRotationProgress);
        state.cameraForwardVec = RotateAroundUp(
            currentBehindPlayerForward,
            up,
            remainingOffsetRadians);
        player->SetCameraForwardDirection(state.cameraForwardVec, up);

        if (rotationProgress >= 1.0f) {
            state.cameraForwardVec = currentBehindPlayerForward;
            player->SetCameraForwardDirection(state.cameraForwardVec, up);
            state.isAutoFollowingBehindPlayer = false;
            state.isSurfaceTraversalAutoAligning = false;
            state.autoFollowElapsedSeconds = 0.0f;
            state.autoFollowStartOffsetRadians = 0.0f;
        }
        return;
    }

    state.cameraForwardVec = normalForward;
}

void PlayerCamera::UpdateBackwardFacingFraming(
    Player* player,
    PlayerCameraState& state,
    const PlayerCameraSettings& settings,
    float deltaTime,
    bool allowsMovementCameraAssist)
{
    glm::vec3 facingForward;
    glm::vec3 directionFromPlayerToCamera;
    const bool hasFacingDirection = TryGetTangentDirection(
        player->GetFacingForwardVec(), state.upVec, facingForward);
    const bool hasCameraDirection = TryGetTangentDirection(
        state.cameraForwardVec, state.upVec, directionFromPlayerToCamera);

    if (!allowsMovementCameraAssist ||
        !hasFacingDirection ||
        !hasCameraDirection) {
        state.isBackwardFacingFramingActive = false;
        state.backwardMovementHoldSeconds = 0.0f;
    } else {
        const float facingTowardCameraDot = glm::clamp(
            glm::dot(facingForward, directionFromPlayerToCamera), -1.0f, 1.0f);
        const float framingStartDot = std::cos(
            glm::radians(settings.backwardFacingFramingStartAngleDegrees));
        const float framingEndDot = std::cos(
            glm::radians(settings.backwardFacingFramingEndAngleDegrees));
        const float backwardInput = std::max(
            0.0f,
            player->GetMoveForwardInput());
        const bool isMovingBackward =
            backwardInput >= settings.backwardFacingFramingMinimumBackwardInput;
        const bool isFacingTowardCamera =
            facingTowardCameraDot >= framingStartDot;

        if (state.isBackwardFacingFramingActive) {
            if (facingTowardCameraDot < framingEndDot) {
                state.isBackwardFacingFramingActive = false;
                state.backwardMovementHoldSeconds = 0.0f;
            }
        } else {
            if (!isMovingBackward || !isFacingTowardCamera) {
                state.backwardMovementHoldSeconds = 0.0f;
            } else {
                state.backwardMovementHoldSeconds += std::max(0.0f, deltaTime);
                if (state.backwardMovementHoldSeconds >=
                    settings.backwardFacingFramingActivationDelaySeconds) {
                    state.isBackwardFacingFramingActive = true;
                    state.backwardMovementHoldSeconds = 0.0f;
                }
            }
        }
    }

    const float targetLookAheadDistance =
        state.isBackwardFacingFramingActive
            ? settings.backwardFacingLookAheadDistance
            : 0.0f;
    const float safeDeltaTime = std::max(0.0f, deltaTime);
    const float lookAheadBlend = 1.0f - std::exp(
        -settings.backwardFacingFramingSmoothingSpeed * safeDeltaTime);
    state.backwardFacingLookAheadDistance = glm::mix(
        state.backwardFacingLookAheadDistance,
        targetLookAheadDistance,
        lookAheadBlend);
}

void PlayerCamera::UpdateSurfaceTraversalAutoAlign(
    Player* player,
    PlayerCameraState& state,
    const PlayerCameraSettings& settings,
    bool allowsMovementCameraAssist)
{
    const float movementInputMagnitude = glm::length(
        glm::vec2(
            player->GetMoveLeftInput(),
            player->GetMoveForwardInput()));
    const bool isMovementInputActive =
        movementInputMagnitude >=
        settings.surfaceTraversalAutoAlignMinimumMovementInput;
    const bool isAutoAlignBlocked =
        !allowsMovementCameraAssist ||
        state.autoFollowDelayRemainingSeconds > 0.0f;
    if (isAutoAlignBlocked || !isMovementInputActive) {
        ResetSurfaceTraversalTracking(state);
        return;
    }

    const glm::vec3 playerUp = player->GetUpVec();
    const float playerUpLengthSquared = glm::dot(playerUp, playerUp);
    if (playerUpLengthSquared <= directionEpsilonSquared) {
        ResetSurfaceTraversalTracking(state);
        return;
    }

    const glm::vec3 currentSurfaceNormal =
        playerUp / std::sqrt(playerUpLengthSquared);
    if (!state.hasSurfaceTraversalStartUp) {
        state.surfaceTraversalStartUpVec = currentSurfaceNormal;
        state.hasSurfaceTraversalStartUp = true;
        return;
    }

    if (state.hasAutoAlignedForCurrentSurfaceTraversal) {
        return;
    }

    const float surfaceNormalDot = glm::clamp(
        glm::dot(
            state.surfaceTraversalStartUpVec,
            currentSurfaceNormal),
        -1.0f,
        1.0f);
    const float autoAlignMaximumDot = std::cos(
        glm::radians(settings.surfaceTraversalAutoAlignAngleDegrees));
    if (surfaceNormalDot > autoAlignMaximumDot ||
        !StartAutomaticFollowBehindPlayer(*player, state)) {
        return;
    }

    state.isSurfaceTraversalAutoAligning = true;
    state.hasAutoAlignedForCurrentSurfaceTraversal = true;
    state.isBackwardFacingFramingActive = false;
    state.backwardMovementHoldSeconds = 0.0f;
    player->LockMovementDirectionForCameraAutoAlign();
}
