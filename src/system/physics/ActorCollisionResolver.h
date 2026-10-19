#pragma once

#include <glm/glm.hpp>
#include <optional>

class btDiscreteDynamicsWorld;
class btConvexShape;

class Actor;
enum class ActorCollisionFilter;
struct ActorMovementCollisionResult;

// Actorの移動経路と終点の重なりを調べ、ステージや他Actorとの衝突を解決する。
class ActorCollisionResolver {
public:
    // ワールド座標の要求移動を、有効な他Actorとの衝突、ステージとの衝突の順に解決する。
    // world・playerShapeがnullptrでも他Actorとの補正は行い、ステージとの判定だけを省略する。
    ActorMovementCollisionResult CheckCollision(btDiscreteDynamicsWorld* world, btConvexShape* playerShape,
                                                Actor* actor, const glm::vec3& moveDelta, const glm::vec3& desiredPos,
                                                float collisionCenterHeight,
                                                ActorCollisionFilter actorCollisionFilter) const;

    bool DoesSweepHitBlockingStage(btDiscreteDynamicsWorld* world, btConvexShape* actorShape, const Actor& actor,
                                   const glm::vec3& fromPosition, const glm::vec3& toPosition,
                                   float collisionCenterHeight) const;

private:
    struct StageSweepResolution {
        glm::vec3 position{0.0f};
        glm::vec3 blockingNormal{0.0f};
        bool didBlockRequestedMovement = false;
    };

    struct StageOverlapResolution {
        glm::vec3 position{0.0f};
        glm::vec3 blockingNormal{0.0f};
        bool hadOverlap = false;
        bool hasRemainingOverlap = false;
    };

    std::optional<glm::vec3> CheckConflictActors(btDiscreteDynamicsWorld* world, btConvexShape* movingActorShape,
                                                 Actor* actor, const glm::vec3& desiredPos, float collisionCenterHeight,
                                                 ActorCollisionFilter actorCollisionFilter) const;
    std::optional<glm::vec3> CheckConflictActor(btDiscreteDynamicsWorld* world, btConvexShape* movingActorShape,
                                                Actor* movingActor, Actor* blockingActor, const glm::vec3& desiredPos,
                                                float collisionCenterHeight) const;
    std::optional<StageSweepResolution> CheckConflictWall(btDiscreteDynamicsWorld* world, btConvexShape* playerShape,
                                                          Actor* actor, const glm::vec3& moveDelta,
                                                          const glm::vec3& desiredPos,
                                                          float collisionCenterHeight) const;
    StageOverlapResolution ResolveStageOverlap(btDiscreteDynamicsWorld* world, btConvexShape* playerShape, Actor* actor,
                                               const glm::vec3& position, float collisionCenterHeight) const;
};
