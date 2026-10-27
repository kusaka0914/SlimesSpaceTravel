#include "system/camera/PlayerCamera.h"

#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Player.h"
#include "system/camera/CameraCollisionResolver.h"
#include "system/camera/CameraLateralOrbit.h"
#include "system/camera/CinematicCameraTypes.h"
#include "system/camera/PlayerCameraSettings.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <optional>

namespace {
constexpr float directionEpsilonSquared = 0.000001f;

bool UsesSphereFollow(const Player& player)
{
    const auto* planet = player.GetCurrentPlanet();
    return planet && planet->GetPlanetShape() == Planet::PlanetShape::Sphere;
}

glm::vec3 OrbitPosition(const glm::vec3& center, const glm::vec3& back, const glm::vec3& up, float distance,
                        float pitch)
{
    return center + (back * std::cos(pitch) - up * std::sin(pitch)) * distance;
}

glm::vec3 NormalizeOr(const glm::vec3& value, const glm::vec3& fallback)
{
    return glm::dot(value, value) > directionEpsilonSquared ? glm::normalize(value) : fallback;
}

bool TryGetTangentDirection(const glm::vec3& direction, const glm::vec3& up, glm::vec3& tangent)
{
    const glm::vec3 projected = direction - up * glm::dot(direction, up);
    if (glm::dot(projected, projected) <= directionEpsilonSquared) {
        return false;
    }
    tangent = glm::normalize(projected);
    return true;
}

glm::vec3 RotateBetween(const glm::vec3& from, const glm::vec3& to, const glm::vec3& up, float amount)
{
    const float angle = std::atan2(glm::dot(glm::cross(from, to), up), glm::clamp(glm::dot(from, to), -1.0f, 1.0f));
    return glm::angleAxis(angle * amount, up) * from;
}

float SmoothingAmount(float speed, float deltaTime)
{
    return 1.0f - std::exp(-std::max(0.0f, speed) * std::max(0.0f, deltaTime));
}

glm::mat4 MakeView(const glm::vec3& position, const glm::vec3& target, glm::vec3 up)
{
    const auto direction = NormalizeOr(target - position, {0, 0, -1});
    if (glm::dot(glm::cross(direction, up), glm::cross(direction, up)) <= directionEpsilonSquared) {
        up = std::abs(direction.y) < 0.9f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
    }
    return glm::lookAt(position, position + direction, up);
}
} // namespace

PlayerCamera::PlayerCamera(CameraCollisionResolver& collisionResolver)
    : mCollisionResolver(collisionResolver)
{
}

void PlayerCamera::Update(const std::vector<Player*>& players, const std::vector<float>& yawDeltas,
                          const PlayerCameraSettings& settings, float deltaTime, bool allowsMovementCameraAssist,
                          const std::vector<bool>& dedicatedCameras)
{
    ResizeState(players.size());
    for (std::size_t index = 0; index < players.size(); ++index) {
        auto* player = players[index];
        if (!player) {
            continue;
        }
        const float yaw = index < yawDeltas.size() ? yawDeltas[index] : 0.0f;
        if (std::abs(yaw) > 0.000001f) {
            NotifyManualInput(player, static_cast<int>(index));
            mStates[index].manualYaw += yaw;
        }
        // 視線から移動基準を返すため、PlayerMovement側で同じyawを二重に適用しない。
        player->SetCameraYaw(0.0f);
        const bool dedicated = index < dedicatedCameras.size() && dedicatedCameras[index];
        UpdateState(player, static_cast<int>(index), settings, deltaTime, allowsMovementCameraAssist, dedicated);
    }
}

void PlayerCamera::NotifyManualInput(Player* player, int playerIndex)
{
    if (!player || playerIndex < 0) {
        return;
    }
    ResizeState(static_cast<std::size_t>(playerIndex + 1));
    mStates[playerIndex].isAligningBehindPlayer = false;
    mStates[playerIndex].manualInputPending = true;
    mStates[playerIndex].hasAttackTargetForward = false;
    if (mStates[playerIndex].ownsEllipseMovementLock) {
        player->UnlockMovementDirectionForCameraAutoAlign();
    }
    mStates[playerIndex].ownsEllipseMovementLock = false;
    mStates[playerIndex].ellipseTransition.Reset();
}

