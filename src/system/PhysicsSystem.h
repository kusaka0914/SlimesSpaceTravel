#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <vector>

class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btDbvtBroadphase;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;
class btTriangleMesh;
class btBvhTriangleMeshShape;
class btRigidBody;
class btSphereShape;
class btCollisionObject;
class btCollisionShape;

class Game;
class Actor;

class PhysicsWorldBuilder;
class StageCollisionBuilder;
class EditorPickSystem;
class FallRespawnTriggerSystem;
class ActorCollisionResolver;

class PhysicsSystem {
public:
    struct RayHitActor {
        Actor* actor = nullptr;
        glm::vec3 hitPos{0.0f};
        glm::vec3 hitNormal{0.0f, 1.0f, 0.0f};
        float distance = 0.0f;
    };

    explicit PhysicsSystem(Game* game);
    ~PhysicsSystem();

    void Initialize();

    btDiscreteDynamicsWorld* GetBulletWorld() const { return mBulletWorld.get(); }

    glm::vec3 CheckCollision(Actor* actor, const glm::vec3& moveDelta, const glm::vec3& desiredPos);

    std::optional<RayHitActor> PickActorByRay(const glm::vec3& rayFrom, const glm::vec3& rayTo) const;

    void SyncKinematicBodies() const;

    std::optional<RayHitActor> CheckFallRespawnBySweep(const glm::vec3& from, const glm::vec3& to) const;

private:
    void ClearBulletWorld();
    void CreateWorld();
    void CreatePlayerShape();

private:
    Game* mGame = nullptr;

    std::unique_ptr<PhysicsWorldBuilder> mWorldBuilder;
    std::unique_ptr<StageCollisionBuilder> mStageCollisionBuilder;
    std::unique_ptr<EditorPickSystem> mEditorPickSystem;
    std::unique_ptr<FallRespawnTriggerSystem> mFallRespawnTriggerSystem;
    std::unique_ptr<ActorCollisionResolver> mActorCollisionResolver;

    std::unique_ptr<btDefaultCollisionConfiguration> mBulletCollisionConfig;
    std::unique_ptr<btCollisionDispatcher> mBulletDispatcher;
    std::unique_ptr<btDbvtBroadphase> mBulletBroadphase;
    std::unique_ptr<btSequentialImpulseConstraintSolver> mBulletSolver;
    std::unique_ptr<btDiscreteDynamicsWorld> mBulletWorld;

    std::unique_ptr<btSphereShape> mPlayerShape;

    std::vector<std::unique_ptr<btRigidBody>> mBulletRigidBodies;
    std::vector<std::unique_ptr<btBvhTriangleMeshShape>> mBulletTriangleMeshShapes;
    std::vector<std::unique_ptr<btTriangleMesh>> mBulletTriangleMeshes;

    std::vector<std::unique_ptr<btCollisionObject>> mEditorPickObjects;
    std::vector<std::unique_ptr<btCollisionShape>> mEditorPickShapes;

    std::vector<std::unique_ptr<btCollisionObject>> mFallRespawnTriggerObjects;
    std::vector<std::unique_ptr<btCollisionShape>> mFallRespawnTriggerShapes;
};
