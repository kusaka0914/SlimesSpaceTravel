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

// 編集対象の選択用Bulletオブジェクトを生成・同期し、レイによる選択を行う。
class EditorPickSystem {
public:
    explicit EditorPickSystem(Game* game);

    // 生成物の所有権を出力コンテナーへ追加し、worldへ登録する。Actorは借用し、破棄前に登録を解除すること。
    void CreatePickBodies(btDiscreteDynamicsWorld* world, std::vector<std::unique_ptr<btCollisionObject>>& pickObjects,
                          std::vector<std::unique_ptr<btCollisionShape>>& pickShapes,
                          std::vector<std::unique_ptr<btTriangleMesh>>& pickTriangleMeshes) const;

    void SyncPickBodies(btDiscreteDynamicsWorld* world,
                        const std::vector<std::unique_ptr<btCollisionObject>>& pickObjects) const;

    // ワールド座標の線分で最も近い編集対象を返す。未命中・worldがnullptrならnullopt。
    std::optional<PhysicsSystem::RayHitActor>
    PickActorByRay(btDiscreteDynamicsWorld* world, const glm::vec3& rayFrom, const glm::vec3& rayTo,
                   const std::vector<std::unique_ptr<btCollisionObject>>& pickObjects) const;

    // 命中した編集対象を距離順に返す。未命中・worldがnullptrなら空配列。
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

    std::unique_ptr<btTriangleMesh> CreateTriangleMesh(const glm::vec3& actorScale, const std::vector<float>& positions,
                                                       const std::vector<unsigned int>& indices) const;

    Game* mGame = nullptr;
};
