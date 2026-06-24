#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <vector>

class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btDbvtBroadphase;
class btBroadphaseInterface;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;
class btGhostPairCallback;
class btTriangleMesh;
class btBvhTriangleMeshShape;
class btRigidBody;
class btPairCachingGhostObject;
class btCapsuleShape;
class btSphereShape;
class btKinematicCharacterController;
class btCollisionObject;
class btCollisionShape;

class Game;
class Actor;

class PhysicsSystem {
public:
    struct RayHitActor {
        Actor* actor = nullptr;
        glm::vec3 hitPos{0.0f};
        glm::vec3 hitNormal{0.0f, 1.0f, 0.0f};
        float distance = 0.0f;
    };

    PhysicsSystem(Game* game);
    ~PhysicsSystem();

    void Initialize();

    btDiscreteDynamicsWorld* GetBulletWorld() const { return mBulletWorld.get(); }

    glm::vec3 CheckCollision(Actor* Actor, const glm::vec3& moveDelta, const glm::vec3& desiredPos);

    std::optional<RayHitActor> PickActorByRay(const glm::vec3& rayFrom, const glm::vec3& rayTo) const;

private:
    void ClearBulletWorld();
    void CreateWorld();
    void CreateStaticMeshBody(Actor* actor);
    void CreateStageCollisionBodies();
    void CreatePlayerShape();
    std::unique_ptr<btTriangleMesh> CreateTriangleMesh(const glm::vec3& actorScale, const std::vector<float>& pos,
                                                       const std::vector<unsigned int>& idx);
    void CreateEditorPickBodies();
    void CreateEditorPickBody(Actor* actor);
    void SyncEditorPickBodies() const;

    std::optional<glm::vec3> CheckConflictActors(Actor* actor, const glm::vec3& desiredPos);
    std::optional<glm::vec3> CheckConflictActor(Actor* actor, const glm::vec3& desiredPos);
    std::optional<glm::vec3> CheckConflictWall(Actor* actor, const glm::vec3& moveDelta, const glm::vec3& desiredPos);

private:
    Game* mGame;

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
};