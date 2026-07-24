#include "system/physics/StageCollisionBuilder.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/FallRespawnPoint.h"
#include "actor/MovingPlatform.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/StageObject.h"
#include "system/MeshLoadSystem.h"
#include "utils/MathUtils.h"

#include <btBulletDynamicsCommon.h>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <string>

StageCollisionBuilder::StageCollisionBuilder(Game* game)
    : mGame(game)
{
}

void StageCollisionBuilder::CreateStageCollisionBodies(
    btDiscreteDynamicsWorld* world, std::vector<std::unique_ptr<btRigidBody>>& rigidBodies,
    std::vector<std::unique_ptr<btBvhTriangleMeshShape>>& triangleMeshShapes,
    std::vector<std::unique_ptr<btTriangleMesh>>& triangleMeshes) const
{
    if (!mGame || !mGame->GetCurrentStage() || !world) {
        return;
    }

    const std::vector<Planet*> planets = mGame->GetCurrentStage()->GetPlanets();

    for (Planet* planet : planets) {
        if (!planet) {
            continue;
        }

        CreateStaticMeshBody(world, planet, rigidBodies, triangleMeshShapes, triangleMeshes);

        for (Platform* platform : planet->GetPlatforms()) {
            CreateStaticMeshBody(world, platform, rigidBodies, triangleMeshShapes, triangleMeshes);
        }

        for (MovingPlatform* platform : planet->GetMovingPlatforms()) {
            CreateKinematicMeshBody(world, platform, rigidBodies, triangleMeshShapes, triangleMeshes);
        }

        for (StageObject* stageObject : planet->GetStageObjects()) {
            if (stageObject && stageObject->GetCollisionEnabled()) {
                CreateStaticMeshBody(world, stageObject, rigidBodies, triangleMeshShapes, triangleMeshes);
            }
        }
    }
}

void StageCollisionBuilder::CreateStaticMeshBody(
    btDiscreteDynamicsWorld* world, Actor* actor, std::vector<std::unique_ptr<btRigidBody>>& rigidBodies,
    std::vector<std::unique_ptr<btBvhTriangleMeshShape>>& triangleMeshShapes,
    std::vector<std::unique_ptr<btTriangleMesh>>& triangleMeshes) const
{
    if (!actor || !world || !mGame || !mGame->GetMeshLoadSystem()) {
        return;
    }

    const std::string actorModelPath = "../assets/models/" + actor->GetModelPath();

    std::vector<float> pos;
    std::vector<unsigned int> idx;

    if (!mGame->GetMeshLoadSystem()->LoadMeshPositionsAndIndices(actorModelPath.c_str(), pos, idx) || pos.size() < 9 ||
        idx.size() < 3) {
        return;
    }

    const glm::vec3& actorScale = actor->GetScale();
    auto triangleMesh = CreateTriangleMesh(actorScale, pos, idx);

    if (!triangleMesh) {
        return;
    }

    auto triangleMeshShape = std::make_unique<btBvhTriangleMeshShape>(triangleMesh.get(), true);

    btRigidBody::btRigidBodyConstructionInfo rigidBodyConstructionInfo(0.0f, nullptr, triangleMeshShape.get());

    auto rigidBody = std::make_unique<btRigidBody>(rigidBodyConstructionInfo);
    rigidBody->setUserPointer(actor);

    btTransform actorTransform = CreateActorTransform(mGame, actor);
    rigidBody->setWorldTransform(actorTransform);

    rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);

    triangleMeshes.emplace_back(std::move(triangleMesh));
    triangleMeshShapes.emplace_back(std::move(triangleMeshShape));
    rigidBodies.emplace_back(std::move(rigidBody));

    world->addRigidBody(rigidBodies.back().get(), static_cast<short>(btBroadphaseProxy::DefaultFilter),
                        static_cast<short>(-1));
}

