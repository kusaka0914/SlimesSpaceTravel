#pragma once

#include "actor/CharacterActor.h"
#include "actor/player/PlayerAnimationController.h"
#include "actor/player/PlayerBoatRide.h"
#include "actor/player/PlayerCombat.h"
#include "actor/player/PlayerGrounding.h"
#include "actor/player/PlayerInput.h"
#include "actor/player/PlayerInteraction.h"
#include "actor/player/PlayerJewelGauge.h"
#include "actor/player/PlayerMovement.h"
#include "actor/player/PlayerPlanetGravityController.h"
#include "actor/player/PlayerParticleEffectController.h"
#include "actor/player/PlayerRespawn.h"
#include "actor/player/PlayerStateMachine.h"
#include "actor/player/PlayerStatus.h"
#include "actor/player/PlayerTypes.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdint>
#include <vector>

class Game;
class NPC;
class Boat;
class Enemy;
class Planet;
struct PlayerConfig;

class Player : public CharacterActor {
public:
    using ActionState = PlayerActionState;
    using AttackKind = PlayerAttackKind;
    using PlayerRaySegment = ::PlayerRaySegment;

    explicit Player(Game* game);

    static constexpr float SplitBodyScaleMultiplier = 0.8f;
    static constexpr float SplitAttackMultiplier = 0.6f;

    void ApplyConfig(const PlayerConfig& config);

    void Initialize() override;
    void ProcessActor() override;
    void UpdateActor(float deltaTime) override;
    bool ShouldRenderSolidWhite() const override;
    glm::vec3 GetRenderPosition() const override;
    glm::vec3 GetRenderScale() const override;
    glm::quat GetRenderModelRotationOffset() const override;

    void ApplyDamage(Enemy* enemy, float deltaTime);
    void ApplyDamageFromActor(
        const glm::vec3& damageSourcePosition,
        float damage);
    void StartDamageKnockBack(
        const glm::vec3& damageSourcePosition);
    void StartNormalHitReaction();
    void StartStarCollectionCelebration(float durationSeconds);
    void StopStarCollectionCelebration();

    void ApplyFallDamageAndRespawn(float damage);
    void OnBoatArrived(Boat* boat);
    bool IsWaitingForBoat() const;
    bool CancelWaitingBoatRide();
    void RespawnAtRestartPoint();
    void Restart();
    void ForceGroundedForCinematic();
    void ForceGroundedForCinematicAt(
        Planet* planet,
        const glm::vec3& surfaceReferencePosition,
        const glm::vec3& groundUpDirection);
    void RecoverFromFatigue();
    void OnAttachedToAdhesivePlatform();
    void SetBaseScale(const glm::vec3& scale);
    void SetSplitForm(bool isSplitForm);
    void SetCurrentPlanet(Planet* currentPlanet);
    void MoveToCurrentPlanetOrigin();
    void DebugMoveToPlanet(Planet* planet, int planetIndex);
    void DebugMoveToPosition(
        const glm::vec3& worldPosition,
        Planet* planet,
        int planetIndex);
    void SetControlLocked(bool locked) { mControlLocked = locked; }
    void SuppressJumpUntilReleased()
    {
        mInput.SuppressJumpUntilReleased();
    }
    void RequestNextWeakAttackAnimation();
    void RequestStrongAttackAnimation();

    bool IsInvincible() const { return mStatus.IsInvincible(); }

    bool IsAlive() const { return mStatus.IsAlive(); }

    bool IsAttacking() const { return mStateMachine.IsAttackingState() || mCombat.IsAttacking(); }
    bool IsSpecialCharging() const { return mCombat.IsSpecialCharging(); }
    bool IsContinuousAttacking() const
    {
        return mCombat.IsContinuousAttacking();
    }
    std::uint64_t GetResolvedAttackSequence() const
    {
        return mCombat.GetResolvedAttackSequence();
    }

    void SetHasUsedDodge(bool hasUsedDodge) { mMovement.SetHasUsedDodge(hasUsedDodge); }

