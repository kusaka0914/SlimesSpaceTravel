#include "system/camera/zones/CameraZoneController.h"

#include "actor/CameraZone.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "system/camera/CameraCollisionResolver.h"
#include "system/camera/CameraProjection.h"
#include "system/mesh/LoadedModel.h"
#include "world/Stage.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/quaternion.hpp>

namespace {
glm::vec3 SafeDirection(glm::vec3 value, glm::vec3 fallback)
{
    return glm::dot(value, value) > 0.000001f ? glm::normalize(value) : fallback;
}
CameraPose BlendPose(const CameraPose& from, const CameraPose& to, float amount)
{
    CameraPose result;
    result.target = glm::mix(from.target, to.target, amount);
    const auto upRotation = glm::quat(SafeDirection(from.up, {0, 1, 0}), SafeDirection(to.up, {0, 1, 0}));
    result.up = glm::slerp(glm::quat(1, 0, 0, 0), upRotation, amount) * SafeDirection(from.up, {0, 1, 0});
    // 反転時に頭上を横切らないよう、接平面内の回転と高さを別々に補間する。
    const auto fromOffset = from.position - from.target;
    const auto toOffset = to.position - to.target;
    const auto up = SafeDirection(result.up, {0, 1, 0});
    const auto fromTangent = fromOffset - up * glm::dot(fromOffset, up);
    const auto toTangent = toOffset - up * glm::dot(toOffset, up);
    const auto perpendicular =
        SafeDirection(glm::cross(up, std::abs(up.x) < 0.9f ? glm::vec3(1, 0, 0) : glm::vec3(0, 0, 1)), {0, 0, 1});
    const auto fromDirection = SafeDirection(fromTangent, perpendicular);
    const auto toDirection = SafeDirection(toTangent, fromDirection);
    const float alignment = std::clamp(glm::dot(fromDirection, toDirection), -1.0f, 1.0f);
    // ほぼ180度のときは回転側を固定し、対象の微動で補間経路が反転するのを防ぐ。
    const float yaw = alignment < -0.999f ? std::acos(alignment)
                                          : std::atan2(glm::dot(glm::cross(fromDirection, toDirection), up), alignment);
    const auto direction = glm::angleAxis(yaw * amount, up) * fromDirection;
    const float horizontalDistance = glm::mix(glm::length(fromTangent), glm::length(toTangent), amount);
    const float height = glm::mix(glm::dot(fromOffset, up), glm::dot(toOffset, up), amount);
    result.position = result.target + direction * horizontalDistance + up * height;
    result.fieldOfViewDegrees = glm::mix(from.fieldOfViewDegrees, to.fieldOfViewDegrees, amount);
    return result;
}
float GetFramingRadius(const Actor& actor)
{
    const auto* model = actor.GetLoadedModel();
    if (model && model->hasBounds) {
        const glm::vec3 extent =
            glm::max(glm::abs(model->boundsMinimum), glm::abs(model->boundsMaximum)) * glm::abs(actor.GetRenderScale());
        return std::max(actor.GetRadius(), glm::length(extent));
    }
    return std::max(actor.GetRadius(), glm::length(actor.GetScale()) * 0.5f);
}

CameraPose CalculatePose(const CameraZoneViewSettings& settings, const Player& player, Actor* target, float aspect)
{
    const auto up = SafeDirection(player.GetUpVec(), {0, 1, 0});
    const auto rotation = glm::quat(settings.referenceUp, up);
    CameraPose pose;
    pose.up = up;
    pose.fieldOfViewDegrees = settings.fieldOfViewDegrees;
    pose.target = player.GetPos() + rotation * settings.lookOffset;
    if (settings.mode == CameraZoneSettings::Mode::Fixed) {
        pose = settings.fixedPose;
        pose.fieldOfViewDegrees = settings.fieldOfViewDegrees;
        if (settings.fixedTracksPlayer) {
            pose.target = player.GetPos() + rotation * settings.lookOffset;
            pose.up = up;
        }
        return pose;
    }
    float distance = settings.distance;
    if (settings.mode == CameraZoneSettings::Mode::Target) {
        const glm::vec3 targetPosition = target ? target->GetPos() : settings.fallbackTarget;
        pose.target = glm::mix(player.GetPos(), targetPosition, settings.targetWeight) + rotation * settings.lookOffset;
        const float targetRadius = target ? GetFramingRadius(*target) : 0.5f;
        const float boundingRadius = std::max(glm::length(player.GetPos() - pose.target) + GetFramingRadius(player),
                                              glm::length(targetPosition - pose.target) + targetRadius);
        const float halfVertical = glm::radians(settings.fieldOfViewDegrees) * 0.5f;
        const float halfHorizontal = std::atan(std::tan(halfVertical) * std::max(0.01f, aspect));
        // 縦横の狭い方の画角で包含球を収め、上下分割でも対象の見切れを抑える。
        distance =
            std::clamp(boundingRadius * settings.framingMargin / std::sin(std::min(halfVertical, halfHorizontal)),
                       settings.minimumDistance, settings.maximumDistance);
    }
    const float pitch = glm::radians(settings.pitchDegrees);
    const glm::vec3 back = rotation * settings.backDirection;
    pose.position = pose.target + (back * std::cos(pitch) + up * std::sin(pitch)) * distance;
    return pose;
}
} // namespace

