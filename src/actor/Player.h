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
#include "actor/player/PlayerParticleEffectController.h"
#include "actor/player/PlayerPlanetGravityController.h"
#include "actor/player/PlayerRespawn.h"
#include "actor/player/PlayerStateMachine.h"
#include "actor/player/PlayerStatus.h"
#include "actor/player/PlayerTypes.h"

#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

class Game;
class NPC;
class Boat;
class Enemy;
class Planet;
struct PlayerConfig;

// プレイヤーActorの更新と外部操作を統括する。移動・入力・戦闘は専用クラスへ委譲する。
class Player : public CharacterActor {
public:
    // 構築・設定・フレーム更新
    using ActionState = PlayerActionState;
    using AttackKind = PlayerAttackKind;
    using PlayerRaySegment = ::PlayerRaySegment;
    explicit Player(Game* game);

    // 読み込み済み設定を各担当へ配る。物理・重力探索の設定も更新する。
    void ApplyConfig(const PlayerConfig& config);
    void Initialize() override;
    void ProcessActor() override;

    // deltaTimeは秒。衝突解消、接地、状態遷移、演出の順に更新する。
    void UpdateActor(float deltaTime) override;

    // 描画用の変換とアニメーション
    bool ShouldRenderSolidWhite() const override;
    glm::vec3 GetRenderPosition() const override;
    glm::vec3 GetRenderScale() const override;
    glm::quat GetRenderModelRotationOffset() const override;
    void StartNormalHitReaction();
    void StartStarCollectionCelebration(float durationSeconds);
    void StopStarCollectionCelebration();
    void SetBaseScale(const glm::vec3& scale);
    void RequestNextWeakAttackAnimation();
    void RequestStrongAttackAnimation();
    const std::vector<glm::mat4>* GetSkinningMatrices() const override;

