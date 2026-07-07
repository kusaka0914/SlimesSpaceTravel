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
};

class PlayerCamera {
public:
    explicit PlayerCamera(CameraCollisionResolver& collisionResolver);

    void Update(const std::vector<Player*>& players, float yawDelta, float deltaTime);

    glm::mat4 GetView(Player* player, int playerIndex, float cameraDistance, float cameraPitch, bool isFixed = false);
    glm::vec3 GetCameraPos(int playerIndex) const;

private:
    void ResizeState(std::size_t count);
    void UpdateState(Player* player, int playerIndex, float deltaTime);

private:
    CameraCollisionResolver& mCollisionResolver;
    std::vector<PlayerCameraState> mStates;
};
