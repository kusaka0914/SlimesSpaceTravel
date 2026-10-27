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

// Playerごとの箱・足場カメラの選択と補間を管理する。ActorはYAML列名と添字で再解決し、保持しない。
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

    // 操作対象切替時は表示姿勢を引き継ぐ。同じ領域へ戻る場合は、そのPlayerの行き・帰りを復元する。
    // snap指定時は切替先の構図を即座に適用し、進入補間も行わない。
    void TransitionToPlayer(int fromIndex, int toIndex, const std::vector<Player*>& players, bool snap = false);
    // 分裂成功時に旧履歴を消し、分裂元の方向を最初の同一領域での判定へ引き継ぐ。
    void InheritSplitDirection(int fromIndex, int toIndex, const std::vector<Player*>& players);

    // 現在のPlayer一覧を渡し、ゾーンが保持した移動方向ロックも解除する。
    // 演出等による一時停止ではpreserveDirectionを指定し、領域と行き・帰りを維持する。
    void Reset(const std::vector<Player*>& players, bool preserveDirection = false);
    std::optional<CameraPose> GetPose(int playerIndex) const;

private:
    struct SplitDirection {
        int zone = -1;
        std::string sequence;
        CameraZoneDirectionController direction;
    };
    struct State {
        int selectedZone = -1;
        std::string selectedSequence;
        int transitionZone = -1;
        std::string transitionSequence;
        bool transitionReturnCamera = false;
        CameraZoneDirectionController direction;
        std::optional<SplitDirection> splitDirection;
        std::optional<CameraPose> handoffPose;
        bool resumeDirection = false;
        bool snapOnNextApply = false;
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
        std::optional<float> transitionYaw;
        float elapsed = 0.0f;
        float duration = 0.7f;
        glm::vec3 previousPlayerPosition{0};
        CameraPose from{};
        CameraPose output{};
    };
    static void ResetState(State& state, bool preserveDirection);
    std::vector<State> mStates;
};
