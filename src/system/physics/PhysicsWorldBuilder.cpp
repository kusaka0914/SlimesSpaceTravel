#include "system/physics/PhysicsWorldBuilder.h"

#include <btBulletDynamicsCommon.h>

void PhysicsWorldBuilder::CreateBulletWorld(std::unique_ptr<btDefaultCollisionConfiguration>& collisionConfig,
                                            std::unique_ptr<btCollisionDispatcher>& dispatcher,
                                            std::unique_ptr<btDbvtBroadphase>& broadphase,
                                            std::unique_ptr<btSequentialImpulseConstraintSolver>& solver,
                                            std::unique_ptr<btDiscreteDynamicsWorld>& world) const
{
    collisionConfig = std::make_unique<btDefaultCollisionConfiguration>();
    dispatcher = std::make_unique<btCollisionDispatcher>(collisionConfig.get());
    broadphase = std::make_unique<btDbvtBroadphase>();
    solver = std::make_unique<btSequentialImpulseConstraintSolver>();
    world = std::make_unique<btDiscreteDynamicsWorld>(dispatcher.get(), broadphase.get(), solver.get(),
                                                      collisionConfig.get());
    world->setGravity(btVector3(0, -9.8f, 0));
}

void PhysicsWorldBuilder::RemoveObjectsFromWorld(
    btDiscreteDynamicsWorld* world, const std::vector<std::unique_ptr<btCollisionObject>>& fallRespawnTriggerObjects,
    const std::vector<std::unique_ptr<btCollisionObject>>& editorPickObjects,
    const std::vector<std::unique_ptr<btRigidBody>>& rigidBodies) const
{
    if (!world) {
        return;
    }

    for (const auto& triggerObject : fallRespawnTriggerObjects) {
        if (triggerObject) {
            world->removeCollisionObject(triggerObject.get());
        }
    }

    for (const auto& pickObject : editorPickObjects) {
        if (pickObject) {
            world->removeCollisionObject(pickObject.get());
        }
    }

    for (const auto& rigidBody : rigidBodies) {
        if (rigidBody) {
            world->removeRigidBody(rigidBody.get());
        }
    }
}

void PhysicsWorldBuilder::ResetBulletWorld(std::unique_ptr<btDefaultCollisionConfiguration>& collisionConfig,
                                           std::unique_ptr<btCollisionDispatcher>& dispatcher,
                                           std::unique_ptr<btDbvtBroadphase>& broadphase,
                                           std::unique_ptr<btSequentialImpulseConstraintSolver>& solver,
                                           std::unique_ptr<btDiscreteDynamicsWorld>& world) const
{
    world.reset();
    solver.reset();
    broadphase.reset();
    dispatcher.reset();
    collisionConfig.reset();
}
