#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <vector>

class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
struct btDbvtBroadphase;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;
class btTriangleMesh;
class btBvhTriangleMeshShape;
class btRigidBody;
class btConvexShape;
class btCollisionObject;
class btCollisionShape;

class Game;
class Actor;

struct ActorMovementCollisionResult {
    glm::vec3 resolvedPosition{0.0f};
    glm::vec3 blockingNormal{0.0f};
    bool didHitStage = false;
    bool hasUnresolvedStageOverlap = false;
    bool didBlockRequestedMovement = false;
};

enum class ActorCollisionFilter { AllActors, IgnoreAirborneEnemies, IgnoreEnemies, StopAtEnemies };

class PhysicsWorldBuilder;
class StageCollisionBuilder;
class EditorPickSystem;
class FallRespawnTriggerSystem;
class ActorCollisionResolver;
class ActorModelEllipsoidShapeCache;
struct ResolvedActorModelEllipsoidShape;

// Bulletの衝突ワールドを所有し、移動解決・レイ判定・編集用選択の窓口を提供する。
class PhysicsSystem {
public:
    // 命中情報はワールド座標で保持する。actorは借用参照で、Stage再読込後は使用しない。
    struct RayHitActor {
        Actor* actor = nullptr;
        glm::vec3 hitPos{0.0f};
        glm::vec3 hitNormal{0.0f, 1.0f, 0.0f};
        float distance = 0.0f;
    };

    explicit PhysicsSystem(Game* game);
    ~PhysicsSystem();

    // 既存の衝突ワールドを破棄し、現在のStageから再構築する。Stageがなければ空の状態になる。
    void Initialize();

    // Stage・Actorの破棄前に、借用参照を持つBulletオブジェクトを除去する。
    void ClearForEditorStageRebuild();

    btDiscreteDynamicsWorld* GetBulletWorld() const { return mBulletWorld.get(); }

    // 衝突楕円体の幅・高さ・奥行きを設定する。0.1未満は0.1へ補正し、形状を再生成する。
    void SetPlayerCollisionWidth(float width);
    void SetPlayerCollisionHeight(float height);
    void SetPlayerCollisionDepth(float depth);

    // 衝突中心の高さを設定する。負の値は0へ補正し、形状自体は再生成しない。
    void SetPlayerCollisionCenterHeight(float centerHeight);

    float GetPlayerCollisionWidth() const { return mPlayerCollisionWidth; }
    float GetPlayerCollisionHeight() const { return mPlayerCollisionHeight; }
    float GetPlayerCollisionDepth() const { return mPlayerCollisionDepth; }
    float GetPlayerCollisionCenterHeight() const { return mPlayerCollisionCenterHeight; }

    // moveDeltaとdesiredPosから移動先を解決する。物理ワールド未構築時はdesiredPosをそのまま返す。
    ActorMovementCollisionResult
    ResolveMovementCollision(Actor* actor, const glm::vec3& moveDelta, const glm::vec3& desiredPos,
                             ActorCollisionFilter actorCollisionFilter = ActorCollisionFilter::AllActors);

    bool DoesActorSweepHitBlockingStage(const Actor& actor, const glm::vec3& fromPosition,
                                        const glm::vec3& toPosition) const;

    // ワールド座標の線分で編集対象を選択する。未命中・ワールド未構築時はnullopt。
    std::optional<RayHitActor> PickActorByRay(const glm::vec3& rayFrom, const glm::vec3& rayTo) const;

    // 編集対象への命中を距離順に返す。未命中・ワールド未構築時は空配列。
    std::vector<RayHitActor> PickActorsByRay(const glm::vec3& rayFrom, const glm::vec3& rayTo) const;

    // ワールド座標の線分から最初のステージ表面を返す。未命中・ワールド未構築時はnullopt。
    std::optional<RayHitActor> RaycastStageSurface(const glm::vec3& rayFrom, const glm::vec3& rayTo) const;

    // ステージ表面への命中を距離順に返す。判定前に可動物の位置を同期する。
    std::vector<RayHitActor> RaycastStageSurfaces(const glm::vec3& rayFrom, const glm::vec3& rayTo) const;

    void SyncKinematicBodies() const;

    std::optional<RayHitActor> CheckFallRespawnBySweep(const Actor* actor, const glm::vec3& from,
                                                       const glm::vec3& to) const;

    // モデルの境界箱をmovementStartから現在位置まで掃引し、対象Actorの衝突形状との接触を調べる。
    bool
    DoesActorModelSweepOverlapActorCollision(const Actor& movingActor, const glm::vec3& movementStart,
                                             const Actor& targetActor,
                                             const glm::vec3& movingModelHalfExtentPadding = glm::vec3(0.0f)) const;

    // モデル由来の楕円体を掃引する。楕円体を利用できない場合は境界箱による判定へ戻す。
    bool DoesActorEllipsoidModelSweepOverlapActorCollision(const Actor& movingActor, const glm::vec3& movementStart,
                                                           const Actor& targetActor) const;

    ResolvedActorModelEllipsoidShape ResolveActorModelEllipsoidShape(const Actor& actor) const;

private:
    void ClearBulletWorld();
    void CreateWorld();
    void CreatePlayerShape();

    Game* mGame = nullptr;

    std::unique_ptr<PhysicsWorldBuilder> mWorldBuilder;
    std::unique_ptr<StageCollisionBuilder> mStageCollisionBuilder;
    std::unique_ptr<EditorPickSystem> mEditorPickSystem;
    std::unique_ptr<FallRespawnTriggerSystem> mFallRespawnTriggerSystem;
    std::unique_ptr<ActorCollisionResolver> mActorCollisionResolver;
    std::unique_ptr<ActorModelEllipsoidShapeCache> mActorModelEllipsoidShapeCache;

    std::unique_ptr<btDefaultCollisionConfiguration> mBulletCollisionConfig;
    std::unique_ptr<btCollisionDispatcher> mBulletDispatcher;
    std::unique_ptr<btDbvtBroadphase> mBulletBroadphase;
    std::unique_ptr<btSequentialImpulseConstraintSolver> mBulletSolver;
    std::unique_ptr<btDiscreteDynamicsWorld> mBulletWorld;

    std::unique_ptr<btConvexShape> mPlayerShape;
    float mPlayerCollisionWidth = 1.6f;
    float mPlayerCollisionHeight = 0.8f;
    float mPlayerCollisionDepth = 0.8f;
    float mPlayerCollisionCenterHeight = 0.45f;

    std::vector<std::unique_ptr<btRigidBody>> mBulletRigidBodies;
    std::vector<std::unique_ptr<btBvhTriangleMeshShape>> mBulletTriangleMeshShapes;
    std::vector<std::unique_ptr<btTriangleMesh>> mBulletTriangleMeshes;

    std::vector<std::unique_ptr<btCollisionObject>> mEditorPickObjects;
    std::vector<std::unique_ptr<btCollisionShape>> mEditorPickShapes;
    std::vector<std::unique_ptr<btTriangleMesh>> mEditorPickTriangleMeshes;

    std::vector<std::unique_ptr<btCollisionObject>> mFallRespawnTriggerObjects;
    std::vector<std::unique_ptr<btCollisionShape>> mFallRespawnTriggerShapes;
};