void StageCollisionBuilder::CreateKinematicMeshBody(
    btDiscreteDynamicsWorld* world, Actor* actor, std::vector<std::unique_ptr<btRigidBody>>& rigidBodies,
    std::vector<std::unique_ptr<btBvhTriangleMeshShape>>& triangleMeshShapes,
    std::vector<std::unique_ptr<btTriangleMesh>>& triangleMeshes) const
{
    if (!actor || !world || !mGame || !mGame->GetMeshLoadSystem()) {
        return;
    }

    const std::string actorModelPath = "../assets/models/" + actor->GetModelPath();

    std::vector<float> pos;
    std::vector<unsigned int> idx;

    if (!mGame->GetMeshLoadSystem()->LoadMeshPositionsAndIndices(actorModelPath.c_str(), pos, idx) || pos.size() < 9 ||
        idx.size() < 3) {
        return;
    }

    const glm::vec3& actorScale = actor->GetScale();
    auto triangleMesh = CreateTriangleMesh(actorScale, pos, idx);

    if (!triangleMesh) {
        return;
    }

    auto triangleMeshShape = std::make_unique<btBvhTriangleMeshShape>(triangleMesh.get(), true);

    btRigidBody::btRigidBodyConstructionInfo rigidBodyConstructionInfo(0.0f, nullptr, triangleMeshShape.get());

    auto rigidBody = std::make_unique<btRigidBody>(rigidBodyConstructionInfo);
    rigidBody->setUserPointer(actor);

    btTransform actorTransform = CreateActorTransform(mGame, actor);
    rigidBody->setWorldTransform(actorTransform);

    rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    rigidBody->setActivationState(DISABLE_DEACTIVATION);

    triangleMeshes.emplace_back(std::move(triangleMesh));
    triangleMeshShapes.emplace_back(std::move(triangleMeshShape));
    rigidBodies.emplace_back(std::move(rigidBody));

    world->addRigidBody(rigidBodies.back().get(), static_cast<short>(btBroadphaseProxy::DefaultFilter),
                        static_cast<short>(-1));
}

void StageCollisionBuilder::SyncKinematicBodies(
    btDiscreteDynamicsWorld* world, const std::vector<std::unique_ptr<btRigidBody>>& rigidBodies) const
{
    if (!world) {
        return;
    }

    for (const auto& rigidBody : rigidBodies) {
        if (!rigidBody) {
            continue;
        }

        Actor* actor = static_cast<Actor*>(rigidBody->getUserPointer());

        if (!actor) {
            continue;
        }

        if (!dynamic_cast<MovingPlatform*>(actor)) {
            continue;
        }

        const btTransform actorTransform = CreateActorTransform(mGame, actor);

        rigidBody->setWorldTransform(actorTransform);
        rigidBody->setInterpolationWorldTransform(actorTransform);

        world->updateSingleAabb(rigidBody.get());
    }
}

btTransform StageCollisionBuilder::CreateActorTransform(Game* game, Actor* actor)
{
    btTransform actorTransform;
    actorTransform.setIdentity();

    if (!actor) {
        return actorTransform;
    }

    const glm::vec3& actorPos = actor->GetPos();
    actorTransform.setOrigin(btVector3(actorPos.x, actorPos.y, actorPos.z));

    if (dynamic_cast<Platform*>(actor) || dynamic_cast<MovingPlatform*>(actor) ||
        dynamic_cast<StageObject*>(actor) ||
        dynamic_cast<FallRespawnPoint*>(actor)) {
        if (!game || !game->GetMathUtils()) {
            return actorTransform;
        }

        glm::mat4 orient = game->GetMathUtils()->CreateOrient(actor);

        glm::vec3 axisX = glm::normalize(glm::vec3(orient[0]));
        glm::vec3 axisY = glm::normalize(glm::vec3(orient[1]));
        glm::vec3 axisZ = glm::normalize(glm::vec3(orient[2]));

        btMatrix3x3 basis(axisX.x, axisY.x, axisZ.x, axisX.y, axisY.y, axisZ.y, axisX.z, axisY.z, axisZ.z);
        actorTransform.setBasis(basis);
    }

    return actorTransform;
}

std::unique_ptr<btTriangleMesh> StageCollisionBuilder::CreateTriangleMesh(const glm::vec3& actorScale,
                                                                          const std::vector<float>& pos,
                                                                          const std::vector<unsigned int>& idx) const
{
    auto triangleMesh = std::make_unique<btTriangleMesh>();
    for (size_t i = 0; i + 2 < idx.size(); i += 3) {
        const unsigned int idx0 = idx[i];
        const unsigned int idx1 = idx[i + 1];
        const unsigned int idx2 = idx[i + 2];

        if (idx0 * 3 + 2 >= pos.size() || idx1 * 3 + 2 >= pos.size() || idx2 * 3 + 2 >= pos.size()) {
            return nullptr;
        }

        const btVector3 v0(actorScale.x * pos[idx0 * 3], actorScale.y * pos[idx0 * 3 + 1],
                           actorScale.z * pos[idx0 * 3 + 2]);
        const btVector3 v1(actorScale.x * pos[idx1 * 3], actorScale.y * pos[idx1 * 3 + 1],
                           actorScale.z * pos[idx1 * 3 + 2]);
        const btVector3 v2(actorScale.x * pos[idx2 * 3], actorScale.y * pos[idx2 * 3 + 1],
                           actorScale.z * pos[idx2 * 3 + 2]);

        triangleMesh->addTriangle(v0, v1, v2);
    }
    return triangleMesh;
}
