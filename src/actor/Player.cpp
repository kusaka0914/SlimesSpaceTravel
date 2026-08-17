#include "Player.h"

#include "actor/Boat.h"
#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/player/PlayerConfig.h"
#include "actor/player/PlayerConfigLoader.h"
#include "actor/player/PlayerDamageHandler.h"
#include "component/PlatformBehaviorComponents.h"
#include "system/PhysicsSystem.h"

#include <algorithm>
#include <cmath>
#include <string_view>

namespace {
constexpr std::string_view idleAnimationId = "idle";
constexpr std::string_view walkAnimationId = "walk";
constexpr std::string_view dodgeAnimationId = "dodge";
constexpr std::string_view attackAnimationId = "attack";
constexpr std::string_view secondAttackAnimationId = "second_attack";
constexpr std::string_view strongAttackAnimationId = "strong_attack";
} // namespace

Player::Player(Game* game)
    : CharacterActor(game)
{
}

void Player::ApplyConfig()
{
    const PlayerConfig config = PlayerConfigLoader::Load("../assets/data/actor/players.yaml");

    ApplyPlayerConfig(config);
}

void Player::ApplyPlayerConfig(const PlayerConfig& config)
{
    mStatus.ConfigureHp(config.hp);
    SetScale(glm::vec3(config.scale));

    mCombat.SetAttackSpeed(config.attackSpeed);
    mCombat.SetAttack(config.attack);
    mMovement.SetMoveSpeed(config.moveSpeed);
    mMovement.SetMaximumStepHeight(config.maximumStepHeight);
    mMovement.SetJumpHeight(config.jumpHeight);
    mMovement.SetJumpAscentDuration(config.jumpAscentDuration);
    mMovement.SetJumpFallDuration(config.jumpFallDuration);
    mMovement.SetJumpApexHoverDurationSeconds(
        config.jumpApexHoverDurationSeconds);
    mMovement.SetAirWeakAttackPostHoverDurationSeconds(
        config.airWeakAttackPostHoverDurationSeconds);
    mMovement.SetAirDodgePostHoverDurationSeconds(
        config.airDodgePostHoverDurationSeconds);
    if (mGame) {
        mGame->SetGroundNormalRayLength(config.groundNormalRayLength);
        mGame->SetOverheadGravityRayLength(
            config.overheadGravityRayLength);

        PhysicsSystem* physicsSystem = mGame->GetPhysicsSystem();
        if (physicsSystem) {
            physicsSystem->SetPlayerCollisionWidth(config.collisionWidth);
            physicsSystem->SetPlayerCollisionHeight(config.collisionHeight);
            physicsSystem->SetPlayerCollisionDepth(config.collisionDepth);
            physicsSystem->SetPlayerCollisionCenterHeight(
                config.collisionCenterHeight);
        }
    }

    mMovement.SetDodgeDuration(config.dodgeDuration);
    mMovement.SetDodgeCooldownTime(config.dodgeCooldownTime);
    mMovement.SetDodgeDistance(config.dodgeDistance);

    mCombat.SetNormalAttackRange(config.normalAttackRange);
    mCombat.SetNormalAttackAngle(config.normalAttackAngle);
    mCombat.SetNormalAttack(config.normalAttack);

    mCombat.SetWideAttackRange(config.wideAttackRange);
    mCombat.SetWideAttackAngle(config.wideAttackAngle);
    mCombat.SetWideAttack(config.wideAttack);

    mCombat.SetStrongAttackRange(config.strongAttackRange);
    mCombat.SetStrongAttack(config.strongAttack);
    mCombat.SetStrongAttackSpeed(config.strongAttackSpeed);
    mCombat.SetChargedAttackRange(config.chargedAttackRange);
    mCombat.SetChargedAttackAngle(config.chargedAttackAngle);
    mCombat.SetChargedAttackDamage(config.chargedAttackDamage);
    mCombat.SetChargedAttackChargeDurationSeconds(
        config.chargedAttackChargeDurationSeconds);
    mCombat.SetContinuousAttackRange(config.continuousAttackRange);
    mCombat.SetContinuousAttackAngle(config.continuousAttackAngle);
    mCombat.SetContinuousAttackDamage(config.continuousAttackDamage);
    mCombat.SetContinuousAttackIntervalSeconds(
        config.continuousAttackIntervalSeconds);
    mCombat.SetContinuousAttackDurationSeconds(
        config.continuousAttackDurationSeconds);
    mMovement.SetAirSlamRiseHeight(config.airSlamRiseHeight);
    mMovement.SetAirSlamRiseDurationSeconds(
        config.airSlamRiseDurationSeconds);
    mMovement.SetAirSlamHoverDurationSeconds(
        config.airSlamHoverDurationSeconds);

    mCombat.SetSpecialAttackCooldown(config.specialAttackCooldown);
    mStatus.SetDefaultInvincibleTimer(config.defaultInvincibleTimer);
    mStatus.SetDefaultDamageTimer(config.defaultDamageTimer);
    mCombat.SetDefaultAttackMotionTimer(config.defaultAttackMotionTimer);
    mCombat.SetAttackHitDelay(config.attackHitDelay);
    mCombat.SetAttackCooldown(config.attackCooldown);
    mCombat.SetLastAttackCooldown(config.lastAttackCooldown);
    mCombat.SetDefaultStrongAttackTimer(config.defaultStrongAttackTimer);
    mMovement.SetKnockBackSpeed(config.knockBackSpeed);

    SetModelPath(config.modelPath);

    mAnimationController.Configure(config.animations);
}

