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

struct AppliedPlayerMovement {
    glm::vec3 movementDelta{0.0f};
    glm::vec3 blockingNormal{0.0f};
    bool didHitStage = false;
};

AppliedPlayerMovement MoveWithCollision(
    Player& player,
    const glm::vec3& movementDelta,
    ActorCollisionFilter actorCollisionFilter =
        ActorCollisionFilter::AllActors)
{
    PhysicsSystem& physicsSystem = *player.GetGame()->GetPhysicsSystem();

    const glm::vec3 positionBeforeMovement = player.GetPos();
    const glm::vec3 desiredPosition = player.GetPos() + movementDelta;

    const ActorMovementCollisionResult collisionResult =
        physicsSystem.ResolveMovementCollision(
            &player,
            movementDelta,
            desiredPosition,
            actorCollisionFilter);

    player.SetPos(collisionResult.resolvedPosition);
    return {
        collisionResult.resolvedPosition - positionBeforeMovement,
        collisionResult.blockingNormal,
        collisionResult.didHitStage};
}

AppliedPlayerMovement MoveWithCollisionSubsteps(
    Player& player,
    const glm::vec3& movementDelta,
    float maximumSubstepDistance,
    ActorCollisionFilter actorCollisionFilter =
        ActorCollisionFilter::AllActors)
{
    const glm::vec3 positionBeforeMovement =
        player.GetPos();
    const float movementDistance = glm::length(movementDelta);
    if (movementDistance < 1e-6f) {
        return {};
    }

    const float safeMaximumSubstepDistance =
        std::max(maximumSubstepDistance, 0.001f);
    const int substepCount =
        std::max(
            1,
            static_cast<int>(
                std::ceil(
                    movementDistance /
                    safeMaximumSubstepDistance)));
    const glm::vec3 substepMovement =
        movementDelta /
        static_cast<float>(substepCount);

    AppliedPlayerMovement combinedMovement;
    for (int substepIndex = 0;
         substepIndex < substepCount;
         ++substepIndex) {
        const AppliedPlayerMovement appliedSubstep =
            MoveWithCollision(
                player,
                substepMovement,
                actorCollisionFilter);
        if (!appliedSubstep.didHitStage) {
            continue;
        }

        combinedMovement.blockingNormal =
            appliedSubstep.blockingNormal;
        combinedMovement.didHitStage = true;
    }

    combinedMovement.movementDelta =
        player.GetPos() -
        positionBeforeMovement;
    return combinedMovement;
}

void MoveAirborneVelocityWithCollision(
    Player& player,
    const glm::vec3& upDirection,
    float deltaTime)
{
    const glm::vec3 requestedMovement =
        player.GetVelocity() * deltaTime;
    constexpr float maximumAirborneCollisionSubstepDistance = 0.05f;
    const AppliedPlayerMovement appliedMovement =
        MoveWithCollisionSubsteps(
            player,
            requestedMovement,
            maximumAirborneCollisionSubstepDistance);

    const float requestedUpwardDistance =
        glm::dot(requestedMovement, upDirection);
    const float appliedUpwardDistance =
        glm::dot(appliedMovement.movementDelta, upDirection);
    constexpr float verticalMovementEpsilon = 0.0001f;

    glm::vec3 normalizedBlockingNormal;
    if (!appliedMovement.didHitStage ||
        !TryNormalizeDirection(
            appliedMovement.blockingNormal,
            normalizedBlockingNormal)) {
        return;
    }

    if (requestedUpwardDistance < -verticalMovementEpsilon) {
        constexpr float floorNormalMinimumUpDot = 0.65f;
        const bool didHitFloor =
            glm::dot(
                normalizedBlockingNormal,
                upDirection) >=
            floorNormalMinimumUpDot;
        const bool wasDownwardMovementBlocked =
            appliedUpwardDistance -
                verticalMovementEpsilon >
            requestedUpwardDistance;
        if (didHitFloor &&
            wasDownwardMovementBlocked) {
            player.Land(player.GetPos());
            player.SetShouldJudgeLanding(true);
        }
        return;
    }

    if (requestedUpwardDistance <= verticalMovementEpsilon) {
        return;
    }

    const bool wasUpwardMovementBlocked =
        appliedUpwardDistance +
            verticalMovementEpsilon <
        requestedUpwardDistance;
    if (!wasUpwardMovementBlocked) {
        return;
    }

    constexpr float ceilingNormalMaximumUpDot = -0.35f;
    const bool didHitCeiling =
        glm::dot(normalizedBlockingNormal, upDirection) <=
        ceilingNormalMaximumUpDot;
    if (!didHitCeiling) {
        return;
    }

    glm::vec3 velocity = player.GetVelocity();
    const float upwardSpeed = glm::dot(velocity, upDirection);
    if (upwardSpeed <= 0.0f) {
        return;
    }

    velocity -= upDirection * upwardSpeed;
    player.SetVelocity(velocity);
}

