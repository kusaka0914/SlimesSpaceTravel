#pragma once

#include <memory>
#include <vector>

class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btDbvtBroadphase;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;
class btCollisionObject;
class btRigidBody;

// Bulletワールドと依存オブジェクトの生成・登録解除・破棄順序をまとめる。所有物は呼び出し側に保持する。
class PhysicsWorldBuilder {
public:
    // 空のunique_ptr群へワールドと依存先を生成する。再構築時は既存ワールドを先に破棄しておくこと。
    void CreateBulletWorld(std::unique_ptr<btDefaultCollisionConfiguration>& collisionConfig,
                           std::unique_ptr<btCollisionDispatcher>& dispatcher,
                           std::unique_ptr<btDbvtBroadphase>& broadphase,
                           std::unique_ptr<btSequentialImpulseConstraintSolver>& solver,
                           std::unique_ptr<btDiscreteDynamicsWorld>& world) const;

    // 登録だけを解除する。渡されたオブジェクトの所有権は変更しない。
    void RemoveObjectsFromWorld(btDiscreteDynamicsWorld* world,
                                const std::vector<std::unique_ptr<btCollisionObject>>& fallRespawnTriggerObjects,
                                const std::vector<std::unique_ptr<btCollisionObject>>& editorPickObjects,
                                const std::vector<std::unique_ptr<btRigidBody>>& rigidBodies) const;

    // ワールドから依存先の順でunique_ptr群を解放する。登録オブジェクトの整理は先に行うこと。
    void ResetBulletWorld(std::unique_ptr<btDefaultCollisionConfiguration>& collisionConfig,
                          std::unique_ptr<btCollisionDispatcher>& dispatcher,
                          std::unique_ptr<btDbvtBroadphase>& broadphase,
                          std::unique_ptr<btSequentialImpulseConstraintSolver>& solver,
                          std::unique_ptr<btDiscreteDynamicsWorld>& world) const;
};