void PlayerCamera::AdoptZonePose(int playerIndex, const CameraPose& pose, const Player& player)
{
    if (playerIndex < 0 || static_cast<std::size_t>(playerIndex) >= mStates.size()) {
        return;
    }
    auto& state = mStates[playerIndex];
    // Zone側が既に取得した方向保持は解除せず、通常カメラの回り込みだけを破棄する。
    state.ellipseTransition.Reset();
    state.ownsEllipseMovementLock = false;
    state.isAligningBehindPlayer = false;
    state.hasAttackTargetForward = false;
    state.isTransitioningToPlayer = false;
    state.lateralFollowBlend = 1.0f;
    state.isSettlingJump = false;
    state.orbitRequested = false;
    state.manualYaw = 0.0f;
    state.upVec = NormalizeOr(pose.up, {0, 1, 0});
    state.previousPlayerUp = NormalizeOr(player.GetUpVec(), state.upVec);
    state.previousPlayerPosition = player.GetPos();
    state.hasPlayerPosition = true;
    state.cameraPos = pose.position;
    state.followPosition = pose.position;
    state.desiredFollowPosition = pose.position;
    state.hasFollowPosition = true;
    state.needsNormalFraming = true;
    // 表示中の注視点を保持する。通常構図への復帰先はGetViewで別に組み立てる。
    state.targetPos = pose.target - state.upVec * state.targetHeight;
    state.lookTarget = pose.target;
    const auto zoneBack = UsesSphereFollow(player)
                              ? glm::quat(state.upVec, state.previousPlayerUp) * (pose.position - pose.target)
                              : pose.position - pose.target;
    state.hasCameraForward = TryGetTangentDirection(
        zoneBack, UsesSphereFollow(player) ? state.previousPlayerUp : state.upVec, state.cameraForwardVec);
    state.isRecoveringFromCollision = false;
    state.isBackwardFacingFramingActive = false;
    state.backwardMovementHoldSeconds = 0.0f;
    state.backwardFacingLookAheadDistance = 0.0f;
}

void PlayerCamera::RequestNormalFraming(int playerIndex)
{
    if (playerIndex < 0) {
        return;
    }
    ResizeState(static_cast<std::size_t>(playerIndex + 1));
    mStates[playerIndex].needsNormalFraming = true;
}

void PlayerCamera::Reset()
{
    mStates.clear();
}

void PlayerCamera::CancelEllipseTransitions(const std::vector<Player*>& players)
{
    for (std::size_t index = 0; index < mStates.size(); ++index) {
        auto& state = mStates[index];
        if (state.ownsEllipseMovementLock && index < players.size() && players[index]) {
            players[index]->UnlockMovementDirectionForCameraAutoAlign();
        }
        state.ownsEllipseMovementLock = false;
        state.ellipseTransition.Reset();
    }
}

void PlayerCamera::SnapToPlayer(Player* player, int playerIndex)
{
    if (!player || playerIndex < 0) {
        return;
    }
    ResizeState(static_cast<std::size_t>(playerIndex + 1));
    auto& state = mStates[playerIndex];
    state = {};
    state.followsLateralMovement = UsesSphereFollow(*player);
    state.targetPos = player->GetPos();
    state.previousPlayerPosition = player->GetPos();
    state.upVec = NormalizeOr(player->GetUpVec(), {0, 1, 0});
    state.previousPlayerUp = state.upVec;
    state.hasPlayerPosition = true;
    state.hasCameraForward = TryGetTangentDirection(player->GetForwardVec(), state.upVec, state.cameraForwardVec);
    player->SetCameraYaw(0.0f);
    player->UnlockMovementDirectionForCameraAutoAlign();
}

void PlayerCamera::TransitionToPlayer(int fromPlayerIndex, Player* player, int toPlayerIndex)
{
    if (!player || toPlayerIndex < 0) {
        return;
    }
    if (fromPlayerIndex < 0 || static_cast<std::size_t>(fromPlayerIndex) >= mStates.size() ||
        fromPlayerIndex == toPlayerIndex) {
        SnapToPlayer(player, toPlayerIndex);
        return;
    }
    const auto previous = mStates[fromPlayerIndex];
    ResizeState(static_cast<std::size_t>(toPlayerIndex + 1));
    auto& state = mStates[toPlayerIndex];
    state = previous;
    state.ellipseTransition.Reset();
    state.ownsEllipseMovementLock = false;
    state.hasAttackTargetForward = false;
    state.manualInputPending = false;
    const auto up = NormalizeOr(player->GetUpVec(), previous.previousPlayerUp);
    const auto transport = glm::quat(previous.previousPlayerUp, up);
    state.desiredFollowPosition =
        player->GetPos() + transport * (previous.followPosition - previous.previousPlayerPosition);
    state.previousPlayerPosition = player->GetPos();
    state.previousPlayerUp = up;
    state.followsLateralMovement = UsesSphereFollow(*player);
    state.lateralFollowStart = previous.cameraPos;
    state.lateralFollowBlend = state.hasFollowPosition && state.followsLateralMovement ? 0.0f : 1.0f;
    state.isTransitioningToPlayer = state.hasFollowPosition;
    state.isAligningBehindPlayer = false;
    state.manualYaw = 0.0f;
    player->SetCameraForwardDirection(state.cameraForwardVec, state.upVec);
    player->SetCameraYaw(0.0f);
    player->UnlockMovementDirectionForCameraAutoAlign();
}

