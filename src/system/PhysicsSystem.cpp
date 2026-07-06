#include "system/PhysicsSystem.h"

#include "Game.h"
#include "Stage.h"
#include "system/physics/ActorCollisionResolver.h"
#include "system/physics/EditorPickSystem.h"
#include "system/physics/FallRespawnTriggerSystem.h"
#include "system/physics/PhysicsWorldBuilder.h"
#include "system/physics/StageCollisionBuilder.h"

#include <btBulletDynamicsCommon.h>
#include <memory>

PhysicsSystem::PhysicsSystem(Game* game)
    : mGame(game)
{
    mWorldBuilder = std::make_unique<PhysicsWorldBuilder>();
    mStageCollisionBuilder = std::make_unique<StageCollisionBuilder>(mGame);
    mEditorPickSystem = std::make_unique<EditorPickSystem>(mGame);
    mFallRespawnTriggerSystem = std::make_unique<FallRespawnTriggerSystem>(mGame);
    mActorCollisionResolver = std::make_unique<ActorCollisionResolver>();
}

PhysicsSystem::~PhysicsSystem()
{
    ClearBulletWorld();
}

void PhysicsSystem::Initialize()
{
    ClearBulletWorld();

    if (!mGame || !mGame->GetCurrentStage()) {
        return;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();
    if (planets.empty()) {
        return;
    }

    mWorldBuilder->CreateBulletWorld(mBulletCollisionConfig, mBulletDispatcher, mBulletBroadphase, mBulletSolver,
                                     mBulletWorld);

    if (!mBulletWorld) {
        return;
    }

    CreateWorld();
}

void PhysicsSystem::ClearBulletWorld()
{
    mWorldBuilder->RemoveObjectsFromWorld(mBulletWorld.get(), mFallRespawnTriggerObjects, mEditorPickObjects,
                                          mBulletRigidBodies);

    mFallRespawnTriggerObjects.clear();
    mFallRespawnTriggerShapes.clear();

    mEditorPickObjects.clear();
    mEditorPickShapes.clear();

    // world から外した後に所有物を破棄
    mPlayerShape.reset();
    mBulletRigidBodies.clear();
    mBulletTriangleMeshShapes.clear();
    mBulletTriangleMeshes.clear();

    // world は依存先より先に破棄する
    mWorldBuilder->ResetBulletWorld(mBulletCollisionConfig, mBulletDispatcher, mBulletBroadphase, mBulletSolver,
                                    mBulletWorld);
}

void PhysicsSystem::CreateWorld()
{
    mStageCollisionBuilder->CreateStageCollisionBodies(mBulletWorld.get(), mBulletRigidBodies,
                                                       mBulletTriangleMeshShapes, mBulletTriangleMeshes);
    mFallRespawnTriggerSystem->CreateTriggerBodies(mBulletWorld.get(), mFallRespawnTriggerObjects,
                                                   mFallRespawnTriggerShapes);
    mEditorPickSystem->CreatePickBodies(mBulletWorld.get(), mEditorPickObjects, mEditorPickShapes);
    CreatePlayerShape();
}

void PhysicsSystem::CreatePlayerShape()
{
    constexpr float playerRadius = 0.6f;
    mPlayerShape = std::make_unique<btSphereShape>(playerRadius);
}

void PhysicsSystem::SyncKinematicBodies() const
{
    mStageCollisionBuilder->SyncKinematicBodies(mBulletWorld.get(), mBulletRigidBodies);
}

std::optional<PhysicsSystem::RayHitActor> PhysicsSystem::PickActorByRay(const glm::vec3& rayFrom,
                                                                        const glm::vec3& rayTo) const
{
    if (!mBulletWorld) {
        return std::nullopt;
    }

    SyncKinematicBodies();
    return mEditorPickSystem->PickActorByRay(mBulletWorld.get(), rayFrom, rayTo, mEditorPickObjects);
}

std::optional<PhysicsSystem::RayHitActor> PhysicsSystem::CheckFallRespawnBySweep(const glm::vec3& from,
                                                                                 const glm::vec3& to) const
{
    return mFallRespawnTriggerSystem->CheckFallRespawnBySweep(mBulletWorld.get(), mPlayerShape.get(), from, to,
                                                              mFallRespawnTriggerObjects);
}

glm::vec3 PhysicsSystem::CheckCollision(Actor* actor, const glm::vec3& moveDelta, const glm::vec3& desiredPos)
{
    if (!mBulletWorld || !mPlayerShape) {
        return desiredPos;
    }

    SyncKinematicBodies();
    return mActorCollisionResolver->CheckCollision(mBulletWorld.get(), mPlayerShape.get(), actor, moveDelta,
                                                   desiredPos);
}
