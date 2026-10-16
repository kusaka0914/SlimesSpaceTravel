#pragma once

#include "actor/CharacterActor.h"
#include "actor/enemy/EnemyStateMachine.h"
#include "actor/enemy/EnemyStatus.h"

#include <glm/glm.hpp>
#include <memory>
#include <string>

class EnemyCombat;
class EnemyBehaviorController;
class EnemyDamageHandler;
class EnemyMovement;
class Game;
class GameWorld;
class Player;
struct EnemyAttackPreview;
struct EnemyConfig;

// 敵Actorの更新と外部操作を統括する。状態遷移・移動・戦闘は専用クラスへ委譲する。
class Enemy : public CharacterActor {
public:
    // 構築・設定・更新
    using LifeState = EnemyStateMachine::LifeState;
    using ActionState = EnemyStateMachine::ActionState;
    Enemy(Game* game);
    ~Enemy() override;
    void UpdateActor(float deltaTime) override;

    // 設定値を反映し、HPとガード値も初期化する。
    void ApplyConfig(const EnemyConfig& config);

    // 描画と被弾リアクション
    bool ShouldRenderSolidWhite() const override;
    glm::quat GetRenderModelRotationOffset() const override;
    glm::vec3 GetRenderScale() const override;
    void StartNormalHitReaction();
    void StartBossHitReaction();

    // 攻撃・被ダメージ・ガード
    // HPへのダメージに加え、攻撃元に応じてソロ分裂時の追跡対象を更新する。
    void ApplyDamage(float damage, Player* player);
    EnemyGuardDamageResult ApplyGuardDamage(float guardDamage, float deltaTime);

    // 生存中の敵に対し、現在のガード区画を破壊できるダメージを適用する。死亡中は空の結果を返す。
    EnemyGuardDamageResult ApplyGuardDamageEnsuringCurrentSegmentBreak(float minimumGuardDamage, float deltaTime);
    EnemyGuardDamageResult BreakGuard(float deltaTime);
    void SetIsStrongAttacked(bool isStrongAttacked) { mStatus.SetIsStrongAttacked(isStrongAttacked); }
    void SetGuardSegmentCount(int segmentCount) { mStatus.SetGuardSegmentCount(segmentCount); }
    void SetAttack(float attack) { mStatus.SetAttack(attack); }
    void SetDefaultAttackMotionTimer(float defaultAttackMotionTimer)
    {
        mStatus.SetDefaultAttackMotionTimer(defaultAttackMotionTimer);
    }
    void SetDefaultStandByAttackTimer(float defaultStandByAttackTimer)
    {
        mStatus.SetDefaultStandByAttackTimer(defaultStandByAttackTimer);
    }
    void SetAttackPreparationRange(float attackPreparationRange)
    {
        mStatus.SetAttackPreparationRange(attackPreparationRange);
    }
    void SetAttackSpeed(float attackSpeed) { mStatus.SetAttackSpeed(attackSpeed); }
    void FlipCanCountered() { mStatus.FlipCanCountered(); }
    bool GetCanCountered() const { return mStatus.GetCanCountered(); }
    float GetCurrentGuard() const { return mStatus.GetCurrentGuard(); }
    float GetMaxGuard() const { return mStatus.GetMaxGuard(); }
    float GetGuardValuePerSegment() const { return mStatus.GetGuardValuePerSegment(); }
    float GetAttack() const { return mStatus.GetAttack(); }
    float GetAttackRange() const { return mStatus.GetAttackRange(); }
    float GetStandByAttackTimer() const { return mStatus.GetStandByAttackTimer(); }
    int GetGuardSegmentCount() const { return mStatus.GetGuardSegmentCount(); }
    float GetAttackPreparationRange() const { return mStatus.GetAttackPreparationRange(); }
    float GetAttackSpeed() const { return mStatus.GetAttackSpeed(); }
    float GetDefaultStandByAttackTimer() const { return mStatus.GetDefaultStandByAttackTimer(); }
    float GetDefaultAttackMotionTimer() const { return mStatus.GetDefaultAttackMotionTimer(); }

