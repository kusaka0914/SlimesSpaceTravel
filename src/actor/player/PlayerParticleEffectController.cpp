#include "actor/player/PlayerParticleEffectController.h"

#include "Game.h"
#include "actor/Player.h"
#include "effect/particle/ParticleTypes.h"
#include "system/ParticleSystem.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>

namespace {
constexpr float directionEpsilonSquared = 0.000001f;
constexpr float minimumLandingEffectSpeed = 1.0f;

constexpr const char* walkStartEffectId = "walk_start_dust";
constexpr const char* landingEffectId = "landing_dust";
constexpr const char* specialChargeEffectId = "player_special_charge";
constexpr float specialChargeEmissionIntervalSeconds = 0.12f;

glm::vec3 SafeNormalize(const glm::vec3& value, const glm::vec3& fallback)
{
    if (glm::dot(value, value) <= directionEpsilonSquared) {
        return fallback;
    }

    return glm::normalize(value);
}

ParticleSystem* GetParticleSystem(Player& player)
{
    Game* game = player.GetGame();
    return game ? game->GetParticleSystem() : nullptr;
}
} // namespace

void PlayerParticleEffectController::UpdateWalking(Player& player, bool isWalking)
{
    const bool didStartWalking = isWalking && !mWasWalking;

    if (didStartWalking) {
        EmitWalkStart(player);
    }

    mWasWalking = isWalking;
}

void PlayerParticleEffectController::UpdateSpecialCharging(
    Player& player,
    bool isCharging,
    float deltaTime)
{
    if (!isCharging) {
        mWasSpecialCharging = false;
        mSpecialChargeEmissionTimerSeconds = 0.0f;
        return;
    }

    if (!mWasSpecialCharging) {
        mSpecialChargeEmissionTimerSeconds = 0.0f;
    } else {
        mSpecialChargeEmissionTimerSeconds -= deltaTime;
    }
    mWasSpecialCharging = true;

    if (mSpecialChargeEmissionTimerSeconds > 0.0f) {
        return;
    }

    EmitSpecialCharge(player);
    mSpecialChargeEmissionTimerSeconds = specialChargeEmissionIntervalSeconds;
}

void PlayerParticleEffectController::EmitLanding(Player& player, float landingSpeed)
{
    if (landingSpeed < minimumLandingEffectSpeed) {
        return;
    }

    ParticleSystem* particleSystem = GetParticleSystem(player);
    if (!particleSystem) {
        return;
    }

    const glm::vec3 upDirection =
        SafeNormalize(player.GetUpVec(), glm::vec3(0.0f, 1.0f, 0.0f));

    ParticleSpawnContext context;
    context.position = player.GetPos() + upDirection * 0.05f;
    context.normal = upDirection;
    context.direction = upDirection;
    context.scale = glm::clamp(landingSpeed / 5.5f, 0.75f, 1.65f);

    particleSystem->Emit(landingEffectId, context);
}

void PlayerParticleEffectController::Reset()
{
    mWasWalking = false;
    mWasSpecialCharging = false;
    mSpecialChargeEmissionTimerSeconds = 0.0f;
}

void PlayerParticleEffectController::EmitSpecialCharge(Player& player)
{
    ParticleSystem* particleSystem = GetParticleSystem(player);
    if (!particleSystem) {
        return;
    }

    const glm::vec3 upDirection =
        SafeNormalize(player.GetUpVec(), glm::vec3(0.0f, 1.0f, 0.0f));

    ParticleSpawnContext context;
    context.position = player.GetPos() + upDirection * 0.18f;
    context.normal = upDirection;
    context.direction = upDirection;
    context.scale = 1.0f;
    particleSystem->Emit(specialChargeEffectId, context);
}

void PlayerParticleEffectController::EmitWalkStart(Player& player)
{
    ParticleSystem* particleSystem = GetParticleSystem(player);
    if (!particleSystem) {
        return;
    }

    const glm::vec3 upDirection =
        SafeNormalize(player.GetUpVec(), glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::vec3 forwardDirection =
        SafeNormalize(player.GetFacingForwardVec(), glm::vec3(0.0f, 0.0f, 1.0f));
    const glm::vec3 emissionDirection =
        SafeNormalize(-forwardDirection + upDirection * 0.25f, upDirection);

    const float radius = glm::max(0.1f, player.GetRadius());

    ParticleSpawnContext context;
    context.position =
        player.GetPos() + upDirection * 0.06f - forwardDirection * radius * 0.30f;
    context.normal = upDirection;
    context.direction = emissionDirection;
    context.scale = 1.0f;

    particleSystem->Emit(walkStartEffectId, context);
}