    void SetCurrentPlanetNum(int currentPlanetNum) { mMovement.SetCurrentPlanetNum(currentPlanetNum); }

    void SetPlayerNum(int playerNum) { mMovement.SetPlayerNum(playerNum); }

    void SetCameraYaw(float cameraYaw) { mInput.SetCameraYaw(cameraYaw); }

    void SetCameraForwardDirection(const glm::vec3& forwardDirection, const glm::vec3& upDirection)
    {
        mMovement.SetCameraForwardDirection(forwardDirection, upDirection);
    }

    void LockMovementDirectionForCameraAutoAlign()
    {
        mMovement.LockMovementDirectionForCameraAutoAlign(mInput, GetUpVec());
    }

    void UnlockMovementDirectionForCameraAutoAlign()
    {
        mMovement.UnlockMovementDirectionForCameraAutoAlign();
    }

    bool ConsumeCameraAutoAlignCancellationRequest()
    {
        return mMovement.ConsumeCameraAutoAlignCancellationRequest();
    }

    void SetAttack(float attack) { mCombat.SetAttack(attack); }

    void SetMoveSpeed(float moveSpeed) { mMovement.SetMoveSpeed(moveSpeed); }

    void SetMaximumStepHeight(float maximumStepHeight)
    {
        mMovement.SetMaximumStepHeight(maximumStepHeight);
    }

    void SetAttackSpeed(float attackSpeed) { mCombat.SetAttackSpeed(attackSpeed); }

    void SetHp(float hp) { mStatus.SetHp(hp); }

    void SetMaxHp(float maxHp) { mStatus.SetMaxHp(maxHp); }

    void SetDefaultDamageTimer(float defaultDamageTimer) { mStatus.SetDefaultDamageTimer(defaultDamageTimer); }

    void SetGroundWeakAttackCooldownSeconds(float cooldownSeconds)
    {
        mCombat.SetGroundWeakAttackCooldownSeconds(cooldownSeconds);
    }

    void SetAirWeakAttackCooldownSeconds(float cooldownSeconds)
    {
        mCombat.SetAirWeakAttackCooldownSeconds(cooldownSeconds);
    }

    void SetLastAttackCooldown(float lastAttackCooldown) { mCombat.SetLastAttackCooldown(lastAttackCooldown); }

    void SetSpecialAttackCooldown(float specialAttackCooldown)
    {
        mCombat.SetSpecialAttackCooldown(specialAttackCooldown);
    }

    void SetDodgeDuration(float dodgeDuration) { mMovement.SetDodgeDuration(dodgeDuration); }

    void SetDefaultInvincibleTimer(float defaultInvincibleTimer)
    {
        mStatus.SetDefaultInvincibleTimer(defaultInvincibleTimer);
    }

    void SetDodgeCooldownTime(float dodgeCooldownTime) { mMovement.SetDodgeCooldownTime(dodgeCooldownTime); }

    void SetDodgeDistance(float dodgeDistance) { mMovement.SetDodgeDistance(dodgeDistance); }

    void SetNormalAttackRange(float normalAttackRange) { mCombat.SetNormalAttackRange(normalAttackRange); }

    void SetNormalAttackAngle(float normalAttackAngle) { mCombat.SetNormalAttackAngle(normalAttackAngle); }

    void SetNormalAttack(float normalAttack) { mCombat.SetNormalAttack(normalAttack); }

    void SetWideAttackRange(float wideAttackRange) { mCombat.SetWideAttackRange(wideAttackRange); }

    void SetWideAttackAngle(float wideAttackAngle) { mCombat.SetWideAttackAngle(wideAttackAngle); }

    void SetWideAttack(float wideAttack) { mCombat.SetWideAttack(wideAttack); }

    void SetAirDodgeAttackDamage(float damage) { mCombat.SetAirDodgeAttackDamage(damage); }

    void SetAirDodgeHorizontalHitboxScale(float scale)
    {
        mCombat.SetAirDodgeHorizontalHitboxScale(scale);
    }

