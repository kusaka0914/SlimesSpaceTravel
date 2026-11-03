#include "system/camera/zones/CameraZoneController.h"

#include "system/camera/zones/CameraOrbitTransition.h"

#include "actor/CameraZone.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Player.h"
#include "system/camera/CameraCollisionResolver.h"
#include "system/camera/CameraProjection.h"
#include "system/mesh/LoadedModel.h"
#include "world/Stage.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/quaternion.hpp>

namespace {
// 箱と足場で起動条件だけを分け、構図・方向判定・補間は同じ経路を使う。
struct CameraRegion {
    Actor* actor;
    const CameraZoneSettings* settings;
    CameraZone* box;

    const CameraZoneSettings& GetSettings() const { return *settings; }
    int GetStageYamlIndex() const { return actor->GetStageYamlIndex(); }
    const std::string& GetSequence() const { return actor->GetStageSequenceName(); }
    glm::vec3 GetPos() const { return actor->GetPos(); }
    bool Contains(const Player& player, float margin = 0.0f) const
    {
        return box ? box->Contains(player.GetPos(), margin) : player.GetOnGround() && player.GetGroundActor() == actor;
    }
};

glm::vec3 SafeDirection(glm::vec3 value, glm::vec3 fallback)
{
    return glm::dot(value, value) > 0.000001f ? glm::normalize(value) : fallback;
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
        ResetState(state, !allowsZones && player.IsAlive() && player.GetIsActive() && stage);
        return normalPose;
    }
    if (state.initialized && state.frame == frameNumber) {
        return state.output;
    }
    // 補間姿勢は作り直す。操作対象切替や一時停止では、領域の選択履歴だけを復元する。
    if (!state.initialized || frameNumber > state.frame + 1 ||
        glm::distance(state.previousPlayerPosition, player.GetPos()) > 12.0f) {
        if (state.ownsMovementLock) {
            player.UnlockMovementDirectionForCameraAutoAlign();
        }
        const auto handoffPose = state.handoffPose;
        const auto splitDirection = state.splitDirection;
        const bool snapOnNextApply = state.snapOnNextApply;
        const bool resumeDirection = state.resumeDirection;
        const bool manualInput = state.manualInput;
        const bool resetRequested = state.resetRequested;
        ResetState(state, resumeDirection);
        state.resumeDirection = false;
        state.splitDirection = splitDirection;
        state.snapOnNextApply = snapOnNextApply;
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
    std::vector<CameraRegion> zones;
    for (auto* planet : stage->GetPlanets()) {
        if (!planet) {
            continue;
        }
        for (auto* actor : planet->GetStageObjects()) {
            auto* zone = dynamic_cast<CameraZone*>(actor);
            if (zone && zone->GetIsActive() && zone->GetSettings().enabled) {
                zones.push_back({zone, &zone->GetSettings(), zone});
            }
        }
        for (auto* platform : planet->GetPlatforms()) {
            if (platform && platform->GetIsActive() && !platform->IsDebugDisabled() &&
                platform->GetCollisionEnabled() && platform->GetRenderOpacity() > 0.0f &&
                platform->GetStageYamlIndex() >= 0 && platform->GetCameraSettings().enabled) {
                zones.push_back({platform, &platform->GetCameraSettings(), nullptr});
            }
        }
    }
    CameraRegion* current = nullptr;
    for (auto& region : zones) {
        auto* zone = &region;
        if (zone->GetStageYamlIndex() == state.selectedZone && zone->GetSequence() == state.selectedSequence) {
            current = zone;
            break;
        }
    }
    const bool awaitingSplitSupport =
        current && !current->box && state.splitDirection && player.GetOnGround() && !player.GetGroundActor();
    CameraRegion* selected =
        current && (current->Contains(player, current->GetSettings().exitMargin) || awaitingSplitSupport ||
                    (current->GetSettings().holdWhileAirborne && !player.GetOnGround()))
            ? current
            : nullptr;
    const bool holdsAirborneZone = current && current->GetSettings().holdWhileAirborne && !player.GetOnGround();
    if (!holdsAirborneZone && !awaitingSplitSupport) {
        for (auto& region : zones) {
            auto* candidate = &region;
            if (candidate->Contains(player) &&
                (!selected || candidate->GetSettings().priority > selected->GetSettings().priority ||
                 (candidate->GetSettings().priority == selected->GetSettings().priority && !candidate->box &&
                  selected->box))) {
                selected = candidate;
            }
        }
    }
    if (previewZone >= 0) {
        for (auto& region : zones) {
            auto* zone = &region;
            if (zone->box && zone->GetStageYamlIndex() == previewZone) {
                selected = zone;
                break;
            }
        }
    }
    const float exitDuration = current ? current->GetSettings().exitSeconds : state.duration;
    const std::string desiredSequence = selected ? selected->GetSequence() : "";
    const bool changedZone = state.selectedZone != (selected ? selected->GetStageYamlIndex() : -1) ||
                             state.selectedSequence != desiredSequence;
    state.selectedZone = selected ? selected->GetStageYamlIndex() : -1;
    state.selectedSequence = desiredSequence;
    state.allowsManualRotation = !selected || selected->GetSettings().allowManualRotation;
    if (!state.allowsManualRotation) {
        state.manualInput = false;
        state.manualDelay = 0.0f;
    }
    bool inheritedSplitDirection = false;
    if (state.splitDirection) {
        const auto& inherited = *state.splitDirection;
        inheritedSplitDirection =
            selected && inherited.zone == state.selectedZone && inherited.sequence == desiredSequence;
        if (inheritedSplitDirection) {
            state.direction = inherited.direction;
            if (!awaitingSplitSupport) {
                state.splitDirection.reset();
            }
        } else if (player.GetOnGround() && player.GetGroundActor()) {
            // 別の領域へ着地した場合は通常の進入判定を使う。接地先未確定の間は履歴を捨てない。
            state.splitDirection.reset();
        }
    }
    if (selected) {
        const bool suspended =
            inheritedSplitDirection || state.manualInput || state.manualDelay > 0.0f || state.elapsed < state.duration;
        state.direction.Update(selected->GetSettings(), player, selected->GetPos(),
                               changedZone && !inheritedSplitDirection, suspended, state.resetRequested, elapsed);
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
            state.transitionSequence.clear();
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
        state.output = CameraOrbitTransition::Blend(state.from, normalPose, progress);
        state.output.position = collision.ResolveWithClearance(
            player.GetPos() + player.GetUpVec() * 0.5f, state.output.position, player.GetUpVec(),
            CalculateCameraClearanceRadius(state.output.fieldOfViewDegrees, aspectRatio), {&player});
        state.transitionZone = -1;
        state.transitionSequence.clear();
        state.hasOutput = progress < 1.0f;
        return state.output;
    }
    state.isManualMode = false;
    const int desiredZone = selected ? selected->GetStageYamlIndex() : -1;
    const bool useReturnCamera = selected && selected->GetSettings().bidirectional && state.direction.IsReturnCamera();
    const bool changedTransition = state.transitionZone != desiredZone || state.transitionSequence != desiredSequence ||
                                   state.transitionReturnCamera != useReturnCamera;
    if (changedTransition) {
        state.from = state.output;
        state.transitionYaw.reset();
        state.elapsed = 0.0f;
        state.duration = selected ? (state.transitionZone == desiredZone && state.transitionSequence == desiredSequence
                                         ? selected->GetSettings().directionBlendSeconds
                                         : selected->GetSettings().enterSeconds)
                                  : exitDuration;
        state.transitionReturnCamera = useReturnCamera;
        state.transitionZone = desiredZone;
        state.transitionSequence = desiredSequence;
        if (!state.snapOnNextApply) {
            player.LockMovementDirectionForCameraAutoAlign(true);
            state.ownsMovementLock = true;
        }
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
    if (changedTransition && state.duration > 0.0f && !state.snapOnNextApply) {
        state.transitionYaw = CameraOrbitTransition::SelectYaw(state.from, desired, collision, aspectRatio, {&player});
    }
    if (state.snapOnNextApply) {
        state.duration = 0.0f;
        state.hasOutput = false;
        // 接地先が未確定のフレームで即時切替を消費すると、翌フレームに足場の進入補間が始まる。
        const bool awaitingSupport =
            (!selected || selected->box) &&
            ((player.GetOnGround() && !player.GetGroundActor()) || state.splitDirection.has_value());
        state.snapOnNextApply = awaitingSupport;
    }
    state.elapsed += elapsed;
    const float progress = state.duration <= 0.0f ? 1.0f : std::min(1.0f, state.elapsed / state.duration);
    const float eased = progress * progress * (3.0f - 2.0f * progress);
    if (progress < 1.0f) {
        state.output = CameraOrbitTransition::Blend(state.from, desired, eased, state.transitionYaw);
    } else if (selected && state.hasOutput) {
        // ゾーン内の追従も補間し、着地による法線・注視対象・距離の変化を直接描画へ出さない。
        constexpr float followSmoothingSpeed = 8.0f;
        const float blend = 1.0f - std::exp(-followSmoothingSpeed * elapsed);
        state.output = CameraOrbitTransition::Blend(state.output, desired, blend);
    } else {
        state.output = desired;
    }
    if (state.hasOutput || selected || progress < 1.0f) {
        // 注視対象の足場も衝突対象にする。始点は注視点ではなくPlayer近傍の空間を使う。
        state.output.position = collision.ResolveWithClearance(
            player.GetPos() + player.GetUpVec() * 0.5f, state.output.position, player.GetUpVec(),
            CalculateCameraClearanceRadius(state.output.fieldOfViewDegrees, aspectRatio), {&player});
    }
    // ゾーン内では補間後も入力方向を保持する。通常追従へ復帰した後は保持を解除する。
    state.hasOutput = selected || progress < 1.0f;
    if (!state.hasOutput && state.ownsMovementLock) {
        // 通常追従へ戻った後までワールド方向を固定すると、横入力でカメラから離れ続けてしまう。
        player.UnlockMovementDirectionForCameraAutoAlign();
        state.ownsMovementLock = false;
    }
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
void CameraZoneController::TransitionToPlayer(int fromIndex, int toIndex, const std::vector<Player*>& players,
                                              bool snap)
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
            ResetState(mStates[index], true);
        }
    }
    const bool sameSupport = fromIndex >= 0 && static_cast<std::size_t>(fromIndex) < players.size() &&
                             static_cast<std::size_t>(toIndex) < players.size() && players[fromIndex] &&
                             players[toIndex] && players[fromIndex]->GetOnGround() && players[toIndex]->GetOnGround() &&
                             players[fromIndex]->GetGroundActor() &&
                             players[fromIndex]->GetGroundActor() == players[toIndex]->GetGroundActor();
    if (sameSupport && mStates[toIndex].selectedZone < 0 && static_cast<std::size_t>(fromIndex) < mStates.size()) {
        // 未操作の分身が同じ領域にいる場合も、停止位置から行き・帰りを推測し直さない。
        // 領域外ならApplyの包含・接地判定で別領域を選び、そこで通常の進入判定を行う。
        mStates[toIndex].selectedZone = mStates[fromIndex].selectedZone;
        mStates[toIndex].selectedSequence = mStates[fromIndex].selectedSequence;
        mStates[toIndex].direction = mStates[fromIndex].direction;
        mStates[toIndex].resumeDirection = mStates[fromIndex].resumeDirection;
    }
    mStates[toIndex].handoffPose = snap ? std::nullopt : handoff;
    mStates[toIndex].snapOnNextApply = snap;
}