void Player::Initialize()
{
    mRespawn.SetRestartPlanetIndex(mMovement.GetCurrentPlanetNum());
    mRespawn.SetRestartPos(mPos);
}

bool Player::ShouldRenderSolidWhite() const
{
    if (!mStatus.IsAlive() ||
        !mStatus.ShouldBlinkWhileInvincible()) {
        return false;
    }

    constexpr float blinkIntervalSeconds = 0.1f;
    const int blinkPhase = static_cast<int>(
        mStatus.GetInvincibleTimer() / blinkIntervalSeconds);
    return (blinkPhase % 2) == 0;
}

void Player::RecoverFromFatigue()
{
    if (!mStatus.IsTired()) {
        return;
    }

    mCombat.EndTiredLock(mStatus, mMovement);
    mStateMachine.ChangeState(PlayerActionState::Idle);
}

void Player::MoveToCurrentPlanetOrigin()
{
    Planet* planet = GetCurrentPlanet();
    if (!planet) {
        return;
    }

    constexpr float originTheta = 0.0f;
    constexpr float originPhi = 0.0f;
    SetSphericalPlacement(originTheta, originPhi, GetHeight());
    SetPos(planet->CalculateSurfacePos(originTheta, originPhi, GetHeight()));
    SetVelocity(glm::vec3(0.0f));
    SetOnGround(false);
    SetShouldJudgeLanding(true);
    RefreshFallbackUpVec();

    mRespawn.SetRestartPos(GetPos());
}

void Player::DebugMoveToPlanet(Planet* planet, int planetIndex)
{
    if (!planet || planetIndex < 0) {
        return;
    }

    DebugMoveToPosition(
        planet->CalculateSurfacePos(GetTheta(), GetPhi(), GetHeight()),
        planet,
        planetIndex);
}

void Player::DebugMoveToPosition(
    const glm::vec3& worldPosition,
    Planet* planet,
    int planetIndex)
{
    if (!planet || planetIndex < 0) {
        return;
    }

    constexpr float positionEpsilon = 0.000001f;
    const glm::vec3 planetOffset = worldPosition - planet->GetPos();
    const float centerDistance = glm::length(planetOffset);
    if (centerDistance > positionEpsilon) {
        const glm::vec3 radialDirection = planetOffset / centerDistance;
        const float theta =
            std::atan2(radialDirection.z, radialDirection.x);
        const float phi =
            std::asin(glm::clamp(radialDirection.y, -1.0f, 1.0f));

        float height = centerDistance - std::abs(planet->GetRadius());
        if (planet->GetPlanetShape() == Planet::PlanetShape::Ellipse) {
            const Planet::EllipseSurfaceProjection surfaceProjection =
                planet->CalculateEllipseSurfaceProjection(worldPosition);
            height = surfaceProjection.isOutside
                ? surfaceProjection.distance
                : -surfaceProjection.distance;
        }
        SetSphericalPlacement(theta, phi, height);
    }

    mRespawn.SetRestartPlanetIndex(planetIndex);
    mRespawn.SetRestartPos(worldPosition);
    RespawnAtRestartPoint();
}

void Player::ProcessActor()
{
    mInput.ProcessActor(*this, mMovement);
}

