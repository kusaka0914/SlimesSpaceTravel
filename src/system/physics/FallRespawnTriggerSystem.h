#pragma once

#include "system/PhysicsSystem.h"

#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <vector>

class btDiscreteDynamicsWorld;
class btCollisionObject;
class btCollisionShape;
class btConvexShape;

class Game;
class Actor;
class FallRespawnPoint;

// 落下復帰領域のBulletオブジェクトを生成・同期し、Actorの通過を調べる。
class FallRespawnTriggerSystem {
public:
    explicit FallRespawnTriggerSystem(Game* game);

    // 生成物の所有権を出力コンテナーへ追加し、worldへ登録する。Actorは借用し、破棄前に登録を解除すること。
    void CreateTriggerBodies(btDiscreteDynamicsWorld* world,
                             std::vector<std::unique_ptr<btCollisionObject>>& triggerObjects,
                             std::vector<std::unique_ptr<btCollisionShape>>& triggerShapes) const;

    void SyncTriggerBodies(btDiscreteDynamicsWorld* world,
                           const std::vector<std::unique_ptr<btCollisionObject>>& triggerObjects) const;

    // Actorを線分に沿って掃引し、復帰領域への最初の命中を返す。移動なし・必須引数nullptrはnullopt。
    std::optional<PhysicsSystem::RayHitActor>
    CheckFallRespawnBySweep(btDiscreteDynamicsWorld* world, btConvexShape* playerShape, const Actor* actor,
                            const glm::vec3& from, const glm::vec3& to,
                            const std::vector<std::unique_ptr<btCollisionObject>>& triggerObjects) const;

private:
    void CreateTriggerBody(btDiscreteDynamicsWorld* world, FallRespawnPoint* point,
                           std::vector<std::unique_ptr<btCollisionObject>>& triggerObjects,
                           std::vector<std::unique_ptr<btCollisionShape>>& triggerShapes) const;

    Game* mGame = nullptr;
};
