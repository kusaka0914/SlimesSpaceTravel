#pragma once

#include <glm/glm.hpp>
#include <optional>

class Player;
class PlayerCombat;
class PlayerGrounding;
class PlayerInput;
class Planet;
class PhysicsSystem;
class MathUtils;

// カメラ基準の移動、ジャンプ・回避・攻撃移動を計算し、衝突解決後の位置と速度をPlayerへ反映する。
// 位置・方向はワールド空間。deltaTimeは秒。
class PlayerMovement {
public:
    PlayerMovement(PhysicsSystem& physicsSystem, MathUtils& mathUtils);

    bool CanDodge() const;

    // 入力移動とカメラ用の横移動補正のフレーム記録をクリアする。
    void ResetFrameMovement()
    {
        mInputMovementDirection = glm::vec3(0);
        mCameraLateralCorrection = glm::vec3(0);
    }

    void UpdateCameraRelativeMovementDirections(Player& player, const PlayerInput& input);
    void SetCameraForwardDirection(const glm::vec3& forwardDirection, const glm::vec3& upDirection);

    // 通常カメラの位置から移動基準を更新する。nulloptなら従来の方向基準だけを使う。
    void SetNormalCameraPosition(std::optional<glm::vec3> position) { mNormalCameraPosition = position; }

    // 移動方向を接平面上に固定する。有効な入力方向・上方向がなければ何もしない。
    // releaseOnNeutral=trueなら補間後も保持し、入力を離すか明確に方向変更したときに解除する。
    void LockMovementDirectionForCameraAutoAlign(const PlayerInput& input, const glm::vec3& upDirection,
                                                 bool releaseOnNeutral = false);
    bool IsCameraZoneDirectionHeld() const
    {
        return mIsCameraAutoAlignMovementDirectionLocked && mReleaseCameraDirectionOnNeutral;
    }

    // 通常の入力移動で実際に進んだワールド方向。停止・外力だけの移動ではゼロ。
    const glm::vec3& GetInputMovementDirection() const { return mInputMovementDirection; }

    // 実際に進めた横移動に対応する接線補正。カメラの前後追従からこの分だけを除外する。
    const glm::vec3& GetCameraLateralCorrection() const { return mCameraLateralCorrection; }

    // カメラ追従で一度だけ使う補正を取り出す。次の消費までに再記録がなければ0を返す。
    glm::vec3 ConsumeCameraLateralCorrection()
    {
        const auto correction = mCameraLateralCorrection;
        mCameraLateralCorrection = glm::vec3(0);
        return correction;
    }

    void UnlockMovementDirectionForCameraAutoAlign();

    // 入力方向の明確な変更による自動回転の解除要求を返し、要求を消費する。
    bool ConsumeCameraAutoAlignCancellationRequest();

    // 入力から移動し、衝突と段差を解決する。接着床に固定中は移動しない。
    // speedMultiplierは入力移動だけに掛ける。衝突判定・重力の時間は変えない。
    void MoveFromInput(Player& player, const PlayerInput& input, float deltaTime, float speedMultiplier = 1.0f);

    // ワールド位置へ接平面上を移動し、実際に変位が生じたらtrue。空中・床への固定中はfalse。
    bool MoveTowardPosition(Player& player, const glm::vec3& targetPosition, float deltaTime);
    void UpdateFacingDirectionFromInput(Player& player, const PlayerInput& input);
    void FaceDirection(Player& player, const glm::vec3& facingDirection);

    void ApplyDodgeMovement(Player& player, const PlayerCombat& combat, PlayerGrounding& grounding, float deltaTime);
    void ApplyAttackMovement(Player& player, const PlayerCombat& combat, float deltaTime);
    void ApplyStrongAttackMovement(Player& player, const PlayerCombat& combat, float deltaTime);
    void ApplyKnockBackMovement(Player& player, float deltaTime);

    // 上昇・下降で異なる重力と頂点の滞空を適用し、衝突を解決して位置・速度を更新する。
    void ApplyJumpGravity(Player& player, float deltaTime);
    void StopAirborneVerticalMovement(Player& player) const;
    void ApplyJumpGravityAndInputMovement(Player& player, const PlayerInput& input, float deltaTime,
                                          float speedMultiplier = 1.0f);

    // 上昇・滞空・落下の段階を進める。接地済み、または今回の落下で着地した場合にtrue。
    bool UpdateAirSlamMovement(Player& player, const PlayerCombat& combat, float deltaTime);

    // 回避方向と時間を設定し、速度を0にする。回避可能かの確認は呼び出し側で行う。
    void StartDodgeMovement(Player& player, const PlayerInput& input);

    // ワールド位置へ向かう直線回避を設定する。方向を決められなければfalse。
    bool StartDodgeMovementTowards(Player& player, const glm::vec3& targetPosition);

    // 床との固定を解除し、上昇速度とこのフレームの移動を反映する。通常の着地判定は停止する。
    void StartJumpMovement(Player& player, float deltaTime);
    void ResetEllipseAirborneSurfaceTravel();
    void CancelJumpApexHover();
    void StartAirborneActionHover(float durationSeconds);