void PlayerCamera::SnapBehindPlayer(Player* player, int playerIndex)
{
    SnapToPlayer(player, playerIndex);
    if (!player || playerIndex < 0) {
        return;
    }
    auto& state = mStates[playerIndex];
    state.manualInputPending = true;
    state.hasCameraForward =
        TryGetTangentDirection(-player->GetFacingForwardVec(), state.upVec, state.cameraForwardVec);
    if (state.hasCameraForward) {
        player->SetCameraForwardDirection(state.cameraForwardVec, state.upVec);
    }
}

void PlayerCamera::AlignBehindPlayer(Player* player, int playerIndex)
{
    if (!player || playerIndex < 0) {
        return;
    }
    ResizeState(static_cast<std::size_t>(playerIndex + 1));
    auto& state = mStates[playerIndex];
    state.manualInputPending = true;
    state.hasAttackTargetForward = false;
    state.isAligningBehindPlayer = TryGetTangentDirection(
        -player->GetFacingForwardVec(), NormalizeOr(player->GetUpVec(), {0, 1, 0}), state.alignTargetForwardVec);
    player->UnlockMovementDirectionForCameraAutoAlign();
}

void PlayerCamera::BlendBehindTarget(Player* player, int playerIndex, const glm::vec3& targetPosition, float blend)
{
    if (!player || playerIndex < 0) {
        return;
    }
    ResizeState(static_cast<std::size_t>(playerIndex + 1));
    auto& state = mStates[playerIndex];
    const auto directionUp = UsesSphereFollow(*player) ? NormalizeOr(player->GetUpVec(), state.upVec) : state.upVec;
    glm::vec3 from, to;
    if (TryGetTangentDirection(state.cameraForwardVec, directionUp, from) &&
        TryGetTangentDirection(player->GetPos() - targetPosition, directionUp, to)) {
        state.cameraForwardVec = RotateBetween(from, to, directionUp, std::clamp(blend, 0.0f, 1.0f));
        state.hasCameraForward = true;
        state.orbitRequested = true;
    }
}