    void SetAirDodgeVerticalHitboxScale(float scale)
    {
        mCombat.SetAirDodgeVerticalHitboxScale(scale);
    }

    void SetAirDodgeEnemyPushSpeed(float speed)
    {
        mCombat.SetAirDodgeEnemyPushSpeed(speed);
    }

    void SetAirDodgeEnemyPushDampingPerSecond(float dampingPerSecond)
    {
        mCombat.SetAirDodgeEnemyPushDampingPerSecond(dampingPerSecond);
    }

    void SetStrongAttackRange(float strongAttackRange) { mCombat.SetStrongAttackRange(strongAttackRange); }

    void SetStrongAttack(float strongAttack) { mCombat.SetStrongAttack(strongAttack); }

    void SetStrongAttackSpeed(float strongAttackSpeed) { mCombat.SetStrongAttackSpeed(strongAttackSpeed); }

    void SetChargedAttackRange(float chargedAttackRange) { mCombat.SetChargedAttackRange(chargedAttackRange); }

    void SetChargedAttackAngle(float chargedAttackAngle) { mCombat.SetChargedAttackAngle(chargedAttackAngle); }

    void SetChargedAttackDamage(float chargedAttackDamage) { mCombat.SetChargedAttackDamage(chargedAttackDamage); }

    void SetChargedAttackChargeDurationSeconds(float chargeDurationSeconds)
    {
        mCombat.SetChargedAttackChargeDurationSeconds(chargeDurationSeconds);
    }

    void SetContinuousAttackRange(float continuousAttackRange)
    {
        mCombat.SetContinuousAttackRange(continuousAttackRange);
    }

    void SetContinuousAttackAngle(float continuousAttackAngle)
    {
        mCombat.SetContinuousAttackAngle(continuousAttackAngle);
    }

    void SetContinuousAttackDamage(float continuousAttackDamage)
    {
        mCombat.SetContinuousAttackDamage(continuousAttackDamage);
    }

    void SetContinuousAttackIntervalSeconds(float attackIntervalSeconds)
    {
        mCombat.SetContinuousAttackIntervalSeconds(attackIntervalSeconds);
    }

    void SetContinuousAttackDurationSeconds(float attackDurationSeconds)
    {
        mCombat.SetContinuousAttackDurationSeconds(attackDurationSeconds);
    }

    void SetDefaultStrongAttackTimer(float defaultStrongAttackTimer)
    {
        mCombat.SetDefaultStrongAttackTimer(defaultStrongAttackTimer);
    }

    void SetDefaultAttackMotionTimer(float defaultAttackMotionTimer)
    {
        mCombat.SetDefaultAttackMotionTimer(defaultAttackMotionTimer);
    }

    void SetAttackHitDelay(float attackHitDelay)
    {
        mCombat.SetAttackHitDelay(attackHitDelay);
    }

    void SetRayCastTimer(float rayCastTimer) { mGrounding.SetRayCastTimer(rayCastTimer); }

    void SetInputAvailableTimer(float inputAvailableTimer) { mInput.SetInputAvailableTimer(inputAvailableTimer); }

    void SetKnockBackSpeed(float knockBackSpeed) { mMovement.SetKnockBackSpeed(knockBackSpeed); }
    void SetJumpHeight(float jumpHeight) { mMovement.SetJumpHeight(jumpHeight); }
    void SetJumpAscentDuration(float duration) { mMovement.SetJumpAscentDuration(duration); }
    void SetJumpFallDuration(float duration) { mMovement.SetJumpFallDuration(duration); }
    void SetJumpApexHoverDurationSeconds(float durationSeconds)
    {
        mMovement.SetJumpApexHoverDurationSeconds(durationSeconds);
    }
    void SetAirWeakAttackPostHoverDurationSeconds(
        float durationSeconds)
    {
        mMovement.SetAirWeakAttackPostHoverDurationSeconds(
            durationSeconds);
    }
    void SetAirDodgePostHoverDurationSeconds(float durationSeconds)
    {
        mMovement.SetAirDodgePostHoverDurationSeconds(
            durationSeconds);
    }
    void SetAirSlamRiseHeight(float riseHeight) { mMovement.SetAirSlamRiseHeight(riseHeight); }
    void SetAirSlamRiseDurationSeconds(float durationSeconds)
    {
        mMovement.SetAirSlamRiseDurationSeconds(durationSeconds);
    }
    void SetAirSlamHoverDurationSeconds(float durationSeconds)
    {
        mMovement.SetAirSlamHoverDurationSeconds(durationSeconds);
    }

