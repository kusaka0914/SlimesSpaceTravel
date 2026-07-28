#include "actor/player/PlayerMovement.h"

#include "Game.h"
#include "actor/Player.h"
#include "actor/player/PlayerCombat.h"
#include "actor/player/PlayerGrounding.h"
#include "actor/player/PlayerInput.h"
#include "system/PhysicsSystem.h"
#include "utils/MathUtils.h"

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace {

bool TryNormalizeDirection(const glm::vec3& direction, glm::vec3& normalizedDirection)
{
    constexpr float directionEpsilon = 1e-6f;

    const float directionLength = glm::length(direction);

    if (directionLength < directionEpsilon) {
        return false;
    }

    normalizedDirection = direction / directionLength;
    return true;
}

glm::vec3 GetNormalizedUpDirection(const Player& player)
{
    glm::vec3 normalizedUpDirection;

    if (TryNormalizeDirection(player.GetUpVec(), normalizedUpDirection)) {
        return normalizedUpDirection;
    }

    const glm::vec3 fallbackUpDirection(0.0f, 1.0f, 0.0f);
    return fallbackUpDirection;
}

glm::vec3 ProjectOntoPlane(const glm::vec3& direction, const glm::vec3& planeNormal)
{
    return direction - planeNormal * glm::dot(direction, planeNormal);
}

glm::vec3 CreatePerpendicularDirection(const glm::vec3& upDirection)
{
    constexpr float axisParallelThreshold = 0.9f;

    const bool isNearlyParallelToYAxis = std::abs(upDirection.y) > axisParallelThreshold;

    const glm::vec3 referenceAxis = isNearlyParallelToYAxis ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);

    glm::vec3 perpendicularDirection;

    if (TryNormalizeDirection(glm::cross(referenceAxis, upDirection), perpendicularDirection)) {
        return perpendicularDirection;
    }

    const glm::vec3 fallbackForwardDirection(0.0f, 0.0f, 1.0f);
    return fallbackForwardDirection;
}

float CalculateDodgeMovementDuration(bool isOnGround, float baseDodgeDuration)
{
    if (isOnGround) {
        return baseDodgeDuration;
    }

    constexpr float airborneDurationMultiplier = 4.0f;
    return baseDodgeDuration * airborneDurationMultiplier;
}

void MoveWithCollision(Player& player, const glm::vec3& movementDelta)
{
    PhysicsSystem& physicsSystem = *player.GetGame()->GetPhysicsSystem();

    const glm::vec3 desiredPosition = player.GetPos() + movementDelta;

    const glm::vec3 collisionResolvedPosition = physicsSystem.CheckCollision(&player, movementDelta, desiredPosition);

    player.SetPos(collisionResolvedPosition);
}

float CalculateFacingYaw(Player& player, const glm::vec3& upDirection, const glm::vec3& facingDirection)
{
    MathUtils& mathUtils = *player.GetGame()->GetMathUtils();

    const float directionYaw = mathUtils.GetYawFromDirection(upDirection, facingDirection);

    return directionYaw + glm::pi<float>();
}

void ApplyFacingDirection(Player& player, const glm::vec3& upDirection, const glm::vec3& facingDirection)
{
    player.SetFacingForwardVec(facingDirection);
    player.SetFacingYaw(CalculateFacingYaw(player, upDirection, facingDirection));
}

} // namespace

bool PlayerMovement::CanDodge(const PlayerCombat& combat) const
{
    const bool isCooldownFinished = mDodgeCooldownRemaining <= 0.0f;
    const bool hasDodgeAvailable = !mHasUsedDodge;

    return isCooldownFinished && hasDodgeAvailable;
}

void PlayerMovement::UpdateCameraRelativeMovementDirections(Player& player, const PlayerInput& input)
{
    const glm::vec3 upDirection = GetNormalizedUpDirection(player);

    const glm::vec3 projectedForwardDirection = ProjectOntoPlane(mForwardVec, upDirection);

    glm::vec3 baseForwardDirection;

    if (!TryNormalizeDirection(projectedForwardDirection, baseForwardDirection)) {
        baseForwardDirection = CreatePerpendicularDirection(upDirection);
    }

    glm::vec3 baseLeftDirection;

    if (!TryNormalizeDirection(glm::cross(upDirection, baseForwardDirection), baseLeftDirection)) {
        return;
    }

    const float cameraYawRadians = input.GetCameraYaw();

    const glm::vec3 cameraRelativeForwardDirection =
        baseForwardDirection * std::cos(cameraYawRadians) - baseLeftDirection * std::sin(cameraYawRadians);

    glm::vec3 movementForwardDirection;

    if (!TryNormalizeDirection(cameraRelativeForwardDirection, movementForwardDirection)) {
        return;
    }

    glm::vec3 movementLeftDirection;

    if (!TryNormalizeDirection(glm::cross(upDirection, movementForwardDirection), movementLeftDirection)) {
        return;
    }

    mForwardVec = movementForwardDirection;
    mLeftVec = movementLeftDirection;
}

