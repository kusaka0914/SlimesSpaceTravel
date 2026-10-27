#pragma once

#include "system/camera/CinematicCameraTypes.h"
#include "system/camera/zones/CameraZoneDirectionController.h"
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

class Actor;
class CameraCollisionResolver;
class CameraZone;
class Player;
class Stage;

// Playerごとのゾーン選択・補間・手動操作の猶予を管理する。ActorはYAML添字で再解決し、保持しない。
class CameraZoneController {
public:
    using FindActor = std::function<Actor*(const std::string&, int)>;

    // normalPoseは通常追従の姿勢。frameNumberが同じ呼出しでは時間を再加算しない。
    CameraPose Apply(Player& player, int playerIndex, const CameraPose& normalPose, Stage* stage,
                     const FindActor& findActor, const CameraCollisionResolver& collision, float aspectRatio,
                     float deltaTime, std::uint64_t frameNumber, bool allowsZones, int previewZone = -1);
    // 選択中のゾーンで手動回転が禁止されている場合だけfalse。
    bool AllowsManualRotation(int playerIndex) const;
    void NotifyManualInput(int playerIndex);
    void RequestReset(int playerIndex);
    bool HasActiveZone(int playerIndex) const;

    // 操作対象切替時は旧ゾーンを解除し、表示中の姿勢から新しい対象へ補間する。
    void TransitionToPlayer(int fromIndex, int toIndex, const std::vector<Player*>& players);
    // 現在のPlayer一覧を渡し、ゾーンが保持した移動方向ロックも解除する。
    void Reset(const std::vector<Player*>& players);
    std::optional<CameraPose> GetPose(int playerIndex) const;

private:
    struct State {
        int selectedZone = -1;
        int transitionZone = -1;
        bool transitionReturnCamera = false;
        CameraZoneDirectionController direction;
        std::optional<CameraPose> handoffPose;
        std::uint64_t frame = 0;
        bool initialized = false;
        bool manualInput = false;
        bool allowsManualRotation = true;
        bool isManualMode = false;
        float manualBlendElapsed = 0.0f;
        bool resetRequested = false;
        bool hasOutput = false;
        bool ownsMovementLock = false;
        float manualDelay = 0.0f;
        float elapsed = 0.0f;
        float duration = 0.7f;
        glm::vec3 previousPlayerPosition{0};
        CameraPose from{};
        CameraPose output{};
    };
    std::vector<State> mStates;
};
