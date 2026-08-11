#include "actor/player/PlayerMovement.h"

#include "Game.h"
#include "actor/Planet.h"
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

glm::vec3 GetAirbornePhysicsUpDirection(const Player& player)
{
    const Planet* currentPlanet = player.GetCurrentPlanet();
    const bool usesEllipseGravity =
        !player.GetOnGround() &&
        currentPlanet &&
        currentPlanet->GetPlanetShape() ==
            Planet::PlanetShape::Ellipse;
    if (!usesEllipseGravity) {
        return GetNormalizedUpDirection(player);
    }

    return currentPlanet->CalculateEllipseSurfaceProjection(
        player.GetPos()).outwardNormal;
}

float CalculateEllipseAirControlMultiplier(
    const glm::vec3& visualUpDirection,
    const glm::vec3& physicsUpDirection)
{
    const float normalAlignment = glm::clamp(
        glm::dot(visualUpDirection, physicsUpDirection),
        0.0f,
        1.0f);
    constexpr float minimumAirControlMultiplier = 0.25f;
    return
        minimumAirControlMultiplier +
        (1.0f - minimumAirControlMultiplier) *
            normalAlignment;
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

    // A corner can still overlap after the bounded depenetration pass.
    // Keeping the previous valid position prevents that partial correction from entering the mesh.
    const glm::vec3 resolvedPosition =
        collisionResult.hasUnresolvedStageOverlap
            ? positionBeforeMovement
            : collisionResult.resolvedPosition;
    player.SetPos(resolvedPosition);
    return {
        resolvedPosition - positionBeforeMovement,
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

bool TryStepUpFromGround(
    Player& player,
    const glm::vec3& positionBeforeMovement,
    const glm::vec3& normalResolvedPosition,
    const glm::vec3& movementDelta,
    float maximumStepHeight)
{
    constexpr float movementEpsilon = 0.0001f;
    if (!player.GetOnGround() ||
        maximumStepHeight <= movementEpsilon) {
        return false;
    }

    const glm::vec3 upDirection =
        GetNormalizedUpDirection(player);
    const glm::vec3 horizontalMovement =
        ProjectOntoPlane(movementDelta, upDirection);
    glm::vec3 movementDirection;
    if (!TryNormalizeDirection(
            horizontalMovement,
            movementDirection)) {
        return false;
    }

    player.SetPos(positionBeforeMovement);
    const AppliedPlayerMovement upwardMovement =
        MoveWithCollision(
            player,
            upDirection * maximumStepHeight);
    const float appliedUpwardDistance =
        glm::dot(
            upwardMovement.movementDelta,
            upDirection);
    const bool hasEnoughHeadClearance =
        appliedUpwardDistance + movementEpsilon >=
        maximumStepHeight;
    if (!hasEnoughHeadClearance) {
        player.SetPos(normalResolvedPosition);
        return false;
    }

    (void)MoveWithCollision(
        player,
        horizontalMovement);
    const float normalForwardDistance =
        glm::dot(
            normalResolvedPosition - positionBeforeMovement,
            movementDirection);
    const float steppedForwardDistance =
        glm::dot(
            player.GetPos() - positionBeforeMovement,
            movementDirection);
    const bool movedFartherThanNormalResolution =
        steppedForwardDistance >
        normalForwardDistance + movementEpsilon;
    if (!movedFartherThanNormalResolution) {
        player.SetPos(normalResolvedPosition);
        return false;
    }

    constexpr float stepLandingProbeExtraDistance = 0.05f;
    const AppliedPlayerMovement downwardMovement =
        MoveWithCollision(
            player,
            -upDirection *
                (maximumStepHeight +
                 stepLandingProbeExtraDistance));
    glm::vec3 landingNormal;
    const bool hasLandingNormal =
        TryNormalizeDirection(
            downwardMovement.blockingNormal,
            landingNormal);
    const bool landedOnWalkableSurface =
        downwardMovement.didHitStage &&
        hasLandingNormal &&
        player.IsWalkableGroundNormal(
            landingNormal,
            upDirection);

    const float steppedHeight =
        glm::dot(
            player.GetPos() - positionBeforeMovement,
            upDirection);
    const bool landedAboveStartingSurface =
        steppedHeight > movementEpsilon;
    const bool stayedWithinMaximumStepHeight =
        steppedHeight <=
        maximumStepHeight + movementEpsilon;
    if (!landedOnWalkableSurface ||
        !landedAboveStartingSurface ||
        !stayedWithinMaximumStepHeight) {
        player.SetPos(normalResolvedPosition);
        return false;
    }

    glm::vec3 velocity = player.GetVelocity();
    velocity -=
        upDirection *
        glm::dot(velocity, upDirection);
    player.SetVelocity(velocity);
    player.SetOnGround(true);
    return true;
}

void MoveAirborneWithCollision(
    Player& player,
    const glm::vec3& upDirection,
    const glm::vec3& requestedMovement)
{
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
        const bool didHitFloor =
            player.IsWalkableGroundNormal(
                normalizedBlockingNormal,
                upDirection);
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
            player.IsWalkableGroundNormal(
                normalizedBlockingNormal,
                upDirection);
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
    const glm::vec3 movementDelta =
        CalculateInputMovementDelta(
            player,
            input,
            deltaTime);

    const glm::vec3 positionBeforeMovement =
        player.GetPos();
    const AppliedPlayerMovement normalMovement =
        MoveWithCollision(
            player,
            movementDelta);
    if (!normalMovement.didHitStage ||
        !player.GetOnGround()) {
        return;
    }

    glm::vec3 blockingNormal;
    if (!TryNormalizeDirection(
            normalMovement.blockingNormal,
            blockingNormal) ||
        player.IsWalkableGroundNormal(
            blockingNormal,
            GetNormalizedUpDirection(player))) {
        return;
    }

    const glm::vec3 normalResolvedPosition =
        player.GetPos();
    (void)TryStepUpFromGround(
        player,
        positionBeforeMovement,
        normalResolvedPosition,
        movementDelta,
        mMaximumStepHeight);
}

glm::vec3 PlayerMovement::CalculateInputMovementDelta(
    const Player& player,
    const PlayerInput& input,
    float deltaTime) const
{
    const glm::vec3 forwardMovement =
        mForwardVec * input.GetMoveForward();
    const glm::vec3 leftMovement =
        mLeftVec * input.GetMoveLeft();

    constexpr float fallbackAirControlMultiplier = 0.3f;
    const float inputMovementMultiplier =
        player.WasPlanetGravityFallbackAppliedThisJump()
            ? fallbackAirControlMultiplier
            : 1.0f;
    return
        (forwardMovement + leftMovement) *
        mMoveSpeed *
        inputMovementMultiplier *
        deltaTime;
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

    glm::vec3 movementDelta =
        mDodgeDir * dodgeSpeed * deltaTime;
    if (!player.GetOnGround() &&
        mDodgeTrajectory ==
            DodgeTrajectory::FollowEllipseSurface) {
        Planet* currentPlanet = player.GetCurrentPlanet();
        if (currentPlanet &&
            currentPlanet->GetPlanetShape() ==
                Planet::PlanetShape::Ellipse) {
            movementDelta =
                CalculateEllipseDodgeMovementDelta(
                    player,
                    *currentPlanet,
                    dodgeSpeed,
                    deltaTime);
        }
    }

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
        dodgeDirection,
        !player.GetOnGround() &&
                player.GetCurrentPlanet() &&
                player.GetCurrentPlanet()->GetPlanetShape() ==
                    Planet::PlanetShape::Ellipse
            ? DodgeTrajectory::FollowEllipseSurface
            : DodgeTrajectory::Straight);
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
        dodgeDirection,
        DodgeTrajectory::Straight);
    return true;
}