void PlayerMovement::SetCameraForwardDirection(const glm::vec3& forwardDirection, const glm::vec3& upDirection)
{
    glm::vec3 normalizedUpDirection;
    if (!TryNormalizeDirection(upDirection, normalizedUpDirection)) {
        return;
    }

    glm::vec3 normalizedForwardDirection;
    if (!TryNormalizeDirection(ProjectOntoPlane(forwardDirection, normalizedUpDirection),
                               normalizedForwardDirection)) {
        return;
    }

    glm::vec3 normalizedLeftDirection;
    if (!TryNormalizeDirection(glm::cross(normalizedUpDirection, normalizedForwardDirection),
                               normalizedLeftDirection)) {
        return;
    }

    mForwardVec = normalizedForwardDirection;
    mLeftVec = normalizedLeftDirection;
}

void PlayerMovement::UpdateFacingDirectionFromInput(Player& player, const PlayerInput& input)
{
    const glm::vec3 requestedFacingDirection = mForwardVec * input.GetMoveForward() + mLeftVec * input.GetMoveLeft();

    glm::vec3 facingDirection;

    if (!TryNormalizeDirection(requestedFacingDirection, facingDirection)) {
        return;
    }

    FaceDirection(player, facingDirection);
}

void PlayerMovement::FaceDirection(Player& player, const glm::vec3& facingDirection)
{
    glm::vec3 normalizedFacingDirection;
    if (!TryNormalizeDirection(facingDirection, normalizedFacingDirection)) {
        return;
    }

    const glm::vec3 upDirection = GetNormalizedUpDirection(player);
    ApplyFacingDirection(player, upDirection, normalizedFacingDirection);
}

void PlayerMovement::UpdateDodgeCooldown(float deltaTime)
{
    mDodgeCooldownRemaining = std::max(0.0f, mDodgeCooldownRemaining - deltaTime);
}

void PlayerMovement::MoveFromInput(Player& player, const PlayerInput& input, float deltaTime)
{
    const glm::vec3 forwardMovement = mForwardVec * input.GetMoveForward();
    const glm::vec3 leftMovement = mLeftVec * input.GetMoveLeft();

    constexpr float fallbackAirControlMultiplier = 0.3f;
    const float inputMovementMultiplier =
        player.WasPlanetGravityFallbackAppliedThisJump() ? fallbackAirControlMultiplier : 1.0f;
    const glm::vec3 movementDelta =
        (forwardMovement + leftMovement) * mMoveSpeed * inputMovementMultiplier * deltaTime;

    MoveWithCollision(player, movementDelta);
}

void PlayerMovement::ApplyDodgeMovement(Player& player, const PlayerCombat& combat, PlayerGrounding& grounding,
                                         float deltaTime)
{
    const float dodgeMovementDuration = CalculateDodgeMovementDuration(player.GetOnGround(), mDodgeDuration);

    if (dodgeMovementDuration <= 0.0f) {
        return;
    }

    float dodgeSpeed = mDodgeDistance / dodgeMovementDuration;

    const bool shouldReduceDodgeSpeed = combat.IsSpecialCharging() || combat.GetCanSpecialAttack();

    if (shouldReduceDodgeSpeed) {
        constexpr float reducedSpeedMultiplier = 0.25f;
        dodgeSpeed *= reducedSpeedMultiplier;
    }

    const glm::vec3 movementDelta = mDodgeDir * dodgeSpeed * deltaTime;

    MoveWithCollision(player, movementDelta);

    if (!player.GetOnGround()) {
        return;
    }

    constexpr float snapUpOffset = 0.5f;
    constexpr float snapDownLength = 1.0f;

    grounding.SnapToGround(player, snapUpOffset, snapDownLength);
}

void PlayerMovement::ApplyAttackMovement(Player& player, const PlayerCombat& combat, float deltaTime)
{
    const glm::vec3 movementDelta = player.GetFacingForwardVec() * combat.GetAttackSpeed() * deltaTime;

    MoveWithCollision(player, movementDelta);
}

void PlayerMovement::ApplyChargeMovement(Player& player, float deltaTime)
{
    const glm::vec3 movementDelta = -player.GetFacingForwardVec() * mChargeMoveSpeed * deltaTime;

    MoveWithCollision(player, movementDelta);
}

