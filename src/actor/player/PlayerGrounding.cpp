#include "actor/player/PlayerGrounding.h"

#include "actor/Planet.h"
#include "actor/Player.h"
#include "actor/player/PlayerCombat.h"
#include "actor/player/PlayerMovement.h"
#include "system/PhysicsSystem.h"

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>

void PlayerGrounding::OnLanded(Player& player, PlayerMovement& movement, PlayerCombat& combat)
{
    movement.SetHasUsedDodge(false);
    combat.OnLanded();
    player.GetGame()->OnLanded();
}

void PlayerGrounding::OnUpVecUpdateFailed(Player& player)
{
    if (mRayCastTimer > 0.0f) {
        return;
    }

    if (!player.GetCurrentPlanet()) {
        return;
    }

    if (player.GetCurrentPlanet()->GetPlanetShape() == Planet::PlanetShape::Normal) {
        return;
    }

    player.RefreshFallbackUpVec();
    player.SetVelocity(glm::vec3(0.0f));
    ResetRayCastTimer();
}

void PlayerGrounding::OnCastSucceeded()
{
    ResetRayCastTimer();
}

void PlayerGrounding::SnapToGround(Player& player, float upOffset, float downLength)
{
    if (glm::length(player.GetUpVec()) < 1e-6f) {
        return;
    }

    const glm::vec3 up = glm::normalize(player.GetUpVec());
    const glm::vec3 from = player.GetPos() + up * upOffset;
    const glm::vec3 to = player.GetPos() - up * downLength;

    btCollisionWorld::ClosestRayResultCallback cb(btVector3(from.x, from.y, from.z), btVector3(to.x, to.y, to.z));

    auto* bulletWorld = player.GetGame()->GetPhysicsSystem()->GetBulletWorld();
    if (!bulletWorld) {
        return;
    }

    bulletWorld->rayTest(cb.m_rayFromWorld, cb.m_rayToWorld, cb);

    if (!cb.hasHit()) {
        return;
    }

    Actor* hitActor =
        cb.m_collisionObject
            ? static_cast<Actor*>(
                  cb.m_collisionObject->getUserPointer())
            : nullptr;
    if (hitActor && !hitActor->ShouldAffectGravityDirection()) {
        player.SetVelocity(glm::vec3(0.0f));
        player.RefreshFallbackUpVec();
        player.SetOnGround(false);
        return;
    }

    const glm::vec3 hitNormal(
        cb.m_hitNormalWorld.x(),
        cb.m_hitNormalWorld.y(),
        cb.m_hitNormalWorld.z());
    if (!player.IsWalkableGroundNormal(hitNormal, up)) {
        return;
    }

    const glm::vec3 hitPos(cb.m_hitPointWorld.x(), cb.m_hitPointWorld.y(), cb.m_hitPointWorld.z());
    const ActorMovementCollisionResult collisionResult =
        player.GetGame()->GetPhysicsSystem()->ResolveMovementCollision(
            &player,
            hitPos - player.GetPos(),
            hitPos);

    if (collisionResult.hasUnresolvedStageOverlap) {
        return;
    }

    const bool wasSnapBlockedByNonGround =
        collisionResult.didHitStage &&
        !player.IsWalkableGroundNormal(
            collisionResult.blockingNormal,
            up);
    if (wasSnapBlockedByNonGround) {
        return;
    }

    player.SetPos(collisionResult.resolvedPosition);
    player.SetOnGround(true);
    player.SetVelocity(glm::vec3(0.0f));
}

void PlayerGrounding::UpdateRayCastTimer(float deltaTime)
{
    if (mRayCastTimer >= 0.0f) {
        mRayCastTimer -= deltaTime;
    }
}