void PlayerMovement::StartDodgeMovementInDirection(
    Player& player,
    const glm::vec3& dodgeDirection,
    DodgeTrajectory trajectory)
{
    mDodgeDir = dodgeDirection;
    mDodgeTrajectory = trajectory;
    mEllipseDodgeNormalSpeed = 0.0f;
    mEllipseDodgeReturnStartSurfaceDistance = 0.0f;
    mEllipseDodgeSurfaceAttractionSpeed = 0.0f;
    if (trajectory == DodgeTrajectory::FollowEllipseSurface) {
        const Planet* currentPlanet = player.GetCurrentPlanet();
        if (currentPlanet) {
            const Planet::EllipseSurfaceProjection projection =
                currentPlanet->CalculateEllipseSurfaceProjection(
                    player.GetPos());
            const glm::vec3 dodgeUpDirection =
                projection.outwardNormal;
            mEllipseDodgeNormalSpeed =
                glm::dot(
                    player.GetVelocity(),
                    dodgeUpDirection);

            constexpr float minimumReturnStartSurfaceDistance = 1.25f;
            constexpr float allowedSurfaceDistanceIncrease = 0.35f;
            mEllipseDodgeReturnStartSurfaceDistance =
                std::max(
                    minimumReturnStartSurfaceDistance,
                    projection.distance +
                        allowedSurfaceDistanceIncrease);

            player.SetVelocity(
                dodgeUpDirection *
                mEllipseDodgeNormalSpeed);
        }
    } else {
        player.SetVelocity(glm::vec3(0.0f));
    }
    mDodgeTimer = CalculateDodgeMovementDuration(player.GetOnGround(), mDodgeDuration);
    mDodgeCooldownRemaining = mDodgeCooldownDuration;
    mHasUsedDodge = true;
}