void PlayerCamera::UpdateFollowPosition(Player* player, PlayerCameraState& state, const PlayerCameraSettings& settings,
                                        float deltaTime, bool allowsLateralOrbit)
{
    const auto inputMovement = player->ConsumeCameraInputMovementDelta();
    const auto up = NormalizeOr(player->GetUpVec(), state.previousPlayerUp);
    const auto delta = player->GetPos() - state.previousPlayerPosition;
    if (state.ellipseTransition.IsActive()) {
        player->ConsumeCameraLateralCorrection();
        state.previousPlayerPosition = player->GetPos();
        state.previousPlayerUp = up;
        return;
    }
    if (state.followsLateralMovement) {
        // 球面では方向だけを地面法線へ運び、位置はGetViewでプレイヤー全方向に追従させる。
        state.cameraForwardVec = glm::quat(state.previousPlayerUp, up) * state.cameraForwardVec;
        player->ConsumeCameraLateralCorrection();
        constexpr float profileTransitionSeconds = 0.7f;
        state.lateralFollowBlend =
            std::min(1.0f, state.lateralFollowBlend + std::max(0.0f, deltaTime) / profileTransitionSeconds);
        state.previousPlayerPosition = player->GetPos();
        state.previousPlayerUp = up;
        return;
    }
    glm::vec3 back;
    if (!TryGetTangentDirection(state.cameraForwardVec, state.previousPlayerUp, back)) {
        back = NormalizeOr(glm::cross(state.previousPlayerUp, glm::vec3(1, 0, 0)), {0, 0, 1});
    }
    // 横入力でも壁滑りで前後に進むため、入力ではなく実移動を追う。円弧用の接線補正だけを除く。
    const auto lateralCorrection = player->ConsumeCameraLateralCorrection();
    const auto followDelta = glm::dot(delta, delta) > 0.00000001f ? delta - lateralCorrection : delta;
    const float forwardDistance = glm::dot(followDelta, back);
    const bool hasForwardMovement = std::abs(forwardDistance) > 0.00001f;
    const bool isAirborne = !player->GetOnGround();
    state.isSettlingJump = state.isSettlingJump || isAirborne;
    if (state.hasFollowPosition && (hasForwardMovement || isAirborne)) {
        glm::vec3 translation(0.0f);
        if (hasForwardMovement) {
            translation += back * forwardDistance;
            // 法線変化のうち前後方向だけを使い、左右移動で基準位置が惑星を周回しないようにする。
            const float curveAngle = std::atan2(glm::dot(up, back), glm::dot(up, state.previousPlayerUp));
            const auto axis = NormalizeOr(glm::cross(state.previousPlayerUp, back), {1, 0, 0});
            const auto rotation = glm::angleAxis(curveAngle, axis);
            state.desiredFollowPosition =
                state.previousPlayerPosition + rotation * (state.desiredFollowPosition - state.previousPlayerPosition);
        }
        // 空中ではジャンプ・落下の高さを追う。接地中の法線追従は実際に前後へ進んだ場合に限定する。
        if (isAirborne || hasForwardMovement) {
            translation += state.previousPlayerUp * glm::dot(delta, state.previousPlayerUp);
        }
        state.desiredFollowPosition += translation;
    } else if (!state.isTransitioningToPlayer) {
        // 地上の左右追跡では位置の残り補間を止める。着地後だけは高さの遅れを解消し、ジャンプごとの漂流を防ぐ。
        const auto jumpCorrection =
            state.isSettlingJump ? up * glm::dot(state.desiredFollowPosition - state.followPosition, up) : glm::vec3(0);
        state.desiredFollowPosition = state.followPosition + jumpCorrection;
    }
    if (state.hasFollowPosition) {
        state.followPosition = glm::mix(state.followPosition, state.desiredFollowPosition,
                                        SmoothingAmount(settings.positionSmoothingSpeed, deltaTime));
        if (glm::distance(state.followPosition, state.desiredFollowPosition) < 0.001f) {
            state.followPosition = state.desiredFollowPosition;
            state.isTransitioningToPlayer = false;
            state.isSettlingJump = false;
        }
    }
    if (allowsLateralOrbit && state.hasFollowPosition && player->GetOnGround() &&
        !dynamic_cast<Platform*>(player->GetGroundActor()) && !player->IsCameraZoneDirectionHeld() &&
        std::abs(player->GetMoveLeftInput()) > 0.01f) {
        // 地上の実入力変位だけを使い、足場・空中・方向保持中には周回を重ねない。
        if (const auto rotation = CameraLateralOrbit::CalculateRotation(state.followPosition, player->GetPos(), up,
                                                                        inputMovement, settings.lateralOrbitStrength)) {
            state.followPosition = player->GetPos() + *rotation * (state.followPosition - player->GetPos());
            state.desiredFollowPosition =
                player->GetPos() + *rotation * (state.desiredFollowPosition - player->GetPos());
        }
    }
    state.previousPlayerPosition = player->GetPos();
    state.previousPlayerUp = up;
}