void CameraZoneController::InheritSplitDirection(int fromIndex, int toIndex, const std::vector<Player*>& players)
{
    if (fromIndex < 0 || toIndex < 0 || fromIndex == toIndex || static_cast<std::size_t>(toIndex) >= players.size() ||
        !players[toIndex]) {
        return;
    }
    if (mStates.size() <= static_cast<std::size_t>(toIndex)) {
        mStates.resize(toIndex + 1);
    }
    if (mStates[toIndex].ownsMovementLock) {
        players[toIndex]->UnlockMovementDirectionForCameraAutoAlign();
    }
    ResetState(mStates[toIndex], false);
    if (static_cast<std::size_t>(fromIndex) < mStates.size() && mStates[fromIndex].selectedZone >= 0) {
        auto direction = mStates[fromIndex].direction;
        direction.CancelPendingSwitch();
        mStates[toIndex].splitDirection =
            SplitDirection{mStates[fromIndex].selectedZone, mStates[fromIndex].selectedSequence, direction};
        // 引き継ぎ元と同じ領域なら退出余裕・空中維持も継承し、重なった別の箱を新規選択しない。
        mStates[toIndex].selectedZone = mStates[fromIndex].selectedZone;
        mStates[toIndex].selectedSequence = mStates[fromIndex].selectedSequence;
        mStates[toIndex].direction = direction;
        mStates[toIndex].previousPlayerPosition = players[toIndex]->GetPos();
        mStates[toIndex].resumeDirection = true;
    }
}

