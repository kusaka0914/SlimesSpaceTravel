#pragma once

#include "system/PhysicsSystem.h"

#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <vector>

class btDiscreteDynamicsWorld;
class btCollisionObject;
class btCollisionShape;

class Game;
class Actor;

class EditorPickSystem {
public:
    explicit EditorPickSystem(Game* game);

    void CreatePickBodies(btDiscreteDynamicsWorld* world,
                          std::vector<std::unique_ptr<btCollisionObject>>& pickObjects,
                          std::vector<std::unique_ptr<btCollisionShape>>& pickShapes) const;

    void SyncPickBodies(btDiscreteDynamicsWorld* world,
                        const std::vector<std::unique_ptr<btCollisionObject>>& pickObjects) const;

    std::optional<PhysicsSystem::RayHitActor>
    PickActorByRay(btDiscreteDynamicsWorld* world, const glm::vec3& rayFrom, const glm::vec3& rayTo,
                   const std::vector<std::unique_ptr<btCollisionObject>>& pickObjects) const;

private:
    void CreatePickBody(btDiscreteDynamicsWorld* world, Actor* actor,
                        std::vector<std::unique_ptr<btCollisionObject>>& pickObjects,
                        std::vector<std::unique_ptr<btCollisionShape>>& pickShapes) const;

private:
    Game* mGame = nullptr;
};