void Player::UpdateActor(float deltaTime)
{
    if (GetIsActive() && GetCurrentPlanet() && !IsAttachedToPlatform()) {
        for (Platform* platform : GetCurrentPlanet()->GetPlatforms()) {
            if (!platform) {
                continue;
            }

            PlatformAdhesionComponent* adhesionComponent =
                platform->GetAdhesionComponent();
            if (adhesionComponent &&
                adhesionComponent->TryAttachPlayerIfTouching(*this)) {
                break;
            }
        }
    }

    PhysicsSystem* physicsSystem =
        mGame
            ? mGame->GetPhysicsSystem()
            : nullptr;
    if (GetIsActive() && physicsSystem && !IsAttachedToPlatform()) {
        const ActorMovementCollisionResult overlapResolution =
            physicsSystem->ResolveMovementCollision(
                this,
                glm::vec3(0.0f),
                GetPos());
        SetPos(overlapResolution.resolvedPosition);
    }

    if (mControlLocked) {
        mVelocity = glm::vec3(0.0f);
        mParticleEffectController.UpdateSpecialCharging(*this, false, deltaTime);
        mParticleEffectController.UpdateWalking(*this, false);
        mAnimationController.RequestAnimation(idleAnimationId, false);
        mAnimationController.Update(deltaTime);
        return;
    }

    if (mRespawn.UpdateMissingGroundSurfaceRespawn(*this, mStatus, deltaTime)) {
        mVelocity = glm::vec3(0.0f);
        return;
    }

    const bool wasOnGroundBeforeLandingCheck = GetOnGround();
    const glm::vec3 velocityBeforeLandingCheck = GetVelocity();
    const glm::vec3 upBeforeLandingCheck = GetUpVec();

    CharacterActor::UpdateActor(deltaTime);

    const bool didLand = !wasOnGroundBeforeLandingCheck && GetOnGround();
    const bool didWalkOffGround =
        wasOnGroundBeforeLandingCheck && !GetOnGround();
    if (didWalkOffGround) {
        // Jump input starts airborne gravity in the state machine below, but
        // simply walking off an edge bypasses that transition.
        mPlanetGravityController.OnJumpStarted(
            *this,
            mMovement);
    }

    float landingSpeed = 0.0f;
    const float upLengthSquared = glm::dot(upBeforeLandingCheck, upBeforeLandingCheck);
    if (didLand && upLengthSquared > 0.000001f) {
        const glm::vec3 normalizedUp = upBeforeLandingCheck / std::sqrt(upLengthSquared);
        landingSpeed = std::max(0.0f, -glm::dot(velocityBeforeLandingCheck, normalizedUp));
    }

    const bool wasOnGroundBeforeStateUpdate = GetOnGround();
    const PlayerActionState previousActionState = mStateMachine.GetActionState();

    mPlanetGravityController.Update(*this, mMovement, deltaTime);

    mStateMachine.Update(*this, mInput, mMovement, mGrounding, mBoatRide, mCombat, mJewelGauge, mStatus, mRespawn,
                         deltaTime);

    if (wasOnGroundBeforeStateUpdate && !GetOnGround()) {
        mPlanetGravityController.OnJumpStarted(
            *this,
            mMovement);
    }

    const PlayerActionState currentActionState = mStateMachine.GetActionState();

    constexpr float movementInputDeadZone = 0.01f;
    const bool hasMovementInput = std::abs(mInput.GetMoveForward()) > movementInputDeadZone ||
                                  std::abs(mInput.GetMoveLeft()) > movementInputDeadZone;
    const bool shouldWalk = currentActionState == PlayerActionState::Idle && GetIsActive() && GetOnGround() &&
                            !IsAttachedToPlatform() &&
                            hasMovementInput;

    if (didLand) {
        mParticleEffectController.EmitLanding(*this, landingSpeed);
    }
    const bool isSpecialChargeActive =
        mCombat.IsSpecialCharging() || mCombat.GetCanSpecialAttack();
    mParticleEffectController.UpdateSpecialCharging(
        *this, isSpecialChargeActive, deltaTime);
    mParticleEffectController.UpdateWalking(*this, shouldWalk);

    mAnimationController.RequestAnimation(shouldWalk ? walkAnimationId : idleAnimationId, false);
    RequestEnteredActionAnimation(previousActionState, currentActionState);
    mAnimationController.Update(deltaTime);
}

void Player::RequestEnteredActionAnimation(PlayerActionState previousState, PlayerActionState currentState)
{
    if (previousState == currentState) {
        return;
    }

    switch (currentState) {
    case PlayerActionState::Dodging:
        mAnimationController.RequestAnimation(dodgeAnimationId);
        return;

    case PlayerActionState::Attacking:
        if (mCombat.GetAttackKind() == PlayerAttackKind::Wide) {
            RequestNextWeakAttackAnimation();
            return;
        }

        mAnimationController.RequestAnimation(strongAttackAnimationId);
        return;

    case PlayerActionState::StrongAttacking:
    case PlayerActionState::AirSlamAttacking:
        mAnimationController.RequestAnimation(strongAttackAnimationId);
        return;

    default:
        return;
    }
}

void Player::SetBaseScale(const glm::vec3& scale)
{
    CharacterActor::SetBaseScale(scale);

    if (mIsSplitForm) {
        SetScale(scale * SplitBodyScaleMultiplier);
    }
}

