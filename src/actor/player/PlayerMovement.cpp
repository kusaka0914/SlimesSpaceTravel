#include "actor/player/PlayerMovement.h"

#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Player.h"
#include "actor/player/PlayerCombat.h"
#include "actor/player/PlayerGrounding.h"
#include "actor/player/PlayerInput.h"
#include "component/PlatformAdhesionComponent.h"
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
    return player.CalculateAirbornePhysicsUpDirection();
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

void TryAttachToAdhesionPlatformAlongMovement(
    Player& player,
    const glm::vec3& movementStart)
{
    PlatformAdhesionComponent::
        TryAttachPlayerToAnyPlatformAlongMovement(
            player,
            movementStart);
}

AppliedPlayerMovement MoveWithCollision(
    PhysicsSystem& physicsSystem,
    Player& player,
    const glm::vec3& movementDelta,
    ActorCollisionFilter actorCollisionFilter =
        ActorCollisionFilter::AllActors)
{
    const glm::vec3 positionBeforeMovement = player.GetPos();
    const glm::vec3 desiredPosition = player.GetPos() + movementDelta;

    const ActorMovementCollisionResult collisionResult =
        physicsSystem.ResolveMovementCollision(
            &player,
            movementDelta,
            desiredPosition,
            actorCollisionFilter);



    const glm::vec3 resolvedPosition =
        collisionResult.hasUnresolvedStageOverlap
            ? positionBeforeMovement
            : collisionResult.resolvedPosition;
    player.SetPos(resolvedPosition);
    TryAttachToAdhesionPlatformAlongMovement(
        player,
        positionBeforeMovement);

    const glm::vec3 finalPosition = player.GetPos();
    return {
        finalPosition - positionBeforeMovement,
        collisionResult.blockingNormal,
        collisionResult.didHitStage};
}

AppliedPlayerMovement MoveWithCollisionSubsteps(
    PhysicsSystem& physicsSystem,
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
                physicsSystem,
                player,
                substepMovement,
                actorCollisionFilter);
        if (player.IsAttachedToPlatform()) {
            combinedMovement.blockingNormal =
                appliedSubstep.blockingNormal;
            combinedMovement.didHitStage =
                appliedSubstep.didHitStage;
            break;
        }
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
    PhysicsSystem& physicsSystem,
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
            physicsSystem,
            player,
            upDirection * maximumStepHeight);
    if (player.IsAttachedToPlatform()) {
        return true;
    }
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
        physicsSystem,
        player,
        horizontalMovement);
    if (player.IsAttachedToPlatform()) {
        return true;
    }
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
            physicsSystem,
            player,
            -upDirection *
                (maximumStepHeight +
                 stepLandingProbeExtraDistance));
    if (player.IsAttachedToPlatform()) {
        return true;
    }
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
    PhysicsSystem& physicsSystem,
    Player& player,
    const glm::vec3& upDirection,
    const glm::vec3& requestedMovement)
{
    constexpr float maximumAirborneCollisionSubstepDistance = 0.05f;
    const AppliedPlayerMovement appliedMovement =
        MoveWithCollisionSubsteps(
            physicsSystem,
            player,
            requestedMovement,
            maximumAirborneCollisionSubstepDistance);
    if (player.IsAttachedToPlatform()) {
        return;
    }

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
    PhysicsSystem& physicsSystem,
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
                physicsSystem,
                player,
                substepMovement);
        if (player.IsAttachedToPlatform()) {
            return true;
        }
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



        player.Land(landingPosition);
        player.SetShouldJudgeLanding(true);
        return true;
    }

    return false;
}

float CalculateFacingYaw(
    MathUtils& mathUtils,
    const glm::vec3& upDirection,
    const glm::vec3& facingDirection)
{
    const float directionYaw = mathUtils.GetYawFromDirection(upDirection, facingDirection);

    return directionYaw + glm::pi<float>();
}

void ApplyFacingDirection(
    MathUtils& mathUtils,
    Player& player,
    const glm::vec3& upDirection,
    const glm::vec3& facingDirection)
{
    player.SetFacingForwardVec(facingDirection);
    player.SetFacingYaw(CalculateFacingYaw(
        mathUtils,
        upDirection,
        facingDirection));
}

}

PlayerMovement::PlayerMovement(
    PhysicsSystem& physicsSystem,
    MathUtils& mathUtils)
    : mPhysicsSystem(physicsSystem),
      mMathUtils(mathUtils)
{
}

bool PlayerMovement::CanDodge(const PlayerCombat& combat) const
{
    const bool isCooldownFinished = mDodgeCooldownRemaining <= 0.0f;
    const bool hasDodgeAvailable = !mHasUsedDodge;

    return isCooldownFinished && hasDodgeAvailable;
}