glm::vec3 PlayerMovement::CalculateEllipseDodgeMovementDelta(
    Player& player,
    const Planet& planet,
    float dodgeSpeed,
    float deltaTime)
{
    const Planet::EllipseSurfaceProjection projection =
        planet.CalculateEllipseSurfaceProjection(player.GetPos());
    const glm::vec3 visualUpDirection =
        GetNormalizedUpDirection(player);
    const glm::vec3 physicsUpDirection =
        projection.outwardNormal;

    const glm::vec3 surfaceTangentDirection =
        ProjectOntoPlane(
            mDodgeDir,
            physicsUpDirection);
    glm::vec3 normalizedSurfaceTangentDirection;
    if (TryNormalizeDirection(
            surfaceTangentDirection,
            normalizedSurfaceTangentDirection)) {
        mDodgeDir = normalizedSurfaceTangentDirection;
    }

    constexpr float dodgeGravityMultiplier = 0.35f;
    const float gravityAcceleration =
        CalculateAirborneGravityAcceleration(
            mEllipseDodgeNormalSpeed) *
        dodgeGravityMultiplier;
    mEllipseDodgeNormalSpeed -=
        gravityAcceleration *
        std::max(0.0f, deltaTime);

    if (projection.isOutside) {
        constexpr float attractionAccelerationPerDistance = 10.0f;
        constexpr float normalTransitionAcceleration = 32.0f;
        constexpr float maximumAttractionAcceleration = 50.0f;
        constexpr float maximumAttractionSpeed = 14.0f;

        const float distanceAttractionAcceleration =
            std::max(
                0.0f,
                projection.distance -
                    mEllipseDodgeReturnStartSurfaceDistance) *
            attractionAccelerationPerDistance;
        const float normalAlignment = glm::clamp(
            glm::dot(
                visualUpDirection,
                projection.outwardNormal),
            0.0f,
            1.0f);
        const float transitionAttractionAcceleration =
            (1.0f - normalAlignment) *
            normalTransitionAcceleration;
        const float attractionAcceleration = glm::clamp(
            distanceAttractionAcceleration +
                transitionAttractionAcceleration,
            0.0f,
            maximumAttractionAcceleration);
        mEllipseDodgeSurfaceAttractionSpeed =
            std::min(
                maximumAttractionSpeed,
                mEllipseDodgeSurfaceAttractionSpeed +
                    attractionAcceleration *
                    std::max(0.0f, deltaTime));
    }

    const glm::vec3 normalVelocity =
        physicsUpDirection *
        mEllipseDodgeNormalSpeed;
    const glm::vec3 surfaceAttractionVelocity =
        -projection.outwardNormal *
        mEllipseDodgeSurfaceAttractionSpeed;
    const glm::vec3 airborneVelocity =
        normalVelocity +
        surfaceAttractionVelocity;
    player.SetVelocity(airborneVelocity);

    const float surfaceMovementMultiplier =
        CalculateEllipseAirControlMultiplier(
            visualUpDirection,
            physicsUpDirection);
    const glm::vec3 requestedMovement =
        (mDodgeDir *
             dodgeSpeed *
             surfaceMovementMultiplier +
         airborneVelocity) *
        deltaTime;
    bool wasMovementClamped = false;
    return ClampEllipseAirborneMovementToSurfaceTravelLimit(
        planet,
        player.GetPos(),
        physicsUpDirection,
        requestedMovement,
        wasMovementClamped);
}