bool MoveAirSlamDownwardUntilFloorCollision(
    Player& player,
    const glm::vec3& upDirection,
    float deltaTime)
{
    const glm::vec3 requestedMovement =
        player.GetVelocity() * deltaTime;
    const float movementDistance =
        glm::length(requestedMovement);
    if (movementDistance < 1e-6f) {
        return false;
    }

    constexpr float maximumSubstepDistance = 0.05f;
    const int substepCount =
        std::max(
            1,
            static_cast<int>(
                std::ceil(
                    movementDistance /
                    maximumSubstepDistance)));
    const glm::vec3 substepMovement =
        requestedMovement /
        static_cast<float>(substepCount);

    constexpr float verticalMovementEpsilon = 0.0001f;
    constexpr float floorNormalMinimumUpDot = 0.65f;

    for (int substepIndex = 0;
         substepIndex < substepCount;
         ++substepIndex) {
        const AppliedPlayerMovement appliedMovement =
            MoveWithCollision(
                player,
                substepMovement);
        if (!appliedMovement.didHitStage) {
            continue;
        }

        glm::vec3 normalizedBlockingNormal;
        if (!TryNormalizeDirection(
                appliedMovement.blockingNormal,
                normalizedBlockingNormal)) {
            continue;
        }

        const float requestedUpwardDistance =
            glm::dot(
                substepMovement,
                upDirection);
        const float appliedUpwardDistance =
            glm::dot(
                appliedMovement.movementDelta,
                upDirection);
        const bool didHitFloor =
            glm::dot(
                normalizedBlockingNormal,
                upDirection) >=
            floorNormalMinimumUpDot;
        const bool wasDownwardMovementBlocked =
            appliedUpwardDistance -
                verticalMovementEpsilon >
            requestedUpwardDistance;
        if (!didHitFloor ||
            !wasDownwardMovementBlocked) {
            continue;
        }

        const PhysicsSystem& physicsSystem =
            *player.GetGame()->GetPhysicsSystem();
        const float collisionBottomOffsetFromPlayerOrigin =
            physicsSystem.GetPlayerCollisionCenterHeight() -
            physicsSystem.GetPlayerCollisionHeight() * 0.5f;
        const float upwardLandingCorrection =
            std::max(
                0.0f,
                collisionBottomOffsetFromPlayerOrigin);
        const glm::vec3 landingPosition =
            player.GetPos() +
            upDirection * upwardLandingCorrection;

        // The collision ellipsoid is centered above the player's visual origin.
        // A downward sweep therefore stops after the visual origin has crossed the floor.
        player.Land(landingPosition);
        player.SetShouldJudgeLanding(true);
        return true;
    }

    return false;
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

    const ActorCollisionFilter actorCollisionFilter =
        combat.IsAirDodgeAttackActive()
            ? ActorCollisionFilter::IgnoreAirborneEnemies
            : ActorCollisionFilter::AllActors;

    // 接触中の薄い壁を1フレームで越えないよう、回避だけ短い区間ごとに衝突を解決する。
    constexpr float maximumDodgeCollisionSubstepDistance = 0.05f;
    (void)MoveWithCollisionSubsteps(
        player,
        movementDelta,
        maximumDodgeCollisionSubstepDistance,
        actorCollisionFilter);

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

    const glm::vec3 dodgeDirection =
        hasMovementInput ? facingDirection : -facingDirection;
    StartDodgeMovementInDirection(
        player,
        dodgeDirection);
}