void PlayerMovement::UpdateCameraRelativeMovementDirections(Player& player, const PlayerInput& input)
{
    const glm::vec3 upDirection = GetNormalizedUpDirection(player);
    const glm::vec2 movementInput(
        input.GetMoveLeft(),
        input.GetMoveForward());
    const float movementInputMagnitude = glm::length(movementInput);
    constexpr float movementInputReleaseThreshold = 0.01f;
    if (mIsCameraAutoAlignMovementDirectionLocked) {
        if (movementInputMagnitude <= movementInputReleaseThreshold) {
            // 表裏遷移の回転中は、スティックが一瞬ニュートラルを
            // 通っても開始時の進行方向を保持する。解除はカメラの
            // 回転完了、または明確な入力方向変更時に行う。
        } else {
            const glm::vec2 currentInputDirection =
                movementInput / movementInputMagnitude;
            constexpr float movementDirectionUnlockAngleDegrees = 90.0f;
            const float movementDirectionUnlockDot = std::cos(
                glm::radians(movementDirectionUnlockAngleDegrees));
            const bool didMovementInputDirectionChange =
                glm::dot(
                    mCameraAutoAlignStartInputDirection,
                    currentInputDirection) <= movementDirectionUnlockDot;
            if (didMovementInputDirectionChange) {
                mIsCameraAutoAlignMovementDirectionLocked = false;
                mHasCameraAutoAlignCancellationRequest = true;
            } else {
                glm::vec3 transportedMovementDirection;
                if (TryNormalizeDirection(
                        ProjectOntoPlane(
                            mCameraAutoAlignMovementDirection,
                            upDirection),
                        transportedMovementDirection)) {
                    mCameraAutoAlignMovementDirection =
                        transportedMovementDirection;
                }
            }
        }
    }

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

void PlayerMovement::LockMovementDirectionForCameraAutoAlign(
    const PlayerInput& input,
    const glm::vec3& upDirection)
{
    glm::vec3 normalizedUpDirection;
    if (!TryNormalizeDirection(upDirection, normalizedUpDirection)) {
        return;
    }

    const glm::vec3 requestedMovementDirection =
        mForwardVec * input.GetMoveForward() +
        mLeftVec * input.GetMoveLeft();
    glm::vec3 movementDirection;
    if (!TryNormalizeDirection(
            ProjectOntoPlane(
                requestedMovementDirection,
                normalizedUpDirection),
            movementDirection)) {
        return;
    }

    mCameraAutoAlignMovementDirection = movementDirection;
    mCameraAutoAlignStartInputDirection = glm::normalize(
        glm::vec2(input.GetMoveLeft(), input.GetMoveForward()));
    mIsCameraAutoAlignMovementDirectionLocked = true;
}

bool PlayerMovement::ConsumeCameraAutoAlignCancellationRequest()
{
    const bool hasCancellationRequest =
        mHasCameraAutoAlignCancellationRequest;
    mHasCameraAutoAlignCancellationRequest = false;
    return hasCancellationRequest;
}

void PlayerMovement::UnlockMovementDirectionForCameraAutoAlign()
{
    mIsCameraAutoAlignMovementDirectionLocked = false;
}

void PlayerMovement::UpdateFacingDirectionFromInput(Player& player, const PlayerInput& input)
{
    const glm::vec3 requestedFacingDirection =
        mIsCameraAutoAlignMovementDirectionLocked
            ? mCameraAutoAlignMovementDirection
            : mForwardVec * input.GetMoveForward() +
                  mLeftVec * input.GetMoveLeft();

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
    ApplyFacingDirection(
        mMathUtils,
        player,
        upDirection,
        normalizedFacingDirection);
}

void PlayerMovement::UpdateDodgeCooldown(float deltaTime)
{
    mDodgeCooldownRemaining = std::max(0.0f, mDodgeCooldownRemaining - deltaTime);
}

void PlayerMovement::MoveFromInput(Player& player, const PlayerInput& input, float deltaTime)
{
    if (player.IsAttachedToPlatform()) {
        return;
    }

    const glm::vec3 movementDelta =
        CalculateInputMovementDelta(
            player,
            input,
            deltaTime);

    const glm::vec3 positionBeforeMovement =
        player.GetPos();
    const AppliedPlayerMovement normalMovement =
        MoveWithCollision(
            mPhysicsSystem,
            player,
            movementDelta);
    if (player.IsAttachedToPlatform()) {
        return;
    }
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
        mPhysicsSystem,
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
    glm::vec3 requestedMovementDirection;
    if (mIsCameraAutoAlignMovementDirectionLocked) {
        const float movementInputMagnitude = glm::clamp(
            glm::length(
                glm::vec2(
                    input.GetMoveLeft(),
                    input.GetMoveForward())),
            0.0f,
            1.0f);
        requestedMovementDirection =
            mCameraAutoAlignMovementDirection *
            movementInputMagnitude;
    } else {
        requestedMovementDirection =
            mForwardVec * input.GetMoveForward() +
            mLeftVec * input.GetMoveLeft();
    }

    constexpr float fallbackAirControlMultiplier = 0.3f;
    const float inputMovementMultiplier =
        player.WasPlanetGravityFallbackAppliedThisJump()
            ? fallbackAirControlMultiplier
            : 1.0f;
    return
        requestedMovementDirection *
        mMoveSpeed *
        inputMovementMultiplier *
        deltaTime;
}

void PlayerMovement::ApplyDodgeMovement(
    Player& player,
    const PlayerCombat& combat,
    PlayerGrounding& grounding,
    float deltaTime)
{
    if (player.IsAttachedToPlatform()) {
        return;
    }

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
        mPhysicsSystem,
        player,
        movementDelta,
        maximumDodgeCollisionSubstepDistance,
        actorCollisionFilter);
    if (player.IsAttachedToPlatform()) {
        return;
    }

    if (!player.GetOnGround()) {
        return;
    }

    constexpr float snapUpOffset = 0.5f;
    constexpr float snapDownLength = 1.0f;

    grounding.SnapToGround(player, snapUpOffset, snapDownLength);
}

