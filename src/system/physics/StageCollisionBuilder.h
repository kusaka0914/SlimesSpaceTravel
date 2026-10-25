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

// Stage上のActorから静的・可動のメッシュ衝突形状を生成し、可動物の姿勢を同期する。
class StageCollisionBuilder {
public:
    explicit StageCollisionBuilder(Game* game);

    // 生成物の所有権を出力コンテナーへ追加し、worldへ登録する。Actorは借用し、破棄前に登録を解除すること。
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

    std::unique_ptr<btTriangleMesh> CreateTriangleMesh(const glm::vec3& actorScale, const std::vector<float>& positions,
                                                       const std::vector<unsigned int>& indices) const;

    Game* mGame = nullptr;
};