void PlayerCamera::UpdateState(Player* player, int playerIndex, const PlayerCameraSettings& settings, float deltaTime,
                               bool allowsMovementCameraAssist, bool dedicatedCamera)
{
    auto& state = mStates[playerIndex];
    if (state.ownsEllipseMovementLock && !player->IsCameraZoneDirectionHeld()) {
        state.ownsEllipseMovementLock = false;
    }
    const bool manualInput = state.manualInputPending;
    state.manualInputPending = false;
    state.attackCameraManualDelay = manualInput
                                        ? settings.attackCameraManualOverrideSeconds
                                        : std::max(0.0f, state.attackCameraManualDelay - std::max(0.0f, deltaTime));
    const bool sphereFollow = UsesSphereFollow(*player);
    if (state.followsLateralMovement != sphereFollow) {
        state.followsLateralMovement = sphereFollow;
        state.desiredFollowPosition = state.followPosition;
        if (sphereFollow && state.hasFollowPosition) {
            // 固定位置から追従方式へ戻る際、残った横方向の距離を一度に詰めない。
            state.lateralFollowStart = state.followPosition;
            state.lateralFollowBlend = 0.0f;
            state.isTransitioningToPlayer = true;
        }
    }
    if (!state.hasPlayerPosition) {
        state.upVec = NormalizeOr(player->GetUpVec(), {0, 1, 0});
        state.targetPos = player->GetPos();
        state.previousPlayerPosition = player->GetPos();
        state.previousPlayerUp = state.upVec;
        state.hasPlayerPosition = true;
    }
    // テレポート・復帰では、以前の位置との差分を通常移動として加算しない。
    if (glm::distance(state.previousPlayerPosition, player->GetPos()) > 12.0f) {
        state.hasFollowPosition = false;
        state.targetPos = player->GetPos();
        state.previousPlayerPosition = player->GetPos();
        state.isRecoveringFromCollision = false;
        state.ellipseTransition.Reset();
        if (state.ownsEllipseMovementLock) {
            player->UnlockMovementDirectionForCameraAutoAlign();
            state.ownsEllipseMovementLock = false;
        }
    }
    const float movementSpeed = deltaTime > 0.000001f
                                    ? std::clamp(glm::dot(player->GetPos() - state.previousPlayerPosition,
                                                          player->GetInputMovementDirection()) /
                                                     deltaTime,
                                                 0.0f, std::max(0.0f, player->GetMoveSpeed()))
                                    : 0.0f;
    const bool allowsLateralOrbit =
        allowsMovementCameraAssist && !dedicatedCamera && !manualInput && state.attackCameraManualDelay <= 0.0f &&
        !state.isAligningBehindPlayer && !state.isTransitioningToPlayer && !state.needsNormalFraming &&
        !state.ellipseTransition.IsActive() && !player->IsAttacking() && !player->IsSpecialCharging();
    UpdateFollowPosition(player, state, settings, deltaTime, allowsLateralOrbit);
    const auto up = NormalizeOr(player->GetUpVec(), {0, 1, 0});
    state.upVec = glm::slerp(glm::quat(1, 0, 0, 0), glm::quat(state.upVec, up),
                             SmoothingAmount(settings.upSmoothingSpeed, deltaTime)) *
                  state.upVec;
    state.upVec = NormalizeOr(state.upVec, up);
    state.targetPos =
        glm::mix(state.targetPos, player->GetPos(), SmoothingAmount(settings.targetSmoothingSpeed, deltaTime));
    state.collisionRecoveryBlend = SmoothingAmount(6.0f, deltaTime);
    if (player->ConsumeCameraAutoAlignCancellationRequest()) {
        player->UnlockMovementDirectionForCameraAutoAlign();
    }
    const bool explicitAlignment = state.isAligningBehindPlayer;
    UpdateCameraForward(player, state, deltaTime);
    UpdateAttackCameraAssist(player, state, settings, deltaTime,
                             allowsMovementCameraAssist && !dedicatedCamera && !manualInput && !explicitAlignment &&
                                 !state.isTransitioningToPlayer && !state.ellipseTransition.IsActive());
    UpdateBackwardFacingFraming(player, state, settings, deltaTime,
                                allowsMovementCameraAssist && player->GetOnGround());
    UpdateEllipseTransition(*player, state, settings, deltaTime, movementSpeed,
                            allowsMovementCameraAssist && !manualInput && !explicitAlignment, dedicatedCamera);
}

void PlayerCamera::UpdateEllipseTransition(Player& player, PlayerCameraState& state,
                                           const PlayerCameraSettings& settings, float deltaTime, float movementSpeed,
                                           bool allowsAssist, bool dedicatedCamera)
{
    const auto* planet = player.GetCurrentPlanet();
    if (!allowsAssist || dedicatedCamera || !planet || planet->GetPlanetShape() != Planet::PlanetShape::Ellipse ||
        !state.hasFollowPosition || state.needsNormalFraming || state.isTransitioningToPlayer ||
        state.isAligningBehindPlayer || state.orbitRequested || state.attackCameraManualDelay > 0.0f) {
        state.ellipseTransition.Reset();
        if (state.ownsEllipseMovementLock && !dedicatedCamera) {
            player.UnlockMovementDirectionForCameraAutoAlign();
        }
        state.ownsEllipseMovementLock = false;
        return;
    }
    EllipseCameraTransitionInput input;
    input.planet = planet;
    input.playerPosition = player.GetPos();
    input.framingCenter = state.targetPos;
    input.movementDirection = player.GetInputMovementDirection();
    input.up = state.upVec;
    input.playerUp = player.GetUpVec();
    input.cameraPosition = state.cameraPos;
    input.cameraBack = state.cameraForwardVec;
    input.movementSpeed = movementSpeed;
    input.distance = state.distance > 0.0f ? state.distance : settings.distance;
    input.pitch = state.distance > 0.0f ? state.pitch : glm::radians(settings.pitchDegrees);
    input.clearanceRadius = state.clearanceRadius;
    input.allowsStart = player.GetOnGround() && !player.IsAttacking();
    const bool wasActive = state.ellipseTransition.IsActive();
    if (const auto pose = state.ellipseTransition.Update(input, mCollisionResolver, {&player}, deltaTime)) {
        if (!wasActive && state.ellipseTransition.IsActive()) {
            player.LockMovementDirectionForCameraAutoAlign(true);
            state.ownsEllipseMovementLock = player.IsCameraZoneDirectionHeld();
        }
        // 位置・注視中心・上方向を同じ姿勢から引き継ぎ、完了後の補間で構図がずれるのを防ぐ。
        state.followPosition = pose->position;
        state.desiredFollowPosition = pose->position;
        state.targetPos = pose->center;
        state.upVec = pose->up;
        state.cameraForwardVec = pose->back;
        state.hasCameraForward = true;
        state.isSettlingJump = false;
    }
}

