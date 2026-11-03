#include "actor/player/PlayerParticleEffectController.h"

#include "actor/Player.h"
#include "effect/particle/ParticleTypes.h"
#include "system/ParticleSystem.h"

#include <algorithm>
#include <cmath>
#include <glm/common.hpp>
#include <glm/geometric.hpp>

namespace {
constexpr float directionEpsilonSquared = 0.000001f;
constexpr float minimumLandingEffectSpeed = 1.0f;

constexpr const char* walkStartEffectId = "walk_start_dust";
constexpr const char* landingEffectId = "landing_dust";
constexpr const char* specialChargeEffectId = "player_special_charge";
constexpr float specialChargeEmissionIntervalSeconds = 0.12f;
constexpr float dashEmissionIntervalSeconds = 0.035f;

glm::vec3 SafeNormalize(const glm::vec3& value, const glm::vec3& fallback)
{
    if (glm::dot(value, value) <= directionEpsilonSquared) {
        return fallback;
    }

    return glm::normalize(value);
}

} // namespace

void PlayerParticleEffectController::UpdateWalking(Player& player, ParticleSystem& particles, bool isWalking)
{
    const bool didStartWalking = isWalking && !mWasWalking;

    if (didStartWalking) {
        EmitWalkStart(player, particles);
    }

    mWasWalking = isWalking;
}