void CameraZoneController::ResetState(State& state, bool preserveDirection)
{
    const int selectedZone = state.selectedZone;
    const auto selectedSequence = state.selectedSequence;
    const auto playerPosition = state.previousPlayerPosition;
    auto direction = state.direction;
    const bool snapOnNextApply = state.snapOnNextApply;
    const auto splitDirection = state.splitDirection;
    state = {};
    if (preserveDirection) {
        state.splitDirection = splitDirection;
        state.snapOnNextApply = snapOnNextApply;
    }
    if (preserveDirection && selectedZone >= 0) {
        // 構図の補間状態だけを捨て、同じ足場・ゾーンへの復帰を新規進入として判定しない。
        direction.CancelPendingSwitch();
        state.selectedZone = selectedZone;
        state.selectedSequence = selectedSequence;
        state.previousPlayerPosition = playerPosition;
        state.direction = direction;
        state.resumeDirection = true;
    }
}

void CameraZoneController::Reset(const std::vector<Player*>& players, bool preserveDirection)
{
    for (std::size_t index = 0; index < mStates.size(); ++index) {
        if (index < players.size() && players[index] && mStates[index].ownsMovementLock) {
            players[index]->UnlockMovementDirectionForCameraAutoAlign();
        }
        const bool keepsPlayerDirection = preserveDirection && index < players.size() && players[index] &&
                                          players[index]->IsAlive() && players[index]->GetIsActive();
        ResetState(mStates[index], keepsPlayerDirection);
    }
    if (!preserveDirection) {
        mStates.clear();
    }
}
std::optional<CameraPose> CameraZoneController::GetPose(int playerIndex) const
{
    return playerIndex >= 0 && static_cast<std::size_t>(playerIndex) < mStates.size() && mStates[playerIndex].hasOutput
               ? std::optional<CameraPose>(mStates[playerIndex].output)
               : std::nullopt;
}
