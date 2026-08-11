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

    void ApplyConfig();

    void Initialize() override;
    void ProcessActor() override;
    void UpdateActor(float deltaTime) override;

    void ApplyDamage(Enemy* enemy, float deltaTime);

    void ApplyFallDamageAndRespawn(float damage);
    void OnBoatArrived(Boat* boat);
    void RespawnAtRestartPoint();
    void Restart();
    void RecoverFromFatigue();
    void SetBaseScale(const glm::vec3& scale);
    void SetSplitForm(bool isSplitForm);
    void MoveToCurrentPlanetOrigin();
    void DebugMoveToPlanet(Planet* planet, int planetIndex);
    void SetControlLocked(bool locked) { mControlLocked = locked; }
    void RequestNextWeakAttackAnimation();

    bool IsInvincible() const { return mStatus.IsInvincible(); }

    bool IsAlive() const { return mStatus.IsAlive(); }

    bool IsAttacking() const { return mStateMachine.IsAttackingState() || mCombat.IsAttacking(); }

    void SetHasUsedDodge(bool hasUsedDodge) { mMovement.SetHasUsedDodge(hasUsedDodge); }

    void SetCurrentPlanetNum(int currentPlanetNum) { mMovement.SetCurrentPlanetNum(currentPlanetNum); }

    void SetPlayerNum(int playerNum) { mMovement.SetPlayerNum(playerNum); }

    void SetCameraYaw(float cameraYaw) { mInput.SetCameraYaw(cameraYaw); }

    void SetCameraForwardDirection(const glm::vec3& forwardDirection, const glm::vec3& upDirection)
    {
        mMovement.SetCameraForwardDirection(forwardDirection, upDirection);
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

    void SetAttackCooldown(float attackCooldown) { mCombat.SetAttackCooldown(attackCooldown); }

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

    void SetStrongAttackRange(float strongAttackRange) { mCombat.SetStrongAttackRange(strongAttackRange); }

    void SetStrongAttack(float strongAttack) { mCombat.SetStrongAttack(strongAttack); }

    void SetStrongAttackSpeed(float strongAttackSpeed) { mCombat.SetStrongAttackSpeed(strongAttackSpeed); }

    void SetDefaultStrongAttackTimer(float defaultStrongAttackTimer)
    {
        mCombat.SetDefaultStrongAttackTimer(defaultStrongAttackTimer);
    }

    void SetDefaultAttackMotionTimer(float defaultAttackMotionTimer)
    {
        mCombat.SetDefaultAttackMotionTimer(defaultAttackMotionTimer);
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

    float GetAttackSpeed() const { return mCombat.GetAttackSpeed(); }

    float GetMaxHp() const { return mStatus.GetMaxHp(); }

    float GetDefaultDamageTimer() const { return mStatus.GetDefaultDamageTimer(); }

    float GetAttackCooldown() const { return mCombat.GetAttackCooldown(); }

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

    float GetStrongAttackRange() const { return mCombat.GetStrongAttackRange(); }

    float GetStrongAttack() const { return mCombat.GetStrongAttack(); }

    float GetStrongAttackSpeed() const { return mCombat.GetStrongAttackSpeed(); }

    float GetDefaultStrongAttackTimer() const { return mCombat.GetDefaultStrongAttackTimer(); }

    float GetDefaultAttackMotionTimer() const { return mCombat.GetDefaultAttackMotionTimer(); }

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
    void ApplyPlayerConfig(const PlayerConfig& config);
    void RequestEnteredActionAnimation(PlayerActionState previousState, PlayerActionState currentState);

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
    std::uint64_t mJumpSequence = 0;
};