void PlayerCamera::UpdateCameraForward(Player* player, PlayerCameraState& state, float deltaTime)
{
    if (!state.isAligningBehindPlayer) {
        return;
    }
    const auto directionUp = state.followsLateralMovement ? NormalizeOr(player->GetUpVec(), state.upVec) : state.upVec;
    glm::vec3 from, to;
    if (!TryGetTangentDirection(state.cameraForwardVec, directionUp, from) ||
        !TryGetTangentDirection(state.alignTargetForwardVec, directionUp, to)) {
        state.isAligningBehindPlayer = false;
        return;
    }
    state.cameraForwardVec = RotateBetween(from, to, directionUp, SmoothingAmount(8.0f, deltaTime));
    state.hasCameraForward = true;
    state.orbitRequested = true;
    if (glm::dot(state.cameraForwardVec, to) >= 0.9995f) {
        state.cameraForwardVec = to;
        state.isAligningBehindPlayer = false;
    }
    player->SetCameraForwardDirection(state.cameraForwardVec, directionUp);
}

void PlayerCamera::UpdateAttackCameraAssist(Player* player, PlayerCameraState& state,
                                            const PlayerCameraSettings& settings, float deltaTime, bool allowsAssist)
{
    if (!allowsAssist || !player->IsAttacking() || state.attackCameraManualDelay > 0.0f ||
        settings.attackTargetSmoothingSpeed <= 0.0f) {
        state.hasAttackTargetForward = false;
        return;
    }
    // らくらくスタイルの攻撃で選ばれた対象を使い、カメラ側では対象を探し直さない。
    const auto directionUp = state.followsLateralMovement ? NormalizeOr(player->GetUpVec(), state.upVec) : state.upVec;
    const auto* target = player->GetAttackDirectionTarget();
    if (target) {
        if (!target->GetIsActive() || !target->IsAlive() || target->GetIsDead() ||
            target->GetCurrentPlanet() != player->GetCurrentPlanet()) {
            state.hasAttackTargetForward = false;
            return;
        }
        state.hasAttackTargetForward =
            TryGetTangentDirection(player->GetPos() - target->GetPos(), directionUp, state.attackTargetForwardVec);
    }
    // 攻撃判定後に対象参照が解除されても、攻撃終了までは直前の補助方向を維持する。
    glm::vec3 from, to;
    if (!state.hasAttackTargetForward ||
        !TryGetTangentDirection(state.hasCameraForward ? state.cameraForwardVec : player->GetForwardVec(), directionUp,
                                from) ||
        !TryGetTangentDirection(state.attackTargetForwardVec, directionUp, to) || glm::dot(from, to) > 0.99999f) {
        return;
    }
    state.cameraForwardVec =
        RotateBetween(from, to, directionUp, SmoothingAmount(settings.attackTargetSmoothingSpeed, deltaTime));
    state.hasCameraForward = true;
    state.orbitRequested = true;
}

