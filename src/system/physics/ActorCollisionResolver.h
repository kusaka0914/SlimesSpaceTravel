#pragma once

#include <glm/glm.hpp>
#include <optional>

class btDiscreteDynamicsWorld;
class btConvexShape;

class Actor;
enum class ActorCollisionFilter;
struct ActorMovementCollisionResult;

class ActorCollisionResolver {
public:
    ActorMovementCollisionResult CheckCollision(
        btDiscreteDynamicsWorld* world,
        btConvexShape* playerShape,
        Actor* actor,
        const glm::vec3& moveDelta,
        const glm::vec3& desiredPos,
        float collisionCenterHeight,
        ActorCollisionFilter actorCollisionFilter) const;

private:
    struct StageSweepResolution {
        glm::vec3 position{0.0f};
        glm::vec3 blockingNormal{0.0f};
    };

    struct StageOverlapResolution {
        glm::vec3 position{0.0f};
        glm::vec3 blockingNormal{0.0f};
        bool hadOverlap = false;
    };

    std::optional<glm::vec3> CheckConflictActors(
        Actor* actor,
        const glm::vec3& desiredPos,
        ActorCollisionFilter actorCollisionFilter) const;
    std::optional<glm::vec3> CheckConflictActor(
        Actor* movingActor,
        Actor* blockingActor,
        const glm::vec3& desiredPos) const;
    std::optional<StageSweepResolution> CheckConflictWall(
        btDiscreteDynamicsWorld* world,
        btConvexShape* playerShape,
        Actor* actor,
        const glm::vec3& moveDelta,
        const glm::vec3& desiredPos,
        float collisionCenterHeight) const;
    StageOverlapResolution ResolveStageOverlap(
        btDiscreteDynamicsWorld* world,
        btConvexShape* playerShape,
        Actor* actor,
        const glm::vec3& position,
        float collisionCenterHeight) const;
};
