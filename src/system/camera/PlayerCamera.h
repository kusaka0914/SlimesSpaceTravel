#pragma once

#include <cstddef>
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <vector>

class CameraCollisionResolver;
class Player;
struct PlayerCameraSettings;

struct PlayerCameraState {
    glm::vec3 cameraPos{0.0f};
    glm::vec3 targetPos{0.0f};
    glm::vec3 upVec{0.0f, 1.0f, 0.0f};
    glm::vec3 cameraForwardVec{0.0f, 0.0f, 1.0f};
    glm::vec3 attackTargetForwardVec{0.0f, 0.0f, 1.0f};
    glm::vec3 alignTargetForwardVec{0.0f, 0.0f, 1.0f};
    glm::vec3 autoFollowStartForwardVec{0.0f, 0.0f, 1.0f};
    glm::vec3 autoFollowTargetForwardVec{0.0f, 0.0f, 1.0f};
    glm::vec3 surfaceTraversalStartUpVec{0.0f, 1.0f, 0.0f};
    bool hasCameraForward = false;
    bool hasAttackTargetForward = false;
    bool isAligningBehindPlayer = false;
    bool isAutoFollowingBehindPlayer = false;
    bool isSurfaceTraversalAutoAligning = false;
    bool isBackwardFacingFramingActive = false;
    bool hasTriggeredAutoFollowForCurrentLateralInput = false;
    bool hasSurfaceTraversalStartUp = false;
    bool hasAutoAlignedForCurrentSurfaceTraversal = false;
    float autoFollowDelayRemainingSeconds = 0.0f;
    float lateralInputHoldSeconds = 0.0f;
    float trackedLateralInputSign = 0.0f;
    float autoFollowElapsedSeconds = 0.0f;
    float autoFollowStartOffsetRadians = 0.0f;
    float backwardMovementHoldSeconds = 0.0f;
    float backwardFacingLookAheadDistance = 0.0f;
};

class PlayerCamera {
public:
    explicit PlayerCamera(CameraCollisionResolver& collisionResolver);

    void Update(const std::vector<Player*>& players,
                const std::vector<float>& yawDeltas,
                const PlayerCameraSettings& settings, float deltaTime,
                bool allowsMovementCameraAssist);
    void Reset();
    void SnapToPlayer(Player* player, int playerIndex);
    void TransitionToPlayer(
        int fromPlayerIndex,
        Player* player,
        int toPlayerIndex);
    void SnapBehindPlayer(Player* player, int playerIndex);
    void AlignBehindPlayer(Player* player, int playerIndex);
    void BlendBehindTarget(Player* player, int playerIndex, const glm::vec3& targetPosition, float blend);

    glm::mat4 GetView(Player* player, int playerIndex, float cameraDistance, float cameraPitch,
                      float targetHeight, bool isFixed = false);
    glm::vec3 GetCameraPos(int playerIndex) const;

private:
    void ResizeState(std::size_t count);
    void UpdateState(Player* player, int playerIndex,
                     const PlayerCameraSettings& settings, float deltaTime,
                     bool allowsMovementCameraAssist);
    void UpdateAutoFollowRequest(Player* player, PlayerCameraState& state,
                                 const PlayerCameraSettings& settings,
                                 float deltaTime, bool allowsMovementCameraAssist);
    void UpdateCameraForward(Player* player, PlayerCameraState& state, float attackTargetSmoothingSpeed,
                             float autoFollowRotationDurationSeconds, float deltaTime);
    void UpdateBackwardFacingFraming(Player* player, PlayerCameraState& state,
                                     const PlayerCameraSettings& settings,
                                     float deltaTime, bool allowsMovementCameraAssist);
    void UpdateSurfaceTraversalAutoAlign(Player* player, PlayerCameraState& state,
                                         const PlayerCameraSettings& settings,
                                         bool allowsMovementCameraAssist);

private:
    CameraCollisionResolver& mCollisionResolver;
    std::vector<PlayerCameraState> mStates;
};