    void SetVelocity(const glm::vec3& velocity) { mVelocity = velocity; }

    void SetTalkableNPC(NPC* talkableNPC) { mInteraction.SetTalkableNPC(talkableNPC); }

    bool GetIsStrongAttacked() const { return mCombat.GetIsStrongAttacked(); }

    bool GetIsSpecialAttackPressed() const { return mInput.GetSpecialAttackPressed(); }
    void NotifyJumpStarted() { ++mJumpSequence; }
    std::uint64_t GetJumpSequence() const { return mJumpSequence; }

    bool GetCanSpecialAttack() const { return mCombat.GetCanSpecialAttack(); }

    bool GetIsTired() const { return mStatus.GetIsTired(); }

    int GetCurrentPlanetNum() const { return mMovement.GetCurrentPlanetNum(); }

    int GetJewelCount() const { return mJewelGauge.GetCount(); }
    void SetJewelCount(int count) { mJewelGauge.SetCount(count); }
    void AddJewelFromItem();

    int GetPlayerNum() const { return mMovement.GetPlayerNum(); }

    float GetAttack() const { return mCombat.GetAttack(); }
    float CalculateOutgoingAttackDamage(float baseDamage) const;
    float GetCollisionScaleMultiplier() const override
    {
        return mIsSplitForm ? SplitBodyScaleMultiplier : 1.0f;
    }

    float GetHp() const { return mStatus.GetHp(); }

    float GetAttackMotionTimer() const { return mCombat.GetAttackMotionTimer(); }

    float GetStrongAttackTimer() const { return mCombat.GetStrongAttackTimer(); }

    float GetInvincibleTimer() const { return mStatus.GetInvincibleTimer(); }

    float GetSpecialAttackCooldownRemaining() const { return mJewelGauge.GetRecoverTimer(); }

    float GetAttackRange() const { return mCombat.GetAttackRange(); }

    float GetAttackAngle() const { return mCombat.GetAttackAngle(); }

    float GetRayCastTimer() const { return mGrounding.GetRayCastTimer(); }

    float GetMoveSpeed() const { return mMovement.GetMoveSpeed(); }

    float GetMaximumStepHeight() const
    {
        return mMovement.GetMaximumStepHeight();
    }

    float GetCameraYaw() const { return mInput.GetCameraYaw(); }

    float GetMoveForwardInput() const { return mInput.GetMoveForward(); }
    float GetMoveLeftInput() const { return mInput.GetMoveLeft(); }

    float GetAttackSpeed() const { return mCombat.GetAttackSpeed(); }

    float GetMaxHp() const { return mStatus.GetMaxHp(); }

    float GetDefaultDamageTimer() const { return mStatus.GetDefaultDamageTimer(); }

    float GetGroundWeakAttackCooldownSeconds() const
    {
        return mCombat.GetGroundWeakAttackCooldownSeconds();
    }

    float GetAirWeakAttackCooldownSeconds() const
    {
        return mCombat.GetAirWeakAttackCooldownSeconds();
    }

    float GetLastAttackCooldown() const { return mCombat.GetLastAttackCooldown(); }

    float GetSpecialAttackCooldown() const { return mCombat.GetSpecialAttackCooldown(); }

    float GetDodgeDuration() const { return mMovement.GetDodgeDuration(); }

    float GetDodgeCooldownTime() const { return mMovement.GetDodgeCooldownTime(); }

    float GetDodgeDistance() const { return mMovement.GetDodgeDistance(); }