CameraPose CameraZoneController::Apply(Player& player, int playerIndex, const CameraPose& normalPose, Stage* stage,
                                       const FindActor& findActor, const CameraCollisionResolver& collision,
                                       float aspectRatio, float deltaTime, std::uint64_t frameNumber, bool allowsZones,
                                       int previewZone)
{
    if (playerIndex < 0) {
        return normalPose;
    }
    if (mStates.size() <= static_cast<std::size_t>(playerIndex)) {
        mStates.resize(playerIndex + 1);
    }
    State& state = mStates[playerIndex];
    if (!allowsZones || !player.IsAlive() || !player.GetIsActive() || !stage) {
        if (state.ownsMovementLock) {
            player.UnlockMovementDirectionForCameraAutoAlign();
        }
        state = {};
        return normalPose;
    }
    if (state.initialized && state.frame == frameNumber) {
        return state.output;
    }
    // 演出の終了・死亡復帰・テレポート後は、以前の空中維持や補間姿勢を持ち越さない。
    if (!state.initialized || frameNumber > state.frame + 1 ||
        glm::distance(state.previousPlayerPosition, player.GetPos()) > 12.0f) {
        if (state.ownsMovementLock) {
            player.UnlockMovementDirectionForCameraAutoAlign();
        }
        const auto handoffPose = state.handoffPose;
        const bool manualInput = state.manualInput;
        const bool resetRequested = state.resetRequested;
        state = {};
        state.manualInput = manualInput;
        state.resetRequested = resetRequested;
        state.initialized = true;
        state.output = handoffPose.value_or(normalPose);
        state.from = state.output;
        if (handoffPose) {
            state.transitionZone = -2;
        }
        state.elapsed = state.duration;
    }
    state.direction.ObserveMovement(player.GetInputMovementDirection());
    if (state.ownsMovementLock && !player.IsCameraZoneDirectionHeld()) {
        state.ownsMovementLock = false;
    }
    state.frame = frameNumber;
    state.previousPlayerPosition = player.GetPos();
    const float elapsed = std::max(0.0f, deltaTime);
    std::vector<CameraZone*> zones;
    for (auto* planet : stage->GetPlanets()) {
        if (!planet) {
            continue;
        }
        for (auto* actor : planet->GetStageObjects()) {
            auto* zone = dynamic_cast<CameraZone*>(actor);
            if (zone && zone->GetIsActive() && zone->GetSettings().enabled) {
                zones.push_back(zone);
            }
        }
    }
    CameraZone* current = nullptr;
    for (auto* zone : zones) {
        if (zone->GetStageYamlIndex() == state.selectedZone) {
            current = zone;
            break;
        }
    }
    CameraZone* selected = current && (current->Contains(player.GetPos(), current->GetSettings().exitMargin) ||
                                       (current->GetSettings().holdWhileAirborne && !player.GetOnGround()))
                               ? current
                               : nullptr;
    const bool holdsAirborneZone = current && current->GetSettings().holdWhileAirborne && !player.GetOnGround();
    if (!holdsAirborneZone) {
        for (auto* candidate : zones) {
            if (candidate->Contains(player.GetPos()) &&
                (!selected || candidate->GetSettings().priority > selected->GetSettings().priority)) {
                selected = candidate;
            }
        }
    }
    if (previewZone >= 0) {
        for (auto* zone : zones) {
            if (zone->GetStageYamlIndex() == previewZone) {
                selected = zone;
                break;
            }
        }
    }
    const float exitDuration = current ? current->GetSettings().exitSeconds : state.duration;
    const bool changedZone = state.selectedZone != (selected ? selected->GetStageYamlIndex() : -1);
    state.selectedZone = selected ? selected->GetStageYamlIndex() : -1;
    state.allowsManualRotation = !selected || selected->GetSettings().allowManualRotation;
    if (!state.allowsManualRotation) {
        state.manualInput = false;
        state.manualDelay = 0.0f;
    }
    if (selected) {
        const bool suspended = state.manualInput || state.manualDelay > 0.0f || state.elapsed < state.duration;
        state.direction.Update(selected->GetSettings(), player, selected->GetPos(), changedZone, suspended,
                               state.resetRequested, elapsed);
    }
    if (state.resetRequested) {
        state.manualDelay = 0;
        state.manualInput = false;
        state.resetRequested = false;
        if (selected) {
            state.transitionZone = -2;
        } else {
            // ゾーン外のリセットは通常カメラへ委ね、退出補間で背面合わせを打ち消さない。
            if (state.ownsMovementLock) {
                player.UnlockMovementDirectionForCameraAutoAlign();
            }
            state.ownsMovementLock = false;
            state.hasOutput = false;
            state.transitionZone = -1;
            state.transitionReturnCamera = false;
            state.elapsed = state.duration;
            state.output = normalPose;
            return normalPose;
        }
    }
    if (state.manualInput) {
        state.manualDelay = selected ? selected->GetSettings().manualReturnDelay : 1.0f;
    } else {
        state.manualDelay = std::max(0.0f, state.manualDelay - elapsed);
    }
    if (state.manualInput || state.manualDelay > 0.0f) {
        if (state.ownsMovementLock) {
            player.UnlockMovementDirectionForCameraAutoAlign();
        }
        state.ownsMovementLock = false;
        state.manualInput = false;
        constexpr float manualHandoffSeconds = 0.15f;
        if (!state.isManualMode) {
            state.from = state.output;
            state.manualBlendElapsed = state.hasOutput ? 0.0f : manualHandoffSeconds;
            state.isManualMode = true;
        }
        state.manualBlendElapsed += elapsed;
        const float progress = std::min(1.0f, state.manualBlendElapsed / manualHandoffSeconds);
        state.output = BlendPose(state.from, normalPose, progress);
        state.output.position = collision.ResolveWithClearance(
            player.GetPos() + player.GetUpVec() * 0.5f, state.output.position, player.GetUpVec(),
            CalculateCameraClearanceRadius(state.output.fieldOfViewDegrees, aspectRatio), {&player});
        state.transitionZone = -1;
        state.hasOutput = progress < 1.0f;
        return state.output;
    }
    state.isManualMode = false;
    const int desiredZone = selected ? selected->GetStageYamlIndex() : -1;
    const bool useReturnCamera = selected && selected->GetSettings().bidirectional && state.direction.IsReturnCamera();
    if (state.transitionZone != desiredZone || state.transitionReturnCamera != useReturnCamera) {
        state.from = state.output;
        state.elapsed = 0.0f;
        state.duration = selected ? (state.transitionZone == desiredZone ? selected->GetSettings().directionBlendSeconds
                                                                         : selected->GetSettings().enterSeconds)
                                  : exitDuration;
        state.transitionReturnCamera = useReturnCamera;
        state.transitionZone = desiredZone;
        player.LockMovementDirectionForCameraAutoAlign(true);
        state.ownsMovementLock = true;
    }
    CameraPose desired = normalPose;
    Actor* target = nullptr;
    if (selected) {
        const CameraZoneViewSettings& settings =
            useReturnCamera ? selected->GetSettings().returnCamera : selected->GetSettings();
        target =
            findActor && settings.targetIndex >= 0 ? findActor(settings.targetSequence, settings.targetIndex) : nullptr;
        if (target && !target->GetIsActive()) {
            target = nullptr;
        }
        desired = CalculatePose(settings, player, target, aspectRatio);
    }
    state.elapsed += elapsed;
    const float progress = state.duration <= 0.0f ? 1.0f : std::min(1.0f, state.elapsed / state.duration);
    const float eased = progress * progress * (3.0f - 2.0f * progress);
    if (progress < 1.0f) {
        state.output = BlendPose(state.from, desired, eased);
    } else if (selected && state.hasOutput) {
        // ゾーン内の追従も補間し、着地による法線・注視対象・距離の変化を直接描画へ出さない。
        constexpr float followSmoothingSpeed = 8.0f;
        const float blend = 1.0f - std::exp(-followSmoothingSpeed * elapsed);
        state.output = BlendPose(state.output, desired, blend);
    } else {
        state.output = desired;
    }
    if (state.hasOutput || selected || progress < 1.0f) {
        // 注視対象の足場も衝突対象にする。始点は注視点ではなくPlayer近傍の空間を使う。
        state.output.position = collision.ResolveWithClearance(
            player.GetPos() + player.GetUpVec() * 0.5f, state.output.position, player.GetUpVec(),
            CalculateCameraClearanceRadius(state.output.fieldOfViewDegrees, aspectRatio), {&player});
    }
    // 補間完了後も同じ入力が続く間は保持する。解除はPlayerMovementの入力判定が行う。
    state.hasOutput = selected || progress < 1.0f;
    if (state.hasOutput) {
        const auto direction = SafeDirection(state.output.position - state.output.target, player.GetForwardVec());
        player.SetCameraForwardDirection(direction, state.output.up);
    }
    return state.output;
}

