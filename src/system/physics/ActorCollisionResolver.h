#pragma once

#include <glm/glm.hpp>
#include <optional>

class btDiscreteDynamicsWorld;
class btSphereShape;

class Actor;

class ActorCollisionResolver {
public:
    glm::vec3 CheckCollision(btDiscreteDynamicsWorld* world, btSphereShape* playerShape, Actor* actor,
                             const glm::vec3& moveDelta, const glm::vec3& desiredPos) const;

private:
    std::optional<glm::vec3> CheckConflictActors(Actor* actor, const glm::vec3& desiredPos) const;
    std::optional<glm::vec3> CheckConflictActor(Actor* actor, const glm::vec3& desiredPos) const;
    std::optional<glm::vec3> CheckConflictWall(btDiscreteDynamicsWorld* world, btSphereShape* playerShape,
                                               Actor* actor, const glm::vec3& moveDelta,
                                               const glm::vec3& desiredPos) const;
};