// GetViewは通常追従の基準位置を衝突補正で書き換えず、描画位置と入力基準だけを更新する。
glm::mat4 PlayerCamera::GetView(Player* player, int playerIndex, float cameraDistance, float cameraPitch,
                                float targetHeight, bool isFixed, float clearanceRadius)
{
    if (!player || playerIndex < 0) {
        return glm::mat4(1.0f);
    }
    ResizeState(static_cast<std::size_t>(playerIndex + 1));
    auto& state = mStates[playerIndex];
    const bool sphereFollow = UsesSphereFollow(*player);
    state.clearanceRadius = clearanceRadius;
    if (!state.hasFollowPosition) {
        state.followsLateralMovement = sphereFollow;
    }
    const bool restoringNormalFraming = state.needsNormalFraming;
    const auto up =
        NormalizeOr(!state.hasPlayerPosition || restoringNormalFraming ? player->GetUpVec() : state.upVec, {0, 1, 0});
    if (restoringNormalFraming) {
        // ゾーン退出や演出による直接移動では、保持した位置を使わず通常設定から構図を組み直す。
        state.targetPos = player->GetPos();
        state.upVec = up;
        state.needsNormalFraming = false;
    }
    const auto movementUp = NormalizeOr(player->GetUpVec(), up);
    const auto& ellipsePose = state.ellipseTransition.GetPose();
    const auto followDirection = ellipsePose              ? ellipsePose->back
                                 : state.hasCameraForward ? state.cameraForwardVec
                                                          : player->GetForwardVec();
    const auto viewDirection = sphereFollow ? glm::quat(movementUp, up) * followDirection : followDirection;
    glm::vec3 back;
    if (!TryGetTangentDirection(viewDirection, up, back)) {
        back = NormalizeOr(glm::cross(up, glm::vec3(1, 0, 0)), {0, 0, 1});
    }
    const bool framingChanged =
        std::abs(state.distance - cameraDistance) > 0.0001f || std::abs(state.pitch - cameraPitch) > 0.0001f;
    if (framingChanged && state.ellipseTransition.IsActive()) {
        // 最新の画角設定を直ちに反映した後、古い回り込み経路で位置を上書きしない。
        state.ellipseTransition.Reset();
        if (state.ownsEllipseMovementLock) {
            player->UnlockMovementDirectionForCameraAutoAlign();
            state.ownsEllipseMovementLock = false;
        }
    }
    if (std::abs(state.manualYaw) > 0.000001f) {
        back = glm::angleAxis(-state.manualYaw, up) * back;
        state.orbitRequested = true;
        state.manualYaw = 0.0f;
    }
    if (!state.hasFollowPosition || restoringNormalFraming || framingChanged || state.orbitRequested) {
        state.lateralFollowBlend = 1.0f;
        if (!state.hasPlayerPosition) {
            state.upVec = up;
            state.targetPos = player->GetPos();
            state.previousPlayerPosition = player->GetPos();
            state.previousPlayerUp = up;
            state.hasPlayerPosition = true;
        }
        state.followPosition = OrbitPosition(state.targetPos, back, up, cameraDistance, cameraPitch);
        state.desiredFollowPosition = state.followPosition;
        state.hasFollowPosition = true;
        state.isRecoveringFromCollision = false;
        state.orbitRequested = false;
    }
    if (sphereFollow) {
        const auto destination = OrbitPosition(state.targetPos, back, up, cameraDistance, cameraPitch);
        const float blend =
            state.lateralFollowBlend * state.lateralFollowBlend * (3.0f - 2.0f * state.lateralFollowBlend);
        state.followPosition = glm::mix(state.lateralFollowStart, destination, blend);
        state.desiredFollowPosition = state.followPosition;
        if (state.lateralFollowBlend >= 1.0f) {
            state.isTransitioningToPlayer = false;
        }
    }
    state.distance = cameraDistance;
    state.pitch = cameraPitch;
    state.targetHeight = targetHeight;
    const float maximumLookAhead = cameraDistance * std::abs(std::cos(cameraPitch)) * 0.8f;
    const auto lookTarget =
        state.targetPos + up * targetHeight + back * std::min(state.backwardFacingLookAheadDistance, maximumLookAhead);
    glm::vec3 desired = state.followPosition;
    if (isFixed) {
        const auto facing = NormalizeOr(player->GetFacingForwardVec(), -back);
        desired = state.targetPos + (facing * std::cos(-0.2f) - up * std::sin(-0.2f)) * cameraDistance;
    }
    const auto anchor = player->GetPos() + NormalizeOr(player->GetUpVec(), up) * 0.5f;
    const auto resolved = mCollisionResolver.ResolveWithClearance(anchor, desired, up, clearanceRadius, {player});
    if (glm::distance(resolved, desired) > 0.001f) {
        state.cameraPos = resolved;
        state.isRecoveringFromCollision = true;
    } else if (state.isRecoveringFromCollision) {
        const auto candidate = glm::mix(state.cameraPos, desired, state.collisionRecoveryBlend);
        state.cameraPos = mCollisionResolver.ResolveWithClearance(anchor, candidate, up, clearanceRadius, {player});
        if (glm::distance(state.cameraPos, desired) < 0.001f) {
            state.cameraPos = desired;
            state.isRecoveringFromCollision = false;
        }
    } else {
        state.cameraPos = desired;
    }
    // Sphereの追従方向へ描画用の法線補間・衝突補正を戻すと、横移動だけでも回転が蓄積する。
    // その他の方式では、注視点の遅れが移動半径を広げないよう実位置同士から求める。
    const auto movementBack = sphereFollow ? glm::quat(up, movementUp) * back : state.cameraPos - player->GetPos();
    if (!isFixed && TryGetTangentDirection(movementBack, movementUp, state.cameraForwardVec)) {
        state.hasCameraForward = true;
        player->SetCameraForwardDirection(state.cameraForwardVec, movementUp);
        // Sphereでは従来のカメラ方向基準を使い、横移動の円状補正は重ねない。
        player->SetNormalCameraPosition(sphereFollow ? std::nullopt : std::optional<glm::vec3>(state.cameraPos));
    } else {
        player->SetNormalCameraPosition(std::nullopt);
    }
    state.lookTarget = isFixed ? state.targetPos + up : lookTarget;
    return MakeView(state.cameraPos, state.lookTarget, up);
}

