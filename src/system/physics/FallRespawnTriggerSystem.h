#pragma once

#include "system/PhysicsSystem.h"

#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <vector>

class btDiscreteDynamicsWorld;
class btCollisionObject;
class btCollisionShape;
class btSphereShape;

class Game;
class FallRespawnPoint;

class FallRespawnTriggerSystem {
public:
    explicit FallRespawnTriggerSystem(Game* game);

    void CreateTriggerBodies(btDiscreteDynamicsWorld* world,
                             std::vector<std::unique_ptr<btCollisionObject>>& triggerObjects,
                             std::vector<std::unique_ptr<btCollisionShape>>& triggerShapes) const;

    void SyncTriggerBodies(btDiscreteDynamicsWorld* world,
                           const std::vector<std::unique_ptr<btCollisionObject>>& triggerObjects) const;

    std::optional<PhysicsSystem::RayHitActor>
    CheckFallRespawnBySweep(btDiscreteDynamicsWorld* world, btSphereShape* playerShape, const glm::vec3& from,
                            const glm::vec3& to,
                            const std::vector<std::unique_ptr<btCollisionObject>>& triggerObjects) const;

private:
    void CreateTriggerBody(btDiscreteDynamicsWorld* world, FallRespawnPoint* point,
                           std::vector<std::unique_ptr<btCollisionObject>>& triggerObjects,
                           std::vector<std::unique_ptr<btCollisionShape>>& triggerShapes) const;

private:
    Game* mGame = nullptr;
};
