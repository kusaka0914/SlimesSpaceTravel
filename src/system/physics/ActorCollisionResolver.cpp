#include "system/physics/ActorCollisionResolver.h"

#include "actor/Actor.h"
#include "actor/Crystal.h"
#include "actor/Enemy.h"
#include "actor/NPC.h"
#include "actor/Planet.h"

#include <algorithm>
#include <btBulletDynamicsCommon.h>
#include <vector>

glm::vec3 ActorCollisionResolver::CheckCollision(btDiscreteDynamicsWorld* world, btSphereShape* playerShape,
                                                 Actor* actor, const glm::vec3& moveDelta,
                                                 const glm::vec3& desiredPos) const
{
    if (!actor) {
        return desiredPos;
    }

    if (auto conflictPos = CheckConflictActors(actor, desiredPos)) {
        return *conflictPos;
    }

    if (!world || !playerShape) {
        return desiredPos;
    }

    if (auto conflictPos = CheckConflictWall(world, playerShape, actor, moveDelta, desiredPos)) {
        return *conflictPos;
    }

    return desiredPos;
}

std::optional<glm::vec3> ActorCollisionResolver::CheckConflictActors(Actor* actor, const glm::vec3& desiredPos) const
{
    if (!actor || !actor->GetCurrentPlanet()) {
        return std::nullopt;
    }

    std::vector<Enemy*> enemies = actor->GetCurrentPlanet()->GetEnemies();

    for (Enemy* enemy : enemies) {
        if (enemy == actor) {
            continue;
        }

        if (auto conflictPos = CheckConflictActor(enemy, desiredPos)) {
            return *conflictPos;
        }
    }

    std::vector<Crystal*> crystals = actor->GetCurrentPlanet()->GetCrystals();
    for (Crystal* crystal : crystals) {
        if (crystal == actor) {
            continue;
        }

        if (auto conflictPos = CheckConflictActor(crystal, desiredPos)) {
            return *conflictPos;
        }
    }

    std::vector<NPC*> npcs = actor->GetCurrentPlanet()->GetNPCs();
    for (NPC* npc : npcs) {
        if (npc == actor) {
            continue;
        }

        if (auto conflictPos = CheckConflictActor(npc, desiredPos)) {
            return *conflictPos;
        }
    }

    return std::nullopt;
}

std::optional<glm::vec3> ActorCollisionResolver::CheckConflictActor(Actor* actor, const glm::vec3& desiredPos) const
{
    if (!actor || !actor->GetIsActive()) {
        return std::nullopt;
    }

    const glm::vec3 actorPos = actor->GetPos();
    const glm::vec3 toDesired = desiredPos - actorPos;

    const float dist = glm::length(toDesired);
    const float radius = actor->GetRadius();

    if (dist < radius && dist > 1e-5f) {
        return actorPos + glm::normalize(toDesired) * radius;
    }

    return std::nullopt;
}

std::optional<glm::vec3> ActorCollisionResolver::CheckConflictWall(btDiscreteDynamicsWorld* world,
                                                                   btSphereShape* playerShape, Actor* actor,
                                                                   const glm::vec3& moveDelta,
                                                                   const glm::vec3& desiredPos) const
{
    if (!world || !playerShape || !actor) {
        return std::nullopt;
    }

    glm::vec3 currentPos = actor->GetPos();
    glm::vec3 actorUpVec = actor->GetUpVec();
    constexpr float actorUpMargin = 0.7f;
    glm::vec3 sweepFrom = currentPos + actorUpVec * actorUpMargin;
    glm::vec3 sweepTo = desiredPos + actorUpVec * actorUpMargin;

    btTransform fromTransform, toTransform;
    fromTransform.setIdentity();
    fromTransform.setOrigin(btVector3(sweepFrom.x, sweepFrom.y, sweepFrom.z));
    toTransform.setIdentity();
    toTransform.setOrigin(btVector3(sweepTo.x, sweepTo.y, sweepTo.z));

    btCollisionWorld::ClosestConvexResultCallback sweepCallback(fromTransform.getOrigin(), toTransform.getOrigin());

    sweepCallback.m_collisionFilterGroup = static_cast<short>(btBroadphaseProxy::DefaultFilter);
    sweepCallback.m_collisionFilterMask = static_cast<short>(btBroadphaseProxy::DefaultFilter);
    world->convexSweepTest(playerShape, fromTransform, toTransform, sweepCallback);

    if (!sweepCallback.hasHit()) {
        return std::nullopt;
    }

    const float allowFrac = std::max(0.0f, sweepCallback.m_closestHitFraction - 0.01f);
    const glm::vec3 posAfterHit = currentPos + moveDelta * allowFrac;
    const glm::vec3 hitNormGlm(sweepCallback.m_hitNormalWorld.x(), sweepCallback.m_hitNormalWorld.y(),
                               sweepCallback.m_hitNormalWorld.z());

    const glm::vec3 blocked = moveDelta * (1.0f - allowFrac);
    const glm::vec3 slideVec = blocked - hitNormGlm * glm::dot(blocked, hitNormGlm);
    const float slideEps = 1e-4f;

    if (glm::length(slideVec) > slideEps) {
        glm::vec3 slideFrom = posAfterHit + actorUpVec * actorUpMargin;
        glm::vec3 slideTo = slideFrom + slideVec;

        btTransform slideFromTransition, slideToTransition;
        slideFromTransition.setIdentity();
        slideFromTransition.setOrigin(btVector3(slideFrom.x, slideFrom.y, slideFrom.z));
        slideToTransition.setIdentity();
        slideToTransition.setOrigin(btVector3(slideTo.x, slideTo.y, slideTo.z));

        btCollisionWorld::ClosestConvexResultCallback slideCallback(slideFromTransition.getOrigin(),
                                                                    slideToTransition.getOrigin());

        slideCallback.m_collisionFilterGroup = static_cast<short>(btBroadphaseProxy::DefaultFilter);
        slideCallback.m_collisionFilterMask = static_cast<short>(btBroadphaseProxy::DefaultFilter);

        world->convexSweepTest(playerShape, slideFromTransition, slideToTransition, slideCallback);

        if (!slideCallback.hasHit()) {
            return posAfterHit + slideVec;
        }

        const float slideAllow = std::max(0.0f, slideCallback.m_closestHitFraction - 0.01f);
        return posAfterHit + slideVec * slideAllow;
    }

    return posAfterHit;
}