glm::vec3 PlayerCamera::GetCameraPos(int playerIndex) const
{
    return playerIndex >= 0 && static_cast<std::size_t>(playerIndex) < mStates.size() ? mStates[playerIndex].cameraPos
                                                                                      : glm::vec3(0);
}

glm::vec3 PlayerCamera::GetLookTarget(int playerIndex) const
{
    return playerIndex >= 0 && static_cast<std::size_t>(playerIndex) < mStates.size() ? mStates[playerIndex].lookTarget
                                                                                      : glm::vec3(0);
}

void PlayerCamera::ResizeState(std::size_t count)
{
    if (mStates.size() < count) {
        mStates.resize(count);
    }
}

void PlayerCamera::UpdateBackwardFacingFraming(Player* player, PlayerCameraState& state,
                                               const PlayerCameraSettings& settings, float deltaTime,
                                               bool allowsMovementCameraAssist)
{
    glm::vec3 facingForward;
    glm::vec3 directionFromPlayerToCamera;
    const bool hasFacingDirection = TryGetTangentDirection(player->GetFacingForwardVec(), state.upVec, facingForward);
    const bool hasCameraDirection =
        TryGetTangentDirection(state.cameraForwardVec, state.upVec, directionFromPlayerToCamera);

    if (!allowsMovementCameraAssist || !hasFacingDirection || !hasCameraDirection) {
        state.isBackwardFacingFramingActive = false;
        state.backwardMovementHoldSeconds = 0.0f;
    } else {
        const float facingTowardCameraDot =
            glm::clamp(glm::dot(facingForward, directionFromPlayerToCamera), -1.0f, 1.0f);
        const float framingStartDot = std::cos(glm::radians(settings.backwardFacingFramingStartAngleDegrees));
        const float framingEndDot = std::cos(glm::radians(settings.backwardFacingFramingEndAngleDegrees));
        const float backwardInput = std::max(0.0f, player->GetMoveForwardInput());
        const bool isMovingBackward = backwardInput >= settings.backwardFacingFramingMinimumBackwardInput;
        const bool isFacingTowardCamera = facingTowardCameraDot >= framingStartDot;

        if (state.isBackwardFacingFramingActive) {
            if (!isMovingBackward || facingTowardCameraDot < framingEndDot) {
                state.isBackwardFacingFramingActive = false;
                state.backwardMovementHoldSeconds = 0.0f;
            }
        } else {
            if (!isMovingBackward || !isFacingTowardCamera) {
                state.backwardMovementHoldSeconds = 0.0f;
            } else {
                state.backwardMovementHoldSeconds += std::max(0.0f, deltaTime);
                if (state.backwardMovementHoldSeconds >= settings.backwardFacingFramingActivationDelaySeconds) {
                    state.isBackwardFacingFramingActive = true;
                    state.backwardMovementHoldSeconds = 0.0f;
                }
            }
        }
    }

    const float targetLookAheadDistance =
        state.isBackwardFacingFramingActive ? settings.backwardFacingLookAheadDistance : 0.0f;
    const float safeDeltaTime = std::max(0.0f, deltaTime);
    const float lookAheadBlend = 1.0f - std::exp(-settings.backwardFacingFramingSmoothingSpeed * safeDeltaTime);
    state.backwardFacingLookAheadDistance =
        glm::mix(state.backwardFacingLookAheadDistance, targetLookAheadDistance, lookAheadBlend);
}
