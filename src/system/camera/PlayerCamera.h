#pragma once

#include <cstddef>
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <vector>

class CameraCollisionResolver;
class Player;

struct PlayerCameraState {
    glm::vec3 cameraPos{0.0f};
    glm::vec3 targetPos{0.0f};
    glm::vec3 upVec{0.0f, 1.0f, 0.0f};
    glm::vec3 cameraForwardVec{0.0f, 0.0f, 1.0f};
    glm::vec3 attackTargetForwardVec{0.0f, 0.0f, 1.0f};
    glm::vec3 alignTargetForwardVec{0.0f, 0.0f, 1.0f};
    bool hasCameraForward = false;
    bool hasAttackTargetForward = false;
    bool isAligningBehindPlayer = false;
};

class PlayerCamera {
public:
    explicit PlayerCamera(CameraCollisionResolver& collisionResolver);

    void Update(const std::vector<Player*>& players, float yawDelta, float upSmoothingSpeed,
                float targetSmoothingSpeed, float attackTargetSmoothingSpeed, float deltaTime);
    void AlignBehindPlayer(Player* player, int playerIndex);

    glm::mat4 GetView(Player* player, int playerIndex, float cameraDistance, float cameraPitch,
                      float targetHeight, bool isFixed = false);
    glm::vec3 GetCameraPos(int playerIndex) const;

private:
    void ResizeState(std::size_t count);
    void UpdateState(Player* player, int playerIndex, float upSmoothingSpeed, float targetSmoothingSpeed,
                     float attackTargetSmoothingSpeed, float deltaTime);
    void UpdateCameraForward(Player* player, PlayerCameraState& state, float attackTargetSmoothingSpeed,
                             float deltaTime);

private:
    CameraCollisionResolver& mCollisionResolver;
    std::vector<PlayerCameraState> mStates;
};