bool PlayerMovement::StartDodgeMovementTowards(
    Player& player,
    const glm::vec3& targetPosition)
{
    glm::vec3 dodgeDirection;
    if (!TryNormalizeDirection(
            targetPosition - player.GetPos(),
            dodgeDirection)) {
        return false;
    }

    StartDodgeMovementInDirection(
        player,
        dodgeDirection);
    return true;
}

void PlayerMovement::StartDodgeMovementInDirection(
    Player& player,
    const glm::vec3& dodgeDirection)
{
    mDodgeDir = dodgeDirection;
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
    MoveAirborneVelocityWithCollision(
        player,
        upDirection,
        deltaTime);

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
    player.AddVelocity(
        -upDirection * gravityAcceleration * deltaTime);
    MoveAirborneVelocityWithCollision(
        player,
        upDirection,
        deltaTime);
}

void PlayerMovement::StartAirSlamMovement(Player& player)
{
    const glm::vec3 upDirection =
        GetNormalizedUpDirection(player);

    constexpr float minimumRiseDurationSeconds = 0.05f;
    const float riseHeight =
        std::max(0.0f, mAirSlamRiseHeight);
    const float riseDurationSeconds =
        std::max(
            minimumRiseDurationSeconds,
            mAirSlamRiseDurationSeconds);
    const float riseSpeed =
        riseHeight / riseDurationSeconds;

    player.SetVelocity(upDirection * riseSpeed);
    player.SetOnGround(false);
    player.SetShouldJudgeLanding(false);
    mAirSlamMovementPhase =
        AirSlamMovementPhase::Rising;
    mAirSlamPhaseRemainingSeconds =
        riseDurationSeconds;
}

bool PlayerMovement::UpdateAirSlamMovement(
    Player& player,
    const PlayerCombat& combat,
    float deltaTime)
{
    if (player.GetOnGround()) {
        return true;
    }

    const glm::vec3 upDirection =
        GetNormalizedUpDirection(player);
    const float fallSpeed =
        std::max(
            1.0f,
            combat.GetStrongAttackSpeed());

    if (mAirSlamMovementPhase ==
        AirSlamMovementPhase::Rising) {
        MoveAirborneVelocityWithCollision(
            player,
            upDirection,
            deltaTime);
        mAirSlamPhaseRemainingSeconds =
            std::max(
                0.0f,
                mAirSlamPhaseRemainingSeconds -
                    deltaTime);

        const float upwardSpeed =
            glm::dot(
                player.GetVelocity(),
                upDirection);
        if (mAirSlamPhaseRemainingSeconds > 0.0f &&
            upwardSpeed > 0.0f) {
            return false;
        }

        player.SetVelocity(glm::vec3(0.0f));
        mAirSlamMovementPhase =
            AirSlamMovementPhase::Hovering;
        mAirSlamPhaseRemainingSeconds =
            std::max(
                0.0f,
                mAirSlamHoverDurationSeconds);
        return false;
    }

    if (mAirSlamMovementPhase ==
        AirSlamMovementPhase::Hovering) {
        player.SetVelocity(glm::vec3(0.0f));
        mAirSlamPhaseRemainingSeconds =
            std::max(
                0.0f,
                mAirSlamPhaseRemainingSeconds -
                    deltaTime);
        if (mAirSlamPhaseRemainingSeconds > 0.0f) {
            return false;
        }

        mAirSlamMovementPhase =
            AirSlamMovementPhase::Falling;
    }

    player.SetVelocity(
        -upDirection * fallSpeed);
    return MoveAirSlamDownwardUntilFloorCollision(
        player,
        upDirection,
        deltaTime);
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
