#include "actor/HazardActor.h"

#include "Game.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "system/PhysicsSystem.h"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace {
constexpr float kMinimumTriggerHalfExtent = 0.0001f;
}

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

glm::vec3 HazardActor::CalculateScaledTriggerHalfExtents() const
{
    const glm::vec3 absoluteScale = glm::abs(GetScale());
    return glm::max(
        absoluteScale * mTriggerRadius,
        glm::vec3(kMinimumTriggerHalfExtent));
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

    const glm::vec3 worldPlayerOffset =
        player.GetPos() - GetPos();
    const glm::vec3 localPlayerOffset =
        glm::inverse(GetOrientation()) * worldPlayerOffset;
    const glm::vec3 contactHalfExtents =
        CalculateScaledTriggerHalfExtents() +
        glm::vec3(playerRadius);
    const glm::vec3 normalizedOffset =
        localPlayerOffset / contactHalfExtents;

    return glm::dot(normalizedOffset, normalizedOffset) <= 1.0f;
}

bool HazardActor::IsWithinPlayerAttack(
    const Player& player) const
{
    const glm::vec3 playerToActor =
        GetPos() - player.GetPos();
    const float distance = glm::length(playerToActor);
    const float scaledTriggerRadius =
        CalculateTriggerRadiusAlongWorldDirection(playerToActor);
    const float attackReach =
        player.GetAttackRange() + scaledTriggerRadius;
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

float HazardActor::CalculateTriggerRadiusAlongWorldDirection(
    const glm::vec3& worldDirection) const
{
    const float directionLength = glm::length(worldDirection);
    if (directionLength <= 0.000001f) {
        return 0.0f;
    }

    const glm::vec3 localDirection =
        glm::inverse(GetOrientation()) *
        (worldDirection / directionLength);
    const glm::vec3 triggerHalfExtents =
        CalculateScaledTriggerHalfExtents();
    const glm::vec3 directionByHalfExtent =
        localDirection / triggerHalfExtents;
    const float inverseRadiusSquared = glm::dot(
        directionByHalfExtent,
        directionByHalfExtent);
    if (inverseRadiusSquared <= 0.000001f) {
        return 0.0f;
    }

    return 1.0f / std::sqrt(inverseRadiusSquared);
}