    // 滞空中は物理上方向の速度成分を除去してtrue。接地中・時間切れなら解除してfalse。
    bool UpdateAirborneActionHover(Player& player, float deltaTime);
    void CancelAirborneActionHover();
    void StartAirSlamMovement(Player& player);
    void StartAirSlamEnemyFallWatch(Player& player);

    // ワールド位置への突進方向を設定し、速度を0にして接地と通常の着地判定を解除する。
    void StartStrongAttackMovementTowards(Player& player, const glm::vec3& targetPosition);
    void UpdateStrongAttackDirectionTowards(Player& player, const glm::vec3& targetPosition);
    void ClearStrongAttackDirectionOverride();

    void StartKnockBack(Player& player, const glm::vec3& damageSourcePosition);
    void StartDodgeLock(float seconds) { mDodgeCooldownRemaining = seconds; }
    void RestoreAirDodge()
    {
        mHasUsedDodge = false;
        mDodgeCooldownRemaining = 0.0f;
    }
    void UpdateDodgeCooldown(float deltaTime);

    void SetHasUsedDodge(bool hasUsedDodge) { mHasUsedDodge = hasUsedDodge; }
    void SetCurrentPlanetNum(int currentPlanetNum) { mCurrentPlanetNum = currentPlanetNum; }
    void SetPlayerNum(int playerNum) { mPlayerNum = playerNum; }
    void SetMoveSpeed(float moveSpeed) { mMoveSpeed = moveSpeed; }
    void SetMaximumStepHeight(float maximumStepHeight)
    {
        mMaximumStepHeight = maximumStepHeight > 0.0f ? maximumStepHeight : 0.0f;
    }
    void SetDodgeDuration(float dodgeDuration) { mDodgeDuration = dodgeDuration; }
    void SetDodgeCooldownTime(float dodgeCooldownTime) { mDodgeCooldownDuration = dodgeCooldownTime; }
    void SetDodgeDistance(float dodgeDistance) { mDodgeDistance = dodgeDistance; }
    void SetKnockBackSpeed(float knockBackSpeed) { mKnockBackSpeed = knockBackSpeed; }
    void SetJumpHeight(float jumpHeight) { mJumpHeight = jumpHeight; }
    void SetJumpAscentDuration(float duration) { mJumpAscentDuration = duration; }
    void SetJumpFallDuration(float duration) { mJumpFallDuration = duration; }
    void SetJumpApexHoverDurationSeconds(float durationSeconds)
    {
        mJumpApexHoverDurationSeconds = durationSeconds > 0.0f ? durationSeconds : 0.0f;
    }
    void SetAirWeakAttackPostHoverDurationSeconds(float durationSeconds)
    {
        mAirWeakAttackPostHoverDurationSeconds = durationSeconds > 0.0f ? durationSeconds : 0.0f;
    }
    void SetAirDodgePostHoverDurationSeconds(float durationSeconds)
    {
        mAirDodgePostHoverDurationSeconds = durationSeconds > 0.0f ? durationSeconds : 0.0f;
    }
    void SetAirSlamRiseHeight(float riseHeight) { mAirSlamRiseHeight = riseHeight; }
    void SetAirSlamRiseDurationSeconds(float durationSeconds) { mAirSlamRiseDurationSeconds = durationSeconds; }
    void SetAirSlamHoverDurationSeconds(float durationSeconds) { mAirSlamHoverDurationSeconds = durationSeconds; }
    void SetDodgeCooldown(float dodgeCooldown) { mDodgeCooldownRemaining = dodgeCooldown; }

    int GetCurrentPlanetNum() const { return mCurrentPlanetNum; }
    int GetPlayerNum() const { return mPlayerNum; }
    float GetDodgeTimer() const { return mDodgeTimer; }
    float GetDodgeDuration() const { return mDodgeDuration; }
    float GetDodgeCooldown() const { return mDodgeCooldownRemaining; }
    float GetDodgeCooldownTime() const { return mDodgeCooldownDuration; }
    float GetDodgeDistance() const { return mDodgeDistance; }
    float GetMoveSpeed() const { return mMoveSpeed; }
    float GetMaximumStepHeight() const { return mMaximumStepHeight; }
    float GetKnockBackSpeed() const { return mKnockBackSpeed; }
    float GetJumpHeight() const { return mJumpHeight; }
    float GetJumpAscentDuration() const { return mJumpAscentDuration; }
    float GetJumpFallDuration() const { return mJumpFallDuration; }
    float GetJumpApexHoverDurationSeconds() const { return mJumpApexHoverDurationSeconds; }
    float GetAirWeakAttackPostHoverDurationSeconds() const { return mAirWeakAttackPostHoverDurationSeconds; }
    float GetAirDodgePostHoverDurationSeconds() const { return mAirDodgePostHoverDurationSeconds; }
    float GetAirSlamRiseHeight() const { return mAirSlamRiseHeight; }
    float GetAirSlamRiseDurationSeconds() const { return mAirSlamRiseDurationSeconds; }
    float GetAirSlamHoverDurationSeconds() const { return mAirSlamHoverDurationSeconds; }
    bool IsAirSlamFalling() const { return mAirSlamMovementPhase == AirSlamMovementPhase::Falling; }
    const glm::vec3& GetForwardVec() const { return mForwardVec; }
    const glm::vec3& GetDodgeDirection() const { return mDodgeDir; }