void PlayerMovement::StartJumpMovement(Player& player, float deltaTime)
{
    RecordEllipseAirborneStartSurfaceNormal(player);

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
    MoveAirborneWithCollision(
        player,
        upDirection,
        player.GetVelocity() * deltaTime);

    player.SetOnGround(false);
    player.SetShouldJudgeLanding(false);
}

void PlayerMovement::ApplyJumpGravity(Player& player, float deltaTime) const
{
    ApplyJumpGravityMovement(
        player,
        glm::vec3(0.0f),
        deltaTime);
}

void PlayerMovement::ApplyJumpGravityAndInputMovement(
    Player& player,
    const PlayerInput& input,
    float deltaTime) const
{
    ApplyJumpGravityMovement(
        player,
        CalculateInputMovementDelta(
            player,
            input,
            deltaTime),
        deltaTime);
}

void PlayerMovement::ApplyJumpGravityMovement(
    Player& player,
    const glm::vec3& inputMovementDelta,
    float deltaTime) const
{
    if (player.GetOnGround()) {
        player.ApplyGravityToSelf(deltaTime);
        return;
    }

    const glm::vec3 visualUpDirection =
        GetNormalizedUpDirection(player);
    const glm::vec3 physicsUpDirection =
        GetAirbornePhysicsUpDirection(player);
    Planet* currentPlanet = player.GetCurrentPlanet();
    const bool usesEllipseGravity =
        currentPlanet &&
        currentPlanet->GetPlanetShape() ==
            Planet::PlanetShape::Ellipse;

    if (usesEllipseGravity) {
        glm::vec3 velocity = player.GetVelocity();
        const float physicsNormalSpeed =
            glm::dot(velocity, physicsUpDirection);
        const glm::vec3 tangentialVelocity =
            velocity -
            physicsUpDirection * physicsNormalSpeed;
        const float tangentialVelocityMultiplier =
            CalculateEllipseAirControlMultiplier(
                visualUpDirection,
                physicsUpDirection);
        velocity =
            physicsUpDirection * physicsNormalSpeed +
            tangentialVelocity * tangentialVelocityMultiplier;
        player.SetVelocity(velocity);
    }

    const float verticalSpeed =
        glm::dot(
            player.GetVelocity(),
            physicsUpDirection);
    const float gravityAcceleration =
        CalculateAirborneGravityAcceleration(verticalSpeed);
    player.AddVelocity(
        -physicsUpDirection *
        gravityAcceleration *
        deltaTime);

    glm::vec3 adjustedInputMovementDelta =
        inputMovementDelta;
    if (usesEllipseGravity) {
        adjustedInputMovementDelta *=
            CalculateEllipseAirControlMultiplier(
                visualUpDirection,
                physicsUpDirection);
    }

    glm::vec3 requestedMovement =
        player.GetVelocity() * deltaTime +
        adjustedInputMovementDelta;
    if (usesEllipseGravity) {
        bool wasMovementClamped = false;
        requestedMovement =
            ClampEllipseAirborneMovementToSurfaceTravelLimit(
                *currentPlanet,
                player.GetPos(),
                physicsUpDirection,
                requestedMovement,
                wasMovementClamped);
        if (wasMovementClamped) {
            const float physicsNormalSpeedAfterGravity =
                glm::dot(
                    player.GetVelocity(),
                    physicsUpDirection);
            player.SetVelocity(
                physicsUpDirection *
                physicsNormalSpeedAfterGravity);
        }
    }

    MoveAirborneWithCollision(
        player,
        physicsUpDirection,
        requestedMovement);
}