    // 移動・ジャンプ・回避
    void SetHasUsedDodge(bool hasUsedDodge) { mMovement.SetHasUsedDodge(hasUsedDodge); }
    void SetCurrentPlanetNum(int currentPlanetNum) { mMovement.SetCurrentPlanetNum(currentPlanetNum); }
    void SetPlayerNum(int playerNum) { mMovement.SetPlayerNum(playerNum); }
    void SetCameraForwardDirection(const glm::vec3& forwardDirection, const glm::vec3& upDirection)
    {
        mMovement.SetCameraForwardDirection(forwardDirection, upDirection);
    }
    void UnlockMovementDirectionForCameraAutoAlign() { mMovement.UnlockMovementDirectionForCameraAutoAlign(); }
    bool ConsumeCameraAutoAlignCancellationRequest() { return mMovement.ConsumeCameraAutoAlignCancellationRequest(); }
    void SetMoveSpeed(float moveSpeed) { mMovement.SetMoveSpeed(moveSpeed); }
    void SetMaximumStepHeight(float maximumStepHeight) { mMovement.SetMaximumStepHeight(maximumStepHeight); }
    void SetDodgeDuration(float dodgeDuration) { mMovement.SetDodgeDuration(dodgeDuration); }
    void SetDodgeCooldownTime(float dodgeCooldownTime) { mMovement.SetDodgeCooldownTime(dodgeCooldownTime); }
    void SetDodgeDistance(float dodgeDistance) { mMovement.SetDodgeDistance(dodgeDistance); }
    void SetKnockBackSpeed(float knockBackSpeed) { mMovement.SetKnockBackSpeed(knockBackSpeed); }
    void SetJumpHeight(float jumpHeight) { mMovement.SetJumpHeight(jumpHeight); }
    void SetJumpAscentDuration(float duration) { mMovement.SetJumpAscentDuration(duration); }
    void SetJumpFallDuration(float duration) { mMovement.SetJumpFallDuration(duration); }
    void SetJumpApexHoverDurationSeconds(float durationSeconds)
    {
        mMovement.SetJumpApexHoverDurationSeconds(durationSeconds);
    }
    void SetAirWeakAttackPostHoverDurationSeconds(float durationSeconds)
    {
        mMovement.SetAirWeakAttackPostHoverDurationSeconds(durationSeconds);
    }
    void SetAirDodgePostHoverDurationSeconds(float durationSeconds)
    {
        mMovement.SetAirDodgePostHoverDurationSeconds(durationSeconds);
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
    int GetCurrentPlanetNum() const { return mMovement.GetCurrentPlanetNum(); }
    int GetPlayerNum() const { return mMovement.GetPlayerNum(); }
    float GetMoveSpeed() const { return mMovement.GetMoveSpeed(); }
    float GetMaximumStepHeight() const { return mMovement.GetMaximumStepHeight(); }
    float GetDodgeDuration() const { return mMovement.GetDodgeDuration(); }
    float GetDodgeCooldownTime() const { return mMovement.GetDodgeCooldownTime(); }
    float GetDodgeDistance() const { return mMovement.GetDodgeDistance(); }
    float GetKnockBackSpeed() const { return mMovement.GetKnockBackSpeed(); }
    float GetJumpHeight() const { return mMovement.GetJumpHeight(); }
    float GetJumpAscentDuration() const { return mMovement.GetJumpAscentDuration(); }
    float GetJumpFallDuration() const { return mMovement.GetJumpFallDuration(); }
    float GetJumpApexHoverDurationSeconds() const { return mMovement.GetJumpApexHoverDurationSeconds(); }
    float GetAirWeakAttackPostHoverDurationSeconds() const
    {
        return mMovement.GetAirWeakAttackPostHoverDurationSeconds();
    }
    float GetAirDodgePostHoverDurationSeconds() const { return mMovement.GetAirDodgePostHoverDurationSeconds(); }
    float GetAirSlamRiseHeight() const { return mMovement.GetAirSlamRiseHeight(); }
    float GetAirSlamRiseDurationSeconds() const { return mMovement.GetAirSlamRiseDurationSeconds(); }
    float GetAirSlamHoverDurationSeconds() const { return mMovement.GetAirSlamHoverDurationSeconds(); }
    const glm::vec3& GetForwardVec() const { return mMovement.GetForwardVec(); }
    void FaceDirection(const glm::vec3& facingDirection) { mMovement.FaceDirection(*this, facingDirection); }

    // 攻撃・攻撃判定
    bool IsAttacking() const { return mStateMachine.IsAttackingState() || mCombat.IsAttacking(); }
    bool IsSpecialCharging() const { return mCombat.IsSpecialCharging(); }
    bool IsContinuousAttacking() const { return mCombat.IsContinuousAttacking(); }
    std::uint64_t GetResolvedAttackSequence() const { return mCombat.GetResolvedAttackSequence(); }
    bool HasPendingAttackHit() const { return mCombat.HasPendingAttackHit(); }
    void SetAttack(float attack) { mCombat.SetAttack(attack); }
    void SetAttackSpeed(float attackSpeed) { mCombat.SetAttackSpeed(attackSpeed); }
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
    void SetNormalAttackRange(float normalAttackRange) { mCombat.SetNormalAttackRange(normalAttackRange); }
    void SetNormalAttackAngle(float normalAttackAngle) { mCombat.SetNormalAttackAngle(normalAttackAngle); }
    void SetNormalAttack(float normalAttack) { mCombat.SetNormalAttack(normalAttack); }
    void SetWideAttackRange(float wideAttackRange) { mCombat.SetWideAttackRange(wideAttackRange); }
    void SetWideAttackAngle(float wideAttackAngle) { mCombat.SetWideAttackAngle(wideAttackAngle); }
    void SetWideAttack(float wideAttack) { mCombat.SetWideAttack(wideAttack); }
    void SetAirDodgeAttackDamage(float damage) { mCombat.SetAirDodgeAttackDamage(damage); }
    void SetAirDodgeHorizontalHitboxScale(float scale) { mCombat.SetAirDodgeHorizontalHitboxScale(scale); }
    void SetAirDodgeVerticalHitboxScale(float scale) { mCombat.SetAirDodgeVerticalHitboxScale(scale); }
    void SetAirDodgeEnemyPushSpeed(float speed) { mCombat.SetAirDodgeEnemyPushSpeed(speed); }
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
    void SetAttackHitDelay(float attackHitDelay) { mCombat.SetAttackHitDelay(attackHitDelay); }
    bool GetIsStrongAttacked() const { return mCombat.GetIsStrongAttacked(); }
    bool GetCanSpecialAttack() const { return mCombat.GetCanSpecialAttack(); }
    float GetAttack() const { return mCombat.GetAttack(); }
    float CalculateOutgoingAttackDamage(float baseDamage) const;
    float GetAttackMotionTimer() const { return mCombat.GetAttackMotionTimer(); }
    float GetStrongAttackTimer() const { return mCombat.GetStrongAttackTimer(); }
    float GetAttackRange() const { return mCombat.GetAttackRange(); }
    float GetAttackAngle() const { return mCombat.GetAttackAngle(); }
    float GetAttackSpeed() const { return mCombat.GetAttackSpeed(); }
    float GetGroundWeakAttackCooldownSeconds() const { return mCombat.GetGroundWeakAttackCooldownSeconds(); }
    float GetAirWeakAttackCooldownSeconds() const { return mCombat.GetAirWeakAttackCooldownSeconds(); }
    float GetLastAttackCooldown() const { return mCombat.GetLastAttackCooldown(); }
    float GetSpecialAttackCooldown() const { return mCombat.GetSpecialAttackCooldown(); }
    float GetNormalAttackRange() const { return mCombat.GetNormalAttackRange(); }
    float GetNormalAttackAngle() const { return mCombat.GetNormalAttackAngle(); }
    float GetNormalAttack() const { return mCombat.GetNormalAttack(); }
    float GetWideAttackRange() const { return mCombat.GetWideAttackRange(); }
    float GetWideAttackAngle() const { return mCombat.GetWideAttackAngle(); }
    float GetWideAttack() const { return mCombat.GetWideAttack(); }
    float GetAirDodgeAttackDamage() const { return mCombat.GetAirDodgeAttackDamage(); }
    float GetAirDodgeHorizontalHitboxScale() const { return mCombat.GetAirDodgeHorizontalHitboxScale(); }
    float GetAirDodgeVerticalHitboxScale() const { return mCombat.GetAirDodgeVerticalHitboxScale(); }
    float GetAirDodgeEnemyPushSpeed() const { return mCombat.GetAirDodgeEnemyPushSpeed(); }
    float GetAirDodgeEnemyPushDampingPerSecond() const { return mCombat.GetAirDodgeEnemyPushDampingPerSecond(); }
    float GetAirWeakEnemyLiftHeight() const { return mCombat.GetAirWeakEnemyLiftHeight(); }
    float GetAirComboDodgePlayerLiftHeight() const { return mCombat.GetAirComboDodgePlayerLiftHeight(); }
    float GetAirComboDodgeEnemyLiftHeight() const { return mCombat.GetAirComboDodgeEnemyLiftHeight(); }
    float GetStrongAttackRange() const { return mCombat.GetStrongAttackRange(); }
    float GetStrongAttack() const { return mCombat.GetStrongAttack(); }
    float GetStrongAttackSpeed() const { return mCombat.GetStrongAttackSpeed(); }
    float GetChargedAttackRange() const { return mCombat.GetChargedAttackRange(); }
    float GetChargedAttackAngle() const { return mCombat.GetChargedAttackAngle(); }
    float GetChargedAttackDamage() const { return mCombat.GetChargedAttackDamage(); }
    float GetChargedAttackChargeDurationSeconds() const { return mCombat.GetChargedAttackChargeDurationSeconds(); }
    float GetContinuousAttackRange() const { return mCombat.GetContinuousAttackRange(); }
    float GetContinuousAttackAngle() const { return mCombat.GetContinuousAttackAngle(); }
    float GetContinuousAttackDamage() const { return mCombat.GetContinuousAttackDamage(); }
    float GetContinuousAttackIntervalSeconds() const { return mCombat.GetContinuousAttackIntervalSeconds(); }
    float GetContinuousAttackDurationSeconds() const { return mCombat.GetContinuousAttackDurationSeconds(); }
    float GetDefaultStrongAttackTimer() const { return mCombat.GetDefaultStrongAttackTimer(); }
    float GetDefaultAttackMotionTimer() const { return mCombat.GetDefaultAttackMotionTimer(); }
    float GetAttackHitDelay() const { return mCombat.GetAttackHitDelay(); }
    float GetAirSlamEnemyDownwardSpeed() const { return mCombat.GetAirSlamEnemyDownwardSpeed(); }
    float GetAirSlamFullDamageHeight() const { return mCombat.GetAirSlamFullDamageHeight(); }
    float GetAirSlamMinimumDamageRatio() const { return mCombat.GetAirSlamMinimumDamageRatio(); }
    ActionState GetActionState() const { return mStateMachine.GetActionState(); }
    const std::vector<::PlayerRaySegment>& GetRayCasts() const { return mCombat.GetRayCasts(); }
    Enemy* GetAttackDirectionTarget() const { return mStateMachine.GetAttackDirectionTarget(); }

    // HP・無敵・疲労
    // 敵由来と配置物由来のダメージは入口を分け、処理後に分裂プレイヤー間の共有資源を同期する。
    void ApplyDamage(Enemy* enemy, float deltaTime);
    void ApplyDamageFromActor(const glm::vec3& damageSourcePosition, float damage);
    void StartDamageKnockBack(const glm::vec3& damageSourcePosition);
    void RecoverFromFatigue();
    bool IsInvincible() const { return mStatus.IsInvincible(); }
    bool IsAlive() const { return mStatus.IsAlive(); }
    void SetHp(float hp) { mStatus.SetHp(hp); }
    void SetMaxHp(float maxHp) { mStatus.SetMaxHp(maxHp); }
    void SetDefaultDamageTimer(float defaultDamageTimer) { mStatus.SetDefaultDamageTimer(defaultDamageTimer); }
    void SetDefaultInvincibleTimer(float defaultInvincibleTimer)
    {
        mStatus.SetDefaultInvincibleTimer(defaultInvincibleTimer);
    }
    bool GetIsTired() const { return mStatus.GetIsTired(); }
    float GetHp() const { return mStatus.GetHp(); }
    float GetInvincibleTimer() const { return mStatus.GetInvincibleTimer(); }
    float GetMaxHp() const { return mStatus.GetMaxHp(); }
    float GetDefaultDamageTimer() const { return mStatus.GetDefaultDamageTimer(); }
    float GetDefaultInvincibleTimer() const { return mStatus.GetDefaultInvincibleTimer(); }

    // ジュエルの消費・回復
    int GetJewelCount() const { return mJewelGauge.GetCount(); }
    void SetJewelCount(int count) { mJewelGauge.SetCount(count); }
    void AddJewelFromItem();
    float GetSpecialAttackCooldownRemaining() const { return mJewelGauge.GetRecoverTimer(); }

    // 入力とカメラ操作
    // ロック中は速度を0にし、通常の接地・行動状態更新を省く。位置の重なり補正は行う。
    void SetControlLocked(bool locked) { mControlLocked = locked; }
    void SuppressJumpUntilReleased() { mInput.SuppressJumpUntilReleased(); }
    void SetCameraYaw(float cameraYaw) { mInput.SetCameraYaw(cameraYaw); }
    void LockMovementDirectionForCameraAutoAlign()
    {
        mMovement.LockMovementDirectionForCameraAutoAlign(mInput, GetUpVec());
    }
    void SetInputAvailableTimer(float inputAvailableTimer) { mInput.SetInputAvailableTimer(inputAvailableTimer); }
    bool GetIsSpecialAttackPressed() const { return mInput.GetSpecialAttackPressed(); }

    // 手動カメラ操作による、このフレームのYaw回転量（ラジアン）。絶対角ではない。
    float GetCameraYaw() const { return mInput.GetCameraYaw(); }
    float GetMoveForwardInput() const { return mInput.GetMoveForward(); }
    float GetMoveLeftInput() const { return mInput.GetMoveLeft(); }
    float GetInputAvailableTimer() const { return mInput.GetInputAvailableTimer(); }

    // 接地と惑星重力
    void OnAttachedToAdhesivePlatform();

    // 惑星が変わった場合はGameへ変更を通知する。参照先の所有権は受け取らない。
    void SetCurrentPlanet(Planet* currentPlanet);
    void SetRayCastTimer(float rayCastTimer) { mGrounding.SetRayCastTimer(rayCastTimer); }
    void SetVelocity(const glm::vec3& velocity) { mVelocity = velocity; }
    void NotifyJumpStarted() { ++mJumpSequence; }
    std::uint64_t GetJumpSequence() const { return mJumpSequence; }
    float GetRayCastTimer() const { return mGrounding.GetRayCastTimer(); }
    bool WasPlanetGravityFallbackAppliedThisJump() const
    {
        return mPlanetGravityController.WasFallbackAppliedThisJump();
    }
    bool IsEllipseAirborneGravityActive() const
    {
        return mPlanetGravityController.IsEllipseAirborneGravityActive(*this);
    }
    bool ShouldUseEllipseSurfaceGravity() const
    {
        return mPlanetGravityController.ShouldUseEllipseSurfaceGravity(*this);
    }
    glm::vec3 CalculateAirbornePhysicsUpDirection() const
    {
        return mPlanetGravityController.CalculateAirbornePhysicsUpDirection(*this);
    }
    void RestartAirborneGravityFallbackDelay()
    {
        mPlanetGravityController.RestartFallbackDelayForAirborneAction(*this);
    }
    const glm::vec3& GetVelocity() const { return mVelocity; }
    void AddVelocity(const glm::vec3& velocityDelta) { mVelocity += velocityDelta; }
    void SetFacingForwardVec(const glm::vec3& facingForwardVec) { mFacingForwardVec = facingForwardVec; }
    void SetOnGround(bool onGround) { mOnGround = onGround; }
    void SetShouldJudgeLanding(bool shouldJudgeLanding) { mShouldJudgeLanding = shouldJudgeLanding; }
    void ApplyGravityToSelf(float deltaTime) { ApplyGravity(deltaTime); }
    void ApplyGravityToSelf(float deltaTime, float gravityAcceleration)
    {
        ApplyGravity(deltaTime, gravityAcceleration);
    }
    void RefreshFallbackUpVec() { UpdateFallbackUpVec(); }

    // 乗船と降船
    // 到着した船から降り、接地判定を再開して速度・空中行動・重力の状態をリセットする。
    void OnBoatArrived(Boat* boat);
    bool IsWaitingForBoat() const;

    // 発進待ちの船から降りて船の外側へ配置する。待機していない場合はfalse。
    bool CancelWaitingBoatRide();

    // リスポーン・演出用の配置
    void ApplyFallDamageAndRespawn(float damage);

    // 位置・行動状態を戻す。Restartと異なり、HPとジュエルの回復は行わない。
    void RespawnAtRestartPoint();

    // HPを最大値へ、ジュエルを最低2個まで回復して再開位置へ戻す。ジュエルが2個を超えていれば維持する。
    void Restart();

    // 未接地時に接地を試み、成立時は速度・攻撃状態を解除する。接地済みなら何もしない。
    // 接地できず惑星も未設定なら攻撃状態は維持するが、探索中に上方向・速度が補正される場合がある。
    void ForceGroundedForCinematic();

    // 指定したワールド位置を惑星表面へ投影し、配置後の通常着地判定を停止する。
    // 惑星未指定、または基準位置が惑星中心に近い場合はForceGroundedForCinematicへ任せる。
    void ForceGroundedForCinematicAt(Planet* planet, const glm::vec3& surfaceReferencePosition,
                                     const glm::vec3& groundUpDirection);
    void MoveToCurrentPlanetOrigin();
    void DebugMoveToPlanet(Planet* planet, int planetIndex);
    void DebugMoveToPosition(const glm::vec3& worldPosition, Planet* planet, int planetIndex);

    // 会話
    void SetTalkableNPC(NPC* talkableNPC) { mInteraction.SetTalkableNPC(talkableNPC); }
    NPC* GetTalkableNPC() const { return mInteraction.GetTalkableNPC(); }

    // 分裂・合流
    static constexpr float SplitBodyScaleMultiplier = 0.8f;
    void MoveTowardForMergeRecall(const glm::vec3& targetPosition, float deltaTime);
    void SetSplitForm(bool isSplitForm);
    float GetCollisionScaleMultiplier() const override { return mIsSplitForm ? SplitBodyScaleMultiplier : 1.0f; }

private:
    struct LandingUpdateResult {
        bool didLand = false;
        float landingSpeed = 0.0f;
    };

    void ResolvePreUpdateCollisions();
    void UpdateWhileControlLocked(float deltaTime);
    LandingUpdateResult UpdateGrounding(float deltaTime);
    PlayerActionState UpdateActionState(float deltaTime);
    void UpdatePresentation(PlayerActionState previousActionState, const LandingUpdateResult& landing, float deltaTime);

    void UpdateNormalHitReaction(float deltaTime);
    void UpdateStarCollectionCelebration(float deltaTime);
    void RequestEnteredActionAnimation(PlayerActionState previousState, PlayerActionState currentState);

    bool ShouldAcceptLandingSurface(Actor* surfaceActor, const glm::vec3& surfaceNormal) const override;

    bool ShouldUpdateUpVecEveryFrame() const override;

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
    bool mWasMergeRecallWalking = false;
    float mNormalHitReactionElapsedSeconds = -1.0f;
    float mStarCollectionCelebrationElapsedSeconds = -1.0f;
    float mStarCollectionCelebrationDurationSeconds = 0.0f;
    std::uint64_t mJumpSequence = 0;
};