    float GetDefaultInvincibleTimer() const { return mStatus.GetDefaultInvincibleTimer(); }

    float GetNormalAttackRange() const { return mCombat.GetNormalAttackRange(); }

    float GetNormalAttackAngle() const { return mCombat.GetNormalAttackAngle(); }

    float GetNormalAttack() const { return mCombat.GetNormalAttack(); }

    float GetWideAttackRange() const { return mCombat.GetWideAttackRange(); }

    float GetWideAttackAngle() const { return mCombat.GetWideAttackAngle(); }

    float GetWideAttack() const { return mCombat.GetWideAttack(); }

    float GetAirDodgeAttackDamage() const { return mCombat.GetAirDodgeAttackDamage(); }

    float GetAirDodgeHorizontalHitboxScale() const
    {
        return mCombat.GetAirDodgeHorizontalHitboxScale();
    }

    float GetAirDodgeVerticalHitboxScale() const
    {
        return mCombat.GetAirDodgeVerticalHitboxScale();
    }

    float GetAirDodgeEnemyPushSpeed() const
    {
        return mCombat.GetAirDodgeEnemyPushSpeed();
    }

    float GetAirDodgeEnemyPushDampingPerSecond() const
    {
        return mCombat.GetAirDodgeEnemyPushDampingPerSecond();
    }

    float GetStrongAttackRange() const { return mCombat.GetStrongAttackRange(); }

    float GetStrongAttack() const { return mCombat.GetStrongAttack(); }

    float GetStrongAttackSpeed() const { return mCombat.GetStrongAttackSpeed(); }

    float GetChargedAttackRange() const { return mCombat.GetChargedAttackRange(); }

    float GetChargedAttackAngle() const { return mCombat.GetChargedAttackAngle(); }

    float GetChargedAttackDamage() const { return mCombat.GetChargedAttackDamage(); }

    float GetChargedAttackChargeDurationSeconds() const
    {
        return mCombat.GetChargedAttackChargeDurationSeconds();
    }

    float GetContinuousAttackRange() const { return mCombat.GetContinuousAttackRange(); }

    float GetContinuousAttackAngle() const { return mCombat.GetContinuousAttackAngle(); }

    float GetContinuousAttackDamage() const { return mCombat.GetContinuousAttackDamage(); }

    float GetContinuousAttackIntervalSeconds() const
    {
        return mCombat.GetContinuousAttackIntervalSeconds();
    }

    float GetContinuousAttackDurationSeconds() const
    {
        return mCombat.GetContinuousAttackDurationSeconds();
    }

    float GetDefaultStrongAttackTimer() const { return mCombat.GetDefaultStrongAttackTimer(); }

    float GetDefaultAttackMotionTimer() const { return mCombat.GetDefaultAttackMotionTimer(); }

    float GetAttackHitDelay() const { return mCombat.GetAttackHitDelay(); }

    float GetInputAvailableTimer() const { return mInput.GetInputAvailableTimer(); }

    float GetKnockBackSpeed() const { return mMovement.GetKnockBackSpeed(); }
    float GetJumpHeight() const { return mMovement.GetJumpHeight(); }
    float GetJumpAscentDuration() const { return mMovement.GetJumpAscentDuration(); }
    float GetJumpFallDuration() const { return mMovement.GetJumpFallDuration(); }
    float GetJumpApexHoverDurationSeconds() const
    {
        return mMovement.GetJumpApexHoverDurationSeconds();
    }
    float GetAirWeakAttackPostHoverDurationSeconds() const
    {
        return mMovement.GetAirWeakAttackPostHoverDurationSeconds();
    }
    float GetAirDodgePostHoverDurationSeconds() const
    {
        return mMovement.GetAirDodgePostHoverDurationSeconds();
    }
    float GetAirSlamRiseHeight() const { return mMovement.GetAirSlamRiseHeight(); }
    float GetAirSlamRiseDurationSeconds() const { return mMovement.GetAirSlamRiseDurationSeconds(); }
    float GetAirSlamHoverDurationSeconds() const { return mMovement.GetAirSlamHoverDurationSeconds(); }