void PlayerMovement::RecordEllipseAirborneStartSurfaceNormal(
    const Player& player)
{
    const Planet* currentPlanet = player.GetCurrentPlanet();
    mHasEllipseAirborneStartSurfaceNormal =
        currentPlanet &&
        currentPlanet->GetPlanetShape() ==
            Planet::PlanetShape::Ellipse;
    if (!mHasEllipseAirborneStartSurfaceNormal) {
        return;
    }

    mEllipseAirborneStartSurfaceNormal =
        currentPlanet->CalculateEllipseSurfaceProjection(
            player.GetPos()).outwardNormal;
}

void PlayerMovement::ResetEllipseAirborneSurfaceTravel()
{
    mHasEllipseAirborneStartSurfaceNormal = false;
}

glm::vec3 PlayerMovement::
ClampEllipseAirborneMovementToSurfaceTravelLimit(
    const Planet& planet,
    const glm::vec3& currentPosition,
    const glm::vec3& physicsUpDirection,
    const glm::vec3& requestedMovement,
    bool& wasMovementClamped) const
{
    wasMovementClamped = false;
    if (!mHasEllipseAirborneStartSurfaceNormal) {
        return requestedMovement;
    }

    constexpr float maximumSurfaceTravelDegrees = 100.0f;
    const float minimumAllowedNormalDot =
        std::cos(glm::radians(maximumSurfaceTravelDegrees));
    const float currentNormalDot = glm::dot(
        mEllipseAirborneStartSurfaceNormal,
        planet.CalculateEllipseSurfaceProjection(
            currentPosition).outwardNormal);
    const float requiredNormalDot =
        std::min(
            currentNormalDot,
            minimumAllowedNormalDot);

    const glm::vec3 requestedPosition =
        currentPosition + requestedMovement;
    const float requestedNormalDot = glm::dot(
        mEllipseAirborneStartSurfaceNormal,
        planet.CalculateEllipseSurfaceProjection(
            requestedPosition).outwardNormal);
    if (requestedNormalDot >= requiredNormalDot) {
        return requestedMovement;
    }

    const glm::vec3 normalMovement =
        physicsUpDirection *
        glm::dot(requestedMovement, physicsUpDirection);
    const glm::vec3 tangentialMovement =
        requestedMovement - normalMovement;
    if (glm::length(tangentialMovement) <= 0.000001f) {
        return requestedMovement;
    }

    float allowedTangentialMultiplier = 0.0f;
    float blockedTangentialMultiplier = 1.0f;
    constexpr int movementLimitSearchIterations = 8;
    for (int searchIndex = 0;
         searchIndex < movementLimitSearchIterations;
         ++searchIndex) {
        const float candidateTangentialMultiplier =
            (allowedTangentialMultiplier +
             blockedTangentialMultiplier) *
            0.5f;
        const glm::vec3 candidatePosition =
            currentPosition +
            normalMovement +
            tangentialMovement *
                candidateTangentialMultiplier;
        const float candidateNormalDot = glm::dot(
            mEllipseAirborneStartSurfaceNormal,
            planet.CalculateEllipseSurfaceProjection(
                candidatePosition).outwardNormal);
        if (candidateNormalDot >= requiredNormalDot) {
            allowedTangentialMultiplier =
                candidateTangentialMultiplier;
        } else {
            blockedTangentialMultiplier =
                candidateTangentialMultiplier;
        }
    }

    wasMovementClamped = true;
    return
        normalMovement +
        tangentialMovement *
            allowedTangentialMultiplier;
}

float PlayerMovement::CalculateAirborneGravityAcceleration(
    const Player& player) const
{
    const glm::vec3 upDirection =
        GetNormalizedUpDirection(player);
    const float verticalSpeed =
        glm::dot(player.GetVelocity(), upDirection);
    return CalculateAirborneGravityAcceleration(verticalSpeed);
}

float PlayerMovement::CalculateAirborneGravityAcceleration(
    float verticalSpeed) const
{
    const float gravityDuration =
        verticalSpeed > 0.0f
            ? std::max(mJumpAscentDuration, 0.05f)
            : std::max(mJumpFallDuration, 0.05f);
    return
        (2.0f * std::max(mJumpHeight, 0.0f)) /
        (gravityDuration * gravityDuration);
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
        MoveAirborneWithCollision(
            player,
            upDirection,
            player.GetVelocity() * deltaTime);
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
