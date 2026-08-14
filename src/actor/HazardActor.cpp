#include "actor/HazardActor.h"

#include "Game.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "system/PhysicsSystem.h"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

HazardActor::HazardActor(Game* game)
    : Actor(game)
{
    SetModelPath("crystal.obj");
    SetScale(glm::vec3(0.75f));
    SetRadius(mTriggerRadius);
}

void HazardActor::SetDamage(float damage)
{
    mDamage = std::max(0.0f, damage);
}

void HazardActor::SetTriggerRadius(float triggerRadius)
{
    mTriggerRadius = std::max(0.01f, triggerRadius);
    SetRadius(mTriggerRadius);
}

void HazardActor::SetDamageIntervalSeconds(
    float damageIntervalSeconds)
{
    mDamageIntervalSeconds =
        std::max(0.0f, damageIntervalSeconds);
}

void HazardActor::UpdateActor(float deltaTime)
{
    if (!mIsActive || !mGame ||
        mGame->GetIsDebugEditorShowing()) {
        return;
    }

    for (Player* player : mGame->GetPlayers()) {
        if (!player || !player->IsAlive() ||
            !IsPlayerOnSamePlanetSurface(*player)) {
            continue;
        }

        float& damageCooldownSeconds =
            mDamageCooldownSecondsByPlayer[player];
        damageCooldownSeconds = std::max(
            0.0f,
            damageCooldownSeconds - deltaTime);

        if (!IsPlayerTouching(*player)) {
            continue;
        }

        if (player->IsInvincible() ||
            damageCooldownSeconds > 0.0f) {
            continue;
        }

        player->ApplyDamageFromActor(GetPos(), mDamage);
        damageCooldownSeconds = mDamageIntervalSeconds;
    }
}

bool HazardActor::TryReactToAttack(Player& player)
{
    if (!mIsActive || !IsPlayerOnSamePlanetSurface(player) ||
        !IsWithinPlayerAttack(player)) {
        return false;
    }

    const std::uint64_t attackSequence =
        player.GetResolvedAttackSequence();
    std::uint64_t& handledSequence =
        mHandledAttackSequences[&player];
    if (attackSequence == 0 ||
        attackSequence == handledSequence) {
        return false;
    }
    handledSequence = attackSequence;

    float& damageCooldownSeconds =
        mDamageCooldownSecondsByPlayer[&player];
    if (!player.IsInvincible() &&
        damageCooldownSeconds <= 0.0f) {
        player.ApplyDamageFromActor(GetPos(), mDamage);
        damageCooldownSeconds = mDamageIntervalSeconds;
    }

    return true;
}

bool HazardActor::IsPlayerOnSamePlanetSurface(
    const Player& player) const
{
    Planet* planet = GetCurrentPlanet();
    if (!planet || player.GetCurrentPlanet() != planet) {
        return false;
    }

    return planet->ArePositionsOnSameSurfaceFace(
        player.GetPos(),
        GetPos());
}

bool HazardActor::IsPlayerTouching(const Player& player) const
{
    const PhysicsSystem* physicsSystem =
        mGame ? mGame->GetPhysicsSystem() : nullptr;
    const float playerRadius = physicsSystem
        ? 0.5f * std::max(
              physicsSystem->GetPlayerCollisionWidth(),
              physicsSystem->GetPlayerCollisionDepth()) *
              player.GetCollisionScaleMultiplier()
        : std::max(0.1f, player.GetRadius());

    const float contactDistance =
        mTriggerRadius + playerRadius;
    const glm::vec3 playerOffset =
        player.GetPos() - GetPos();
    return glm::dot(playerOffset, playerOffset) <=
           contactDistance * contactDistance;
}

bool HazardActor::IsWithinPlayerAttack(
    const Player& player) const
{
    const glm::vec3 playerToActor =
        GetPos() - player.GetPos();
    const float distance = glm::length(playerToActor);
    const float attackReach =
        player.GetAttackRange() + mTriggerRadius;
    if (distance > attackReach) {
        return false;
    }

    if (distance <= 0.000001f) {
        return true;
    }

    const glm::vec3 directionToActor =
        playerToActor / distance;
    const float facingDot = glm::dot(
        player.GetFacingForwardVec(),
        directionToActor);
    const float facingThreshold =
        std::cos(player.GetAttackAngle() * 0.5f);
    return facingDot >= facingThreshold;
}