void PlayerMovement::ApplyAttackMovement(Player& player, const PlayerCombat& combat, float deltaTime)
{
    if (player.IsAttachedToPlatform()) {
        return;
    }

    const glm::vec3 movementDelta = player.GetFacingForwardVec() * combat.GetAttackSpeed() * deltaTime;

    MoveWithCollision(mPhysicsSystem, player, movementDelta);
}

void PlayerMovement::ApplyStrongAttackMovement(Player& player, const PlayerCombat& combat, float deltaTime)
{
    if (player.IsAttachedToPlatform()) {
        return;
    }

    const glm::vec3 attackDirection =
        mHasStrongAttackDirectionOverride ? mStrongAttackDirectionOverride : player.GetFacingForwardVec();
    const glm::vec3 movementDelta = attackDirection * combat.GetStrongAttackSpeed() * deltaTime;

    MoveWithCollision(mPhysicsSystem, player, movementDelta);
}

void PlayerMovement::ApplyKnockBackMovement(Player& player, float deltaTime)
{
    if (player.IsAttachedToPlatform()) {
        return;
    }

    const glm::vec3 tangentialMovementDelta =
        mKnockBackDirection *
        mKnockBackSpeed *
        deltaTime;
    MoveWithCollision(
        mPhysicsSystem,
        player,
        tangentialMovementDelta);
    ApplyJumpGravity(player, deltaTime);
}

void PlayerMovement::StartKnockBack(
    Player& player,
    const glm::vec3& damageSourcePosition)
{
    const glm::vec3 upDirection =
        GetNormalizedUpDirection(player);
    const glm::vec3 awayFromDamageSource =
        player.GetPos() - damageSourcePosition;

    if (!TryNormalizeDirection(
            ProjectOntoPlane(
                awayFromDamageSource,
                upDirection),
            mKnockBackDirection) &&
        !TryNormalizeDirection(
            ProjectOntoPlane(
                -player.GetFacingForwardVec(),
                upDirection),
            mKnockBackDirection)) {
        mKnockBackDirection =
            CreatePerpendicularDirection(upDirection);
    }

    player.DetachFromPlatform();
    RecordEllipseAirborneStartSurfaceNormal(player);
    CancelJumpApexHover();
    CancelAirborneActionHover();
    constexpr float upwardSpeedRatio = 0.5f;
    player.SetVelocity(
        upDirection *
        mKnockBackSpeed *
        upwardSpeedRatio);
    player.SetOnGround(false);
    player.SetShouldJudgeLanding(false);
}