    // 移動・接地・空中への外力
    void ApplyAirDodgePush(const glm::vec3& dodgeDirection, float pushSpeed, float pushDampingPerSecond);
    void ApplyAirComboLift(float liftHeight);

    // 生存中かつ空中の場合のみ開始を試み、開始できなければfalseを返す。
    // 開始高度がfullDamageHeight以上ならmaximumDamage、0ならそのminimumDamageRatio倍になる。
    bool StartGravitySlam(Player& player, float downwardSpeed, float maximumDamage, float fullDamageHeight,
                          float minimumDamageRatio, float groundImpactRadius, bool shouldPlayImpactFeedback);
    void SetIsNormalHitKnockBackEnabled(bool isEnabled) { mStatus.SetIsNormalHitKnockBackEnabled(isEnabled); }
    void SetDefaultLaunchedTimer(float defaultLaunchedTimer) { mStatus.SetDefaultLaunchedTimer(defaultLaunchedTimer); }
    void SetLaunchHeight(float launchHeight) { mStatus.SetLaunchHeight(launchHeight); }
    void SetMoveSpeed(float moveSpeed) { mStatus.SetMoveSpeed(moveSpeed); }
    void SetKnockBackSpeed(float knockBackSpeed) { mStatus.SetKnockBackSpeed(knockBackSpeed); }
    bool IsNormalHitKnockBackEnabled() const { return mStatus.IsNormalHitKnockBackEnabled(); }
    float GetMoveSpeed() const { return mStatus.GetMoveSpeed(); }
    float GetKnockBackSpeed() const { return mStatus.GetKnockBackSpeed(); }
    float GetDefaultLaunchedTimer() const { return mStatus.GetDefaultLaunchedTimer(); }
    float GetLaunchedTimer() const { return mStatus.GetLaunchedTimer(); }
    float GetLaunchHeight() const { return mStatus.GetLaunchHeight(); }
    bool IsLaunched() const { return mStateMachine->GetActionState() == ActionState::Launched; }
    bool IsOnGround() const { return mOnGround; }
    void SetOnGroundForEnemy(bool onGround) { mOnGround = onGround; }
    const glm::vec3& GetLastGroundedPosition() const { return mLastGroundedPosition; }
    const glm::vec3& GetLastGroundedUpDirection() const { return mLastGroundedUpDirection; }
    bool HasRecordedGroundedTransform() const { return mHasRecordedGroundedTransform; }
    const glm::vec3& GetVelocity() const { return mVelocity; }
    void SetVelocity(const glm::vec3& velocity) { mVelocity = velocity; }
    void AddVelocity(const glm::vec3& velocity) { mVelocity += velocity; }
    void AddPos(const glm::vec3& delta) { mPos += delta; }
    void SetFacingForwardForEnemy(const glm::vec3& facingForward) { mFacingForwardVec = facingForward; }
    void SetFacingYawForEnemy(float facingYaw) { SetFacingYaw(facingYaw); }
    bool IsSteepGroundForEnemy(const glm::vec3& hitNormal, const glm::vec3& up) const
    {
        return CheckDotAngleSteep(hitNormal, up);
    }

    // 生存状態・HP・ボス設定
    // 通常の被ダメージ・死亡待ちを経ずに、死亡完了処理を実行する。
    void DefeatImmediately();
    void SetIsBoss(bool isBoss) { mStatus.SetIsBoss(isBoss); }
    void SetIsBossEncounter(bool isBossEncounter) { mIsBossEncounter = isBossEncounter; }
    void SetHp(float hp) { mStatus.SetHp(hp); }
    void SetMaxHp(float maxHp) { mStatus.SetMaxHp(maxHp); }
    void SetDetectionRange(float detectionRange) { mStatus.SetDetectionRange(detectionRange); }
    bool GetIsDead() const { return mStateMachine->IsDead(); }
    bool GetIsBoss() const { return mStatus.GetIsBoss(); }
    bool IsBossEncounter() const { return mIsBossEncounter; }
    float GetHp() const { return mStatus.GetHp(); }
    float GetMaxHp() const { return mStatus.GetMaxHp(); }
    float GetDetectionRange() const { return mStatus.GetDetectionRange(); }
    LifeState GetLifeState() const { return mStateMachine->GetLifeState(); }

