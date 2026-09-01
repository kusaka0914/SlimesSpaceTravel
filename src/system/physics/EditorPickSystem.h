#pragma once

#include "system/PhysicsSystem.h"

#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <vector>

class btDiscreteDynamicsWorld;
class btCollisionObject;
class btCollisionShape;
class btTriangleMesh;

class Game;
class Actor;

class EditorPickSystem {
public:
    explicit EditorPickSystem(Game* game);

    void CreatePickBodies(btDiscreteDynamicsWorld* world,
                          std::vector<std::unique_ptr<btCollisionObject>>& pickObjects,
                          std::vector<std::unique_ptr<btCollisionShape>>& pickShapes,
                          std::vector<std::unique_ptr<btTriangleMesh>>& pickTriangleMeshes) const;

    void SyncPickBodies(btDiscreteDynamicsWorld* world,
                        const std::vector<std::unique_ptr<btCollisionObject>>& pickObjects) const;

    std::optional<PhysicsSystem::RayHitActor>
    PickActorByRay(btDiscreteDynamicsWorld* world, const glm::vec3& rayFrom, const glm::vec3& rayTo,
                   const std::vector<std::unique_ptr<btCollisionObject>>& pickObjects) const;

    std::vector<PhysicsSystem::RayHitActor>
    PickActorsByRay(btDiscreteDynamicsWorld* world, const glm::vec3& rayFrom, const glm::vec3& rayTo,
                    const std::vector<std::unique_ptr<btCollisionObject>>& pickObjects) const;

private:
    void CreatePickBody(btDiscreteDynamicsWorld* world, Actor* actor,
                        std::vector<std::unique_ptr<btCollisionObject>>& pickObjects,
                        std::vector<std::unique_ptr<btCollisionShape>>& pickShapes,
                        std::vector<std::unique_ptr<btTriangleMesh>>& pickTriangleMeshes) const;

    void CreateSpherePickBody(btDiscreteDynamicsWorld* world, Actor* actor,
                              std::vector<std::unique_ptr<btCollisionObject>>& pickObjects,
                              std::vector<std::unique_ptr<btCollisionShape>>& pickShapes) const;

    bool CreateMeshPickBody(btDiscreteDynamicsWorld* world, Actor* actor,
                            std::vector<std::unique_ptr<btCollisionObject>>& pickObjects,
                            std::vector<std::unique_ptr<btCollisionShape>>& pickShapes,
                            std::vector<std::unique_ptr<btTriangleMesh>>& pickTriangleMeshes) const;

    std::unique_ptr<btTriangleMesh> CreateTriangleMesh(const glm::vec3& actorScale,
                                                       const std::vector<float>& positions,
                                                       const std::vector<unsigned int>& indices) const;

private:
    Game* mGame = nullptr;
};