bool CameraZoneController::HasActiveZone(int playerIndex) const
{
    return playerIndex >= 0 && static_cast<std::size_t>(playerIndex) < mStates.size() &&
           mStates[playerIndex].selectedZone >= 0;
}

bool CameraZoneController::AllowsManualRotation(int playerIndex) const
{
    return playerIndex < 0 || static_cast<std::size_t>(playerIndex) >= mStates.size() ||
           mStates[playerIndex].allowsManualRotation;
}

void CameraZoneController::NotifyManualInput(int playerIndex)
{
    if (playerIndex < 0) {
        return;
    }
    if (mStates.size() <= static_cast<std::size_t>(playerIndex)) {
        mStates.resize(playerIndex + 1);
    }
    mStates[playerIndex].manualInput = true;
}
void CameraZoneController::RequestReset(int playerIndex)
{
    if (playerIndex < 0) {
        return;
    }
    if (mStates.size() <= static_cast<std::size_t>(playerIndex)) {
        mStates.resize(playerIndex + 1);
    }
    mStates[playerIndex].resetRequested = true;
}
void CameraZoneController::TransitionToPlayer(int fromIndex, int toIndex, const std::vector<Player*>& players)
{
    if (toIndex < 0 || fromIndex == toIndex) {
        return;
    }
    const auto handoff = GetPose(fromIndex);
    if (mStates.size() <= static_cast<std::size_t>(toIndex)) {
        mStates.resize(toIndex + 1);
    }
    for (int index : {fromIndex, toIndex}) {
        if (index >= 0 && static_cast<std::size_t>(index) < mStates.size()) {
            if (static_cast<std::size_t>(index) < players.size() && players[index] && mStates[index].ownsMovementLock) {
                players[index]->UnlockMovementDirectionForCameraAutoAlign();
            }
            mStates[index] = {};
        }
    }
    mStates[toIndex].handoffPose = handoff;
}

void CameraZoneController::Reset(const std::vector<Player*>& players)
{
    for (std::size_t index = 0; index < std::min(players.size(), mStates.size()); ++index) {
        if (players[index] && mStates[index].ownsMovementLock) {
            players[index]->UnlockMovementDirectionForCameraAutoAlign();
        }
    }
    mStates.clear();
}
std::optional<CameraPose> CameraZoneController::GetPose(int playerIndex) const
{
    return playerIndex >= 0 && static_cast<std::size_t>(playerIndex) < mStates.size() && mStates[playerIndex].hasOutput
               ? std::optional<CameraPose>(mStates[playerIndex].output)
               : std::nullopt;
}