    void ReduceDodgeTimer(float deltaTime) { mDodgeTimer -= deltaTime; }

private:
    PhysicsSystem& mPhysicsSystem;
    MathUtils& mMathUtils;

    enum class DodgeTrajectory {
        Straight,
        FollowEllipseSurface,
    };

    enum class AirSlamMovementPhase {
        Rising,
        Hovering,
        WatchingEnemyFall,
        Falling,
    };

    void StartDodgeMovementInDirection(Player& player, const glm::vec3& dodgeDirection, DodgeTrajectory trajectory);
    glm::vec3 CalculateEllipseDodgeMovementDelta(Player& player, const Planet& planet, float dodgeSpeed,
                                                 float deltaTime);
    float CalculateAirborneGravityAcceleration(float verticalSpeed) const;

    struct InputMovementDelta {
        glm::vec3 displacement{0};
        glm::vec3 lateralMovement{0};
        glm::vec3 lateralCorrection{0};
    };

    InputMovementDelta CalculateInputMovementDelta(const Player& player, const PlayerInput& input, float deltaTime,
                                                   float speedMultiplier) const;
    void RecordCameraLateralCorrection(const InputMovementDelta& movement, const glm::vec3& applied);
    bool ApplyWalkingMovement(Player& player, const glm::vec3& movementDelta);
    void ApplyJumpGravityMovement(Player& player, const glm::vec3& inputMovementDelta, float deltaTime);
    void RecordEllipseAirborneStartSurfaceNormal(const Player& player);
    glm::vec3 ClampEllipseAirborneMovementToSurfaceTravelLimit(const Planet& planet, const glm::vec3& currentPosition,
                                                               const glm::vec3& physicsUpDirection,
                                                               const glm::vec3& requestedMovement,
                                                               bool& wasMovementClamped) const;

    bool mHasUsedDodge = false;
    bool mHasStrongAttackDirectionOverride = false;
    bool mHasEllipseAirborneStartSurfaceNormal = false;
    bool mCanStartJumpApexHover = false;
    bool mIsCameraAutoAlignMovementDirectionLocked = false;
    bool mReleaseCameraDirectionOnNeutral = false;
    glm::vec3 mInputMovementDirection{0.0f};
    glm::vec3 mCameraLateralCorrection{0.0f};
    std::optional<glm::vec3> mNormalCameraPosition;
    bool mHasCameraAutoAlignCancellationRequest = false;

    int mCurrentPlanetNum = 0;
    int mPlayerNum = 1;

    float mDodgeTimer = 0.0f;
    float mDodgeDuration = 0.1f;
    float mDodgeCooldownRemaining = 0.0f;
    float mDodgeCooldownDuration = 0.3f;
    float mDodgeDistance = 3.0f;
    float mAirSlamRiseHeight = 1.0f;
    float mAirSlamRiseDurationSeconds = 0.5f;
    float mAirSlamHoverDurationSeconds = 0.3f;
    float mAirSlamPhaseRemainingSeconds = 0.0f;
    AirSlamMovementPhase mAirSlamMovementPhase = AirSlamMovementPhase::Falling;
    bool mHasStartedAirSlamEnemyFallWatch = false;
    float mMoveSpeed = 10.2f;
    float mMaximumStepHeight = 0.3f;
    float mKnockBackSpeed = 0.0f;

    float mJumpHeight = 1.8367347f;
    float mJumpAscentDuration = 0.4f;
    float mJumpFallDuration = 0.85f;
    float mJumpApexHoverDurationSeconds = 0.5f;
    float mJumpApexHoverRemainingSeconds = 0.0f;
    float mAirWeakAttackPostHoverDurationSeconds = 0.2f;
    float mAirDodgePostHoverDurationSeconds = 0.2f;
    float mAirborneActionHoverRemainingSeconds = 0.0f;

    glm::vec3 mForwardVec = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 mLeftVec = glm::vec3(-1.0f, 0.0f, 0.0f);
    glm::vec3 mCameraAutoAlignMovementDirection = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec2 mCameraAutoAlignStartInputDirection = glm::vec2(0.0f, 1.0f);
    glm::vec3 mKnockBackDirection = glm::vec3(0.0f);
    glm::vec3 mEllipseAirborneStartSurfaceNormal = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 mDodgeDir = glm::vec3(0.0f);
    DodgeTrajectory mDodgeTrajectory = DodgeTrajectory::Straight;
    glm::vec3 mStrongAttackDirectionOverride = glm::vec3(0.0f);
};
