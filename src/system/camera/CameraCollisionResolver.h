#pragma once

#include <glm/glm.hpp>

class Game;

class CameraCollisionResolver {
public:
    explicit CameraCollisionResolver(Game* game);

    glm::vec3 Resolve(const glm::vec3& targetPos, const glm::vec3& desiredCameraPos) const;

private:
    Game* mGame;
};