void PlayerMovement::ApplyStrongAttackMovement(Player& player, const PlayerCombat& combat, float deltaTime)
{
    const glm::vec3 attackDirection =
        mHasStrongAttackDirectionOverride ? mStrongAttackDirectionOverride : player.GetFacingForwardVec();
    const glm::vec3 movementDelta = attackDirection * combat.GetStrongAttackSpeed() * deltaTime;

    MoveWithCollision(player, movementDelta);
}

void PlayerMovement::ApplyKnockBackMovement(Player& player, float deltaTime)
{
    const glm::vec3 awayFromKnockBackOrigin = player.GetPos() - mKnockBackFrom;

    glm::vec3 knockBackDirection;

    if (!TryNormalizeDirection(awayFromKnockBackOrigin, knockBackDirection)) {
        return;
    }

    const glm::vec3 movementDelta = knockBackDirection * mKnockBackSpeed * deltaTime;

    MoveWithCollision(player, movementDelta);
}

void PlayerMovement::StartDodgeMovement(Player& player, const PlayerInput& input)
{
    const bool hasMovementInput = input.GetMoveForward() != 0.0f || input.GetMoveLeft() != 0.0f;

    glm::vec3 facingDirection;

    if (!TryNormalizeDirection(player.GetFacingForwardVec(), facingDirection) &&
        !TryNormalizeDirection(mForwardVec, facingDirection)) {
        facingDirection = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    mDodgeDir = hasMovementInput ? facingDirection : -facingDirection;

    mDodgeTimer = CalculateDodgeMovementDuration(player.GetOnGround(), mDodgeDuration);

    mDodgeCooldownRemaining = mDodgeCooldownDuration;

    player.SetVelocity(glm::vec3(0.0f));

    mHasUsedDodge = true;
}

void PlayerMovement::StartJumpMovement(Player& player, float deltaTime)
{
    const glm::vec3 upDirection = GetNormalizedUpDirection(player);

    // 足場から落ち始めた直後でも、下向き速度を消して同じ高さのジャンプにする。
    glm::vec3 velocity = player.GetVelocity();
    const float verticalSpeed = glm::dot(velocity, upDirection);
    if (verticalSpeed < 0.0f) {
        velocity -= upDirection * verticalSpeed;
        player.SetVelocity(velocity);
    }

    const float safeAscentDuration = std::max(mJumpAscentDuration, 0.05f);
    const float jumpSpeed = (2.0f * std::max(mJumpHeight, 0.0f)) / safeAscentDuration;
    const glm::vec3 jumpVelocityDelta = upDirection * jumpSpeed;
    player.AddVelocity(jumpVelocityDelta);

    // ジャンプ開始後は状態更新から即時returnするため、このフレーム分の上昇移動をここで反映する。
    player.SetPos(player.GetPos() + player.GetVelocity() * deltaTime);

    player.SetOnGround(false);
    player.SetShouldJudgeLanding(false);
}

void PlayerMovement::ApplyJumpGravity(Player& player, float deltaTime) const
{
    if (player.GetOnGround()) {
        player.ApplyGravityToSelf(deltaTime);
        return;
    }

    const glm::vec3 upDirection = GetNormalizedUpDirection(player);
    const float verticalSpeed = glm::dot(player.GetVelocity(), upDirection);
    const float duration =
        verticalSpeed > 0.0f ? std::max(mJumpAscentDuration, 0.05f) : std::max(mJumpFallDuration, 0.05f);
    const float gravityAcceleration = (2.0f * std::max(mJumpHeight, 0.0f)) / (duration * duration);
    player.ApplyGravityToSelf(deltaTime, gravityAcceleration);
}

void PlayerMovement::StartStrongAttackMovementTowards(
    Player& player,
    const glm::vec3& targetPosition)
{
    UpdateStrongAttackDirectionTowards(player, targetPosition);

    player.SetVelocity(glm::vec3(0.0f));
    player.SetOnGround(false);
    player.SetShouldJudgeLanding(false);
}

void PlayerMovement::UpdateStrongAttackDirectionTowards(
    Player& player,
    const glm::vec3& targetPosition)
{
    glm::vec3 attackDirection;
    if (!TryNormalizeDirection(targetPosition - player.GetPos(), attackDirection)) {
        ClearStrongAttackDirectionOverride();
        return;
    }

    mStrongAttackDirectionOverride = attackDirection;
    mHasStrongAttackDirectionOverride = true;
}

void PlayerMovement::StartAssistStrongAttackMovement(
    Player& player,
    const glm::vec3& targetPosition)
{
    StartStrongAttackMovementTowards(player, targetPosition);
}

void PlayerMovement::ClearStrongAttackDirectionOverride()
{
    mHasStrongAttackDirectionOverride = false;
    mStrongAttackDirectionOverride = glm::vec3(0.0f);
}