void Player::SetSplitForm(bool isSplitForm)
{
    mIsSplitForm = isSplitForm;
    const float bodyScaleMultiplier =
        mIsSplitForm ? SplitBodyScaleMultiplier : 1.0f;
    SetScale(GetBaseScale() * bodyScaleMultiplier);
}

float Player::CalculateOutgoingAttackDamage(float baseDamage) const
{
    const float attackMultiplier =
        mIsSplitForm ? SplitAttackMultiplier : 1.0f;
    return baseDamage * attackMultiplier;
}

void Player::RequestNextWeakAttackAnimation()
{
    const std::string_view weakAttackAnimationId =
        mUseSecondAttackAnimationNext
            ? secondAttackAnimationId
            : attackAnimationId;
    mAnimationController.RequestAnimation(weakAttackAnimationId);
    mUseSecondAttackAnimationNext = !mUseSecondAttackAnimationNext;
}

void Player::ApplyDamage(Enemy* enemy, float deltaTime)
{
    PlayerDamageHandler::Apply(*this, mInput, mMovement, mStateMachine, mCombat, mJewelGauge, mStatus, enemy,
                               deltaTime);
}

void Player::ApplyDamageFromActor(
    const glm::vec3& damageSourcePosition,
    float damage)
{
    PlayerDamageHandler::ApplyFromActor(
        *this,
        mInput,
        mMovement,
        mStateMachine,
        mCombat,
        mStatus,
        damageSourcePosition,
        damage);
}

void Player::ApplyFallDamageAndRespawn(float damage)
{
    mRespawn.ApplyFallDamageAndRespawn(*this, mStatus, damage);
    mInput.ClearAttackBuffer();
    mMovement.ClearStrongAttackDirectionOverride();
    mStateMachine.ClearAttackDirectionTarget();
}

void Player::OnBoatArrived(Boat* boat)
{
    mBoatRide.OnBoatArrived(*this, mMovement, mRespawn, boat);

    SetOnGround(false);
    SetShouldJudgeLanding(true);
    SetVelocity(glm::vec3(0.0f));
    mMovement.ResetEllipseAirborneSurfaceTravel();
    mMovement.CancelJumpApexHover();
    mMovement.CancelAirborneActionHover();
    mGrounding.ResetRayCastTimer();
    mPlanetGravityController.OnRespawned();
}

void Player::RespawnAtRestartPoint()
{
    mRespawn.Respawn(*this);
    mStateMachine.ChangeState(PlayerActionState::Idle);
    mCombat.CancelSpecialAttack();

    SetIsActive(true);
    SetVelocity(glm::vec3(0.0f));
    SetOnGround(false);
    SetShouldJudgeLanding(true);
    RefreshFallbackUpVec();

    mInput.ClearAttackBuffer();
    mMovement.ClearStrongAttackDirectionOverride();
    mMovement.ResetEllipseAirborneSurfaceTravel();
    mMovement.CancelJumpApexHover();
    mMovement.CancelAirborneActionHover();
    mStateMachine.ClearAttackDirectionTarget();
    mGrounding.ResetRayCastTimer();
    mPlanetGravityController.OnRespawned();
    mRespawn.OnRespawnCompleted();
    mParticleEffectController.Reset();
    mUseSecondAttackAnimationNext = false;
    mAnimationController.ResetToAnimation(idleAnimationId);
}

void Player::Restart()
{
    mStatus.RestoreFullHp();
    RespawnAtRestartPoint();
}

bool Player::ShouldAcceptLandingSurface(
    Actor* surfaceActor,
    const glm::vec3& surfaceNormal) const
{
    (void)surfaceActor;
    return mPlanetGravityController.ShouldAcceptLandingSurface(
        surfaceNormal);
}

const std::vector<glm::mat4>* Player::GetSkinningMatrices() const
{
    return mAnimationController.GetSkinningMatrices();
}

void Player::OnLanded()
{
    mMovement.ClearStrongAttackDirectionOverride();
    mMovement.ResetEllipseAirborneSurfaceTravel();
    mMovement.CancelJumpApexHover();
    mMovement.CancelAirborneActionHover();
    mPlanetGravityController.OnLanded(*this, mMovement);
    mGrounding.OnLanded(*this, mMovement, mCombat);
}

void Player::OnUpVecUpdateFailed()
{
    if (mPlanetGravityController.IsJumpGravityActive()) {
        return;
    }

    mGrounding.OnUpVecUpdateFailed(*this);
}

void Player::OnCastSucceeded()
{
    mGrounding.OnCastSucceeded();
    mPlanetGravityController.OnGroundRayCastSucceeded();
}

void Player::OnGroundSurfaceDetected()
{
    mRespawn.OnGroundSurfaceDetected();
}

void Player::OnLoadedModelChanged()
{
    mAnimationController.SetLoadedModel(GetLoadedModel());
}
