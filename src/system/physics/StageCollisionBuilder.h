#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>

class btDiscreteDynamicsWorld;
class btTriangleMesh;
class btBvhTriangleMeshShape;
class btRigidBody;
class btTransform;

class Game;
class Actor;

class StageCollisionBuilder {
public:
    explicit StageCollisionBuilder(Game* game);

    void CreateStageCollisionBodies(btDiscreteDynamicsWorld* world,
                                    std::vector<std::unique_ptr<btRigidBody>>& rigidBodies,
                                    std::vector<std::unique_ptr<btBvhTriangleMeshShape>>& triangleMeshShapes,
                                    std::vector<std::unique_ptr<btTriangleMesh>>& triangleMeshes) const;

    void SyncKinematicBodies(btDiscreteDynamicsWorld* world,
                             const std::vector<std::unique_ptr<btRigidBody>>& rigidBodies) const;

    static btTransform CreateActorTransform(Game* game, Actor* actor);

private:
    void CreateStaticMeshBody(btDiscreteDynamicsWorld* world, Actor* actor,
                              std::vector<std::unique_ptr<btRigidBody>>& rigidBodies,
                              std::vector<std::unique_ptr<btBvhTriangleMeshShape>>& triangleMeshShapes,
                              std::vector<std::unique_ptr<btTriangleMesh>>& triangleMeshes) const;

    void CreateKinematicMeshBody(btDiscreteDynamicsWorld* world, Actor* actor,
                                 std::vector<std::unique_ptr<btRigidBody>>& rigidBodies,
                                 std::vector<std::unique_ptr<btBvhTriangleMeshShape>>& triangleMeshShapes,
                                 std::vector<std::unique_ptr<btTriangleMesh>>& triangleMeshes) const;

    std::unique_ptr<btTriangleMesh> CreateTriangleMesh(const glm::vec3& actorScale, const std::vector<float>& pos,
                                                       const std::vector<unsigned int>& idx) const;

private:
    Game* mGame = nullptr;
};
