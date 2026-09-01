#pragma once

#include <glm/glm.hpp>
#include <vector>

class Game;
class Actor;

class CameraCollisionResolver {
public:
    explicit CameraCollisionResolver(Game* game);

    glm::vec3 Resolve(
        const glm::vec3& targetPos,
        const glm::vec3& desiredCameraPos,
        const Actor* ignoredActor = nullptr) const;
    glm::vec3 Resolve(
        const glm::vec3& targetPos,
        const glm::vec3& desiredCameraPos,
        const std::vector<Actor*>& ignoredActors) const;
    bool HasClearLineOfSight(
        const glm::vec3& cameraPos,
        const glm::vec3& targetPos,
        const std::vector<Actor*>& ignoredActors) const;
    bool HasCameraClearance(
        const glm::vec3& cameraPos,
        float clearanceRadius,
        const std::vector<Actor*>& ignoredActors) const;

private:
    glm::vec3 ResolveInternal(
        const glm::vec3& targetPos,
        const glm::vec3& desiredCameraPos,
        const Actor* ignoredActor,
        const std::vector<Actor*>* ignoredActors) const;

    Game* mGame;
};