void PlayerMovement::StartDodgeMovement(Player& player, const PlayerInput& input)
{
    const glm::vec3 requestedDodgeDirection =
        mForwardVec * input.GetMoveForward() +
        mLeftVec * input.GetMoveLeft();
    glm::vec3 inputDodgeDirection;
    const bool hasMovementInput =
        TryNormalizeDirection(requestedDodgeDirection, inputDodgeDirection);

    glm::vec3 facingDirection;

    if (!TryNormalizeDirection(player.GetFacingForwardVec(), facingDirection) &&
        !TryNormalizeDirection(mForwardVec, facingDirection)) {
        facingDirection = glm::vec3(0.0f, 0.0f, 1.0f);
    }

    const glm::vec3 dodgeDirection =
        hasMovementInput ? inputDodgeDirection : -facingDirection;
    StartDodgeMovementInDirection(
        player,
        dodgeDirection,
        player.ShouldUseEllipseSurfaceGravity()
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
    if (!player.GetOnGround()) {
        player.RestartAirborneGravityFallbackDelay();
        CancelJumpApexHover();
        CancelAirborneActionHover();
    }

    mDodgeDir = dodgeDirection;
    mDodgeTrajectory = trajectory;
    player.SetVelocity(glm::vec3(0.0f));
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



    player.SetVelocity(glm::vec3(0.0f));

    const float surfaceMovementMultiplier =
        CalculateEllipseAirControlMultiplier(
            visualUpDirection,
            physicsUpDirection);
    const glm::vec3 requestedMovement =
        mDodgeDir *
        dodgeSpeed *
        surfaceMovementMultiplier *
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
    player.DetachFromPlatform();
    RecordEllipseAirborneStartSurfaceNormal(player);
    mCanStartJumpApexHover = true;
    mJumpApexHoverRemainingSeconds = 0.0f;
    CancelAirborneActionHover();

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
        mPhysicsSystem,
        player,
        upDirection,
        player.GetVelocity() * deltaTime);

    player.SetOnGround(false);
    player.SetShouldJudgeLanding(false);
}

void PlayerMovement::ApplyJumpGravity(Player& player, float deltaTime)
{
    ApplyJumpGravityMovement(
        player,
        glm::vec3(0.0f),
        deltaTime);
}

void PlayerMovement::StopAirborneVerticalMovement(
    Player& player) const
{
    if (player.GetOnGround()) {
        return;
    }

    const glm::vec3 physicsUpDirection =
        GetAirbornePhysicsUpDirection(player);
    const glm::vec3 velocity = player.GetVelocity();
    const float verticalSpeed =
        glm::dot(velocity, physicsUpDirection);
    player.SetVelocity(
        velocity -
        physicsUpDirection * verticalSpeed);
}

void PlayerMovement::ApplyJumpGravityAndInputMovement(
    Player& player,
    const PlayerInput& input,
    float deltaTime)
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
    float deltaTime)
{
    if (player.IsAttachedToPlatform()) {
        player.SetVelocity(glm::vec3(0.0f));
        return;
    }

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
        player.ShouldUseEllipseSurfaceGravity();

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

    if (mJumpApexHoverRemainingSeconds > 0.0f) {
        StopAirborneVerticalMovement(player);
        mJumpApexHoverRemainingSeconds =
            std::max(
                0.0f,
                mJumpApexHoverRemainingSeconds -
                    deltaTime);
    } else {
        const float verticalSpeedBeforeGravity =
            glm::dot(
                player.GetVelocity(),
                physicsUpDirection);
        const float gravityAcceleration =
            CalculateAirborneGravityAcceleration(
                verticalSpeedBeforeGravity);
        player.AddVelocity(
            -physicsUpDirection *
            gravityAcceleration *
            deltaTime);

        const float verticalSpeedAfterGravity =
            glm::dot(
                player.GetVelocity(),
                physicsUpDirection);
        const bool didReachJumpApex =
            mCanStartJumpApexHover &&
            verticalSpeedBeforeGravity > 0.0f &&
            verticalSpeedAfterGravity <= 0.0f;
        if (didReachJumpApex) {
            StopAirborneVerticalMovement(player);
            mCanStartJumpApexHover = false;
            mJumpApexHoverRemainingSeconds =
                mJumpApexHoverDurationSeconds;
        }
    }

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
        mPhysicsSystem,
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

void PlayerMovement::CancelJumpApexHover()
{
    mCanStartJumpApexHover = false;
    mJumpApexHoverRemainingSeconds = 0.0f;
}

void PlayerMovement::StartAirborneActionHover(
    float durationSeconds)
{
    mAirborneActionHoverRemainingSeconds =
        std::max(0.0f, durationSeconds);
}

bool PlayerMovement::UpdateAirborneActionHover(
    Player& player,
    float deltaTime)
{
    if (player.GetOnGround() ||
        mAirborneActionHoverRemainingSeconds <= 0.0f) {
        CancelAirborneActionHover();
        return false;
    }

    StopAirborneVerticalMovement(player);
    mAirborneActionHoverRemainingSeconds =
        std::max(
            0.0f,
            mAirborneActionHoverRemainingSeconds -
                std::max(0.0f, deltaTime));
    return true;
}

void PlayerMovement::CancelAirborneActionHover()
{
    mAirborneActionHoverRemainingSeconds = 0.0f;
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
    player.RestartAirborneGravityFallbackDelay();

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
            mPhysicsSystem,
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
        mPhysicsSystem,
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
