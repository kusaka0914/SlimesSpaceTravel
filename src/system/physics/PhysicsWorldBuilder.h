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

class PhysicsWorldBuilder {
public:
    void CreateBulletWorld(std::unique_ptr<btDefaultCollisionConfiguration>& collisionConfig,
                           std::unique_ptr<btCollisionDispatcher>& dispatcher,
                           std::unique_ptr<btDbvtBroadphase>& broadphase,
                           std::unique_ptr<btSequentialImpulseConstraintSolver>& solver,
                           std::unique_ptr<btDiscreteDynamicsWorld>& world) const;

    void RemoveObjectsFromWorld(btDiscreteDynamicsWorld* world,
                                const std::vector<std::unique_ptr<btCollisionObject>>& fallRespawnTriggerObjects,
                                const std::vector<std::unique_ptr<btCollisionObject>>& editorPickObjects,
                                const std::vector<std::unique_ptr<btRigidBody>>& rigidBodies) const;

    void ResetBulletWorld(std::unique_ptr<btDefaultCollisionConfiguration>& collisionConfig,
                          std::unique_ptr<btCollisionDispatcher>& dispatcher,
                          std::unique_ptr<btDbvtBroadphase>& broadphase,
                          std::unique_ptr<btSequentialImpulseConstraintSolver>& solver,
                          std::unique_ptr<btDiscreteDynamicsWorld>& world) const;
};