void PlayerParticleEffectController::UpdateDashing(Player& player, ParticleSystem& particles, bool isDashing,
                                                   const glm::vec3& direction, float deltaTime)
{
    if (!isDashing) {
        mDashEmissionTimerSeconds = 0.0f;
        return;
    }

    mDashEmissionTimerSeconds -= std::max(0.0f, deltaTime);
    if (mDashEmissionTimerSeconds > 0.0f) {
        return;
    }
    mDashEmissionTimerSeconds = dashEmissionIntervalSeconds;

    const glm::vec3 up = SafeNormalize(player.GetUpVec(), glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::vec3 forward = SafeNormalize(direction, player.GetFacingForwardVec());
    const glm::vec3 side = SafeNormalize(glm::cross(up, forward), player.GetLeftVec());
    const float bodyScale = player.GetBodyScaleMultiplier();
    const glm::vec3 center = player.GetPos() + up * (0.30f * bodyScale);

    // カメラの向きに依存せず、体の四方から実際のダッシュ方向と逆へ流す。
    const glm::vec3 offsets[] = {side * 0.32f, -side * 0.32f, up * 0.25f, -up * 0.25f};
    for (const glm::vec3& offset : offsets) {
        ParticleSpawnContext context;
        context.position = center + offset * bodyScale;
        context.normal = up;
        context.direction = -forward;
        context.scale = bodyScale;
        particles.Emit("player_dash_wind", context);
    }
}

void PlayerParticleEffectController::UpdateSpecialCharging(Player& player, ParticleSystem& particles, bool isCharging,
                                                           float deltaTime)
{
    if (!isCharging) {
        mPreviousSpecialChargeStage = -1;
        mSpecialChargeEmissionTimerSeconds = 0.0f;
        return;
    }

    const int chargeStage = std::clamp(player.GetSpecialAttackJewelsConsumed(), 0, 2);
    const bool didReachFullCharge = chargeStage == 2 && mPreviousSpecialChargeStage != 2;
    if (chargeStage != mPreviousSpecialChargeStage) {
        // 消費したフレームに見た目を切り替え、次の定期放出まで待たせない。
        mSpecialChargeEmissionTimerSeconds = 0.0f;
    } else {
        mSpecialChargeEmissionTimerSeconds -= std::max(0.0f, deltaTime);
    }
    mPreviousSpecialChargeStage = chargeStage;

    if (mSpecialChargeEmissionTimerSeconds > 0.0f) {
        return;
    }

    const float auraScale = chargeStage == 0 ? 0.65f : (chargeStage == 1 ? 1.0f : 1.5f);
    EmitChargeAura(player, particles, didReachFullCharge ? 2.0f : auraScale);
    mSpecialChargeEmissionTimerSeconds = specialChargeEmissionIntervalSeconds;
}

void PlayerParticleEffectController::UpdateAirSlamReadiness(Player& player, ParticleSystem& particles, bool isEnabled,
                                                            float deltaTime)
{
    // 発動後は下降してもオーラを弱めず、確定した威力を着地まで示し続ける。
    const bool isAirSlamActive = player.GetActionState() == Player::ActionState::AirSlamAttacking;
    const float height = isAirSlamActive ? player.GetAirSlamStartHeight() : player.CalculateAirSlamHeight();
    // 足場の傾きや接地誤差を含む通常ジャンプでは出ないよう、最高点に余裕を持たせる。
    const float minimumEffectHeight = std::max(0.0f, player.GetJumpHeight()) * 1.5f + 0.5f;
    if (!isEnabled || !std::isfinite(height) || height <= minimumEffectHeight) {
        mAirSlamEmissionTimerSeconds = 0.0f;
        mWasAirSlamAtFullPower = false;
        return;
    }

    const float fullDamageHeight = std::max(0.001f, player.GetAirSlamFullDamageHeight());
    const float heightRatio = glm::clamp(height / fullDamageHeight, 0.0f, 1.0f);
    const bool isAtFullPower = heightRatio >= 1.0f;
    if (isAtFullPower && !mWasAirSlamAtFullPower) {
        // 最大到達は大きな一度の発光、その後は大きいオーラの継続で区別する。
        EmitChargeAura(player, particles, 2.0f);
        mAirSlamEmissionTimerSeconds = specialChargeEmissionIntervalSeconds;
    } else {
        mAirSlamEmissionTimerSeconds -= std::max(0.0f, deltaTime);
        if (mAirSlamEmissionTimerSeconds <= 0.0f) {
            EmitChargeAura(player, particles, isAtFullPower ? 1.5f : glm::mix(0.65f, 1.2f, heightRatio));
            mAirSlamEmissionTimerSeconds = glm::mix(0.18f, 0.08f, heightRatio);
        }
    }
    mWasAirSlamAtFullPower = isAtFullPower;
}

void PlayerParticleEffectController::EmitLanding(Player& player, ParticleSystem& particles, float landingSpeed)
{
    if (landingSpeed < minimumLandingEffectSpeed) {
        return;
    }

    const glm::vec3 upDirection = SafeNormalize(player.GetUpVec(), glm::vec3(0.0f, 1.0f, 0.0f));

    ParticleSpawnContext context;
    context.position = player.GetPos() + upDirection * 0.05f;
    context.normal = upDirection;
    context.direction = upDirection;
    context.scale = glm::clamp(landingSpeed / 5.5f, 0.75f, 1.65f);

    particles.Emit(landingEffectId, context);
}

void PlayerParticleEffectController::Reset()
{
    mWasWalking = false;
    mDashEmissionTimerSeconds = 0.0f;
    mPreviousSpecialChargeStage = -1;
    mSpecialChargeEmissionTimerSeconds = 0.0f;
    mAirSlamEmissionTimerSeconds = 0.0f;
    mWasAirSlamAtFullPower = false;
}

void PlayerParticleEffectController::EmitChargeAura(Player& player, ParticleSystem& particles, float scale)
{
    const glm::vec3 upDirection = SafeNormalize(player.GetUpVec(), glm::vec3(0.0f, 1.0f, 0.0f));

    ParticleSpawnContext context;
    context.position = player.GetPos() + upDirection * 0.18f;
    context.normal = upDirection;
    context.direction = upDirection;
    context.scale = scale;
    particles.Emit(specialChargeEffectId, context);
}

void PlayerParticleEffectController::EmitWalkStart(Player& player, ParticleSystem& particles)
{
    const glm::vec3 upDirection = SafeNormalize(player.GetUpVec(), glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::vec3 forwardDirection = SafeNormalize(player.GetFacingForwardVec(), glm::vec3(0.0f, 0.0f, 1.0f));
    const glm::vec3 emissionDirection = SafeNormalize(-forwardDirection + upDirection * 0.25f, upDirection);

    const float radius = glm::max(0.1f, player.GetRadius());

    ParticleSpawnContext context;
    context.position = player.GetPos() + upDirection * 0.06f - forwardDirection * radius * 0.30f;
    context.normal = upDirection;
    context.direction = emissionDirection;
    context.scale = 1.0f;

    particles.Emit(walkStartEffectId, context);
}