    bool WasPlanetGravityFallbackAppliedThisJump() const
    {
        return mPlanetGravityController.WasFallbackAppliedThisJump();
    }
    bool IsEllipseAirborneGravityActive() const
    {
        return mPlanetGravityController
            .IsEllipseAirborneGravityActive(*this);
    }
    bool ShouldUseEllipseSurfaceGravity() const
    {
        return mPlanetGravityController
            .ShouldUseEllipseSurfaceGravity(*this);
    }
    glm::vec3 CalculateAirbornePhysicsUpDirection() const
    {
        return mPlanetGravityController
            .CalculateAirbornePhysicsUpDirection(*this);
    }
    void RestartAirborneGravityFallbackDelay()
    {
        mPlanetGravityController
            .RestartFallbackDelayForAirborneAction(*this);
    }

    ActionState GetActionState() const { return mStateMachine.GetActionState(); }

    const glm::vec3& GetForwardVec() const { return mMovement.GetForwardVec(); }

    const std::vector<::PlayerRaySegment>& GetRayCasts() const { return mCombat.GetRayCasts(); }

    const std::vector<glm::mat4>* GetSkinningMatrices() const override;

    NPC* GetTalkableNPC() const { return mInteraction.GetTalkableNPC(); }
    Enemy* GetAttackDirectionTarget() const { return mStateMachine.GetAttackDirectionTarget(); }

    const glm::vec3& GetVelocity() const { return mVelocity; }

    void AddVelocity(const glm::vec3& velocityDelta) { mVelocity += velocityDelta; }

    void SetFacingForwardVec(const glm::vec3& facingForwardVec) { mFacingForwardVec = facingForwardVec; }
    void FaceDirection(const glm::vec3& facingDirection) { mMovement.FaceDirection(*this, facingDirection); }

    void SetOnGround(bool onGround) { mOnGround = onGround; }

    void SetShouldJudgeLanding(bool shouldJudgeLanding) { mShouldJudgeLanding = shouldJudgeLanding; }

    void ApplyGravityToSelf(float deltaTime) { ApplyGravity(deltaTime); }
    void ApplyGravityToSelf(float deltaTime, float gravityAcceleration)
    {
        ApplyGravity(deltaTime, gravityAcceleration);
    }

    void RefreshFallbackUpVec() { UpdateFallbackUpVec(); }

private:
    void UpdateNormalHitReaction(float deltaTime);
    void UpdateStarCollectionCelebration(float deltaTime);
    void RequestEnteredActionAnimation(PlayerActionState previousState, PlayerActionState currentState);

    bool ShouldAcceptLandingSurface(
        Actor* surfaceActor,
        const glm::vec3& surfaceNormal) const override;

    void OnLanded() override;
    void OnUpVecUpdateFailed() override;
    void OnGroundSurfaceDetected() override;
    void OnCastSucceeded() override;
    void OnLoadedModelChanged() override;

private:
    PlayerInput mInput;
    PlayerMovement mMovement;
    PlayerGrounding mGrounding;
    PlayerPlanetGravityController mPlanetGravityController;
    PlayerBoatRide mBoatRide;
    PlayerCombat mCombat;
    PlayerJewelGauge mJewelGauge;
    PlayerStatus mStatus;
    PlayerRespawn mRespawn;
    PlayerInteraction mInteraction;
    PlayerStateMachine mStateMachine;
    PlayerParticleEffectController mParticleEffectController;
    PlayerAnimationController mAnimationController;

    bool mUseSecondAttackAnimationNext = false;
    bool mControlLocked = false;
    bool mIsSplitForm = false;
    float mNormalHitReactionElapsedSeconds = -1.0f;
    float mStarCollectionCelebrationElapsedSeconds = -1.0f;
    float mStarCollectionCelebrationDurationSeconds = 0.0f;
    std::uint64_t mJumpSequence = 0;
};