    // 状態の直接変更用。通常の遷移処理とは区別し、呼び出し側で整合性を保つ。
    void SetLifeState(LifeState lifeState) { mStateMachine->SetLifeState(lifeState); }
    ActionState GetActionState() const { return mStateMachine->GetActionState(); }
    void SetActionState(ActionState actionState) { mStateMachine->SetActionState(actionState); }
    bool IsAlive() const { return mStateMachine->IsAlive(); }
    void SetShouldJudgeLandingForEnemy(bool shouldJudgeLanding) { mShouldJudgeLanding = shouldJudgeLanding; }
    void SetShouldDropJewelOnDeath(bool shouldDrop) { mShouldDropJewelOnDeath = shouldDrop; }
    bool ShouldDropJewelOnDeath() const { return mShouldDropJewelOnDeath; }

    // 行動プロファイルと攻撃予告
    const char* GetCurrentBehaviorActionType() const;
    const std::string& GetBehaviorProfileName() const;

    // 現在の攻撃予告を取得できた場合だけtrueを返し、previewへ結果を書き込む。
    bool GetBehaviorAttackPreview(EnemyAttackPreview& preview) const;
    bool ShouldDrawAttackPreview() const { return mStateMachine->GetActionState() == ActionState::PreparingAttack; }
    bool ShouldDrawAttackImpactFlash() const { return mStateMachine->IsAttackImpactActive(mStatus); }

protected:
    bool ShouldAcceptLandingSurface(Actor* surfaceActor, const glm::vec3& surfaceNormal) const override;

    float ResolveMinimumUpdateIntervalSeconds() const override;
    bool ShouldUpdateUpVecEveryFrame() const override;

private:
    friend class GameWorld;

    enum class HitReactionKind {
        None,
        NormalEnemySpin,
        BossSquashStretch,
    };

    bool CanUseReducedUpdateRate() const;
    Player* ResolvePursuitTarget();
    void RegisterPlayerAttackForAggro(Player* attackingPlayer);
    void ResetSoloSplitAggro();
    void SetShouldUseFullRateUpdate(bool shouldUseFullRateUpdate)
    {
        mShouldUseFullRateUpdate = shouldUseFullRateUpdate;
    }
    void UpdateHitReaction(float deltaTime);

private:
    EnemyStatus mStatus;

    std::unique_ptr<EnemyStateMachine> mStateMachine;
    std::unique_ptr<EnemyMovement> mMovement;
    std::unique_ptr<EnemyCombat> mCombat;
    std::unique_ptr<EnemyDamageHandler> mDamageHandler;
    std::unique_ptr<EnemyBehaviorController> mBehaviorController;
    glm::vec3 mLastGroundedPosition{0.0f};
    glm::vec3 mLastGroundedUpDirection{0.0f, 1.0f, 0.0f};
    bool mHasRecordedGroundedTransform = false;
    bool mIsBossEncounter = false;
    bool mShouldDropJewelOnDeath = false;
    bool mShouldUseFullRateUpdate = true;
    Player* mSoloSplitAggroOverridePlayer = nullptr;
    Player* mSoloSplitAggroHitPlayer = nullptr;
    int mSoloSplitAggroHitCount = 0;
    HitReactionKind mHitReactionKind = HitReactionKind::None;
    float mHitReactionElapsedSeconds = 0.0f;
};
