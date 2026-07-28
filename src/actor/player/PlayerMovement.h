#pragma once

#include <glm/glm.hpp>

class Player;
class PlayerCombat;
class PlayerGrounding;
class PlayerInput;

class PlayerMovement {
public:
    bool CanDodge(const PlayerCombat& combat) const;

    void UpdateCameraRelativeMovementDirections(Player& player, const PlayerInput& input);
    void SetCameraForwardDirection(const glm::vec3& forwardDirection, const glm::vec3& upDirection);
    void MoveFromInput(Player& player, const PlayerInput& input, float deltaTime);
    void UpdateFacingDirectionFromInput(Player& player, const PlayerInput& input);
    void FaceDirection(Player& player, const glm::vec3& facingDirection);

    void ApplyDodgeMovement(Player& player, const PlayerCombat& combat, PlayerGrounding& grounding, float deltaTime);
    void ApplyAttackMovement(Player& player, const PlayerCombat& combat, float deltaTime);
    void ApplyChargeMovement(Player& player, float deltaTime);
    void ApplyStrongAttackMovement(Player& player, const PlayerCombat& combat, float deltaTime);
    void ApplyKnockBackMovement(Player& player, float deltaTime);
    void ApplyJumpGravity(Player& player, float deltaTime) const;

    void StartDodgeMovement(Player& player, const PlayerInput& input);
    void StartJumpMovement(Player& player, float deltaTime);
    void StartStrongAttackMovementTowards(Player& player, const glm::vec3& targetPosition);
    void UpdateStrongAttackDirectionTowards(Player& player, const glm::vec3& targetPosition);
    void StartAssistStrongAttackMovement(Player& player, const glm::vec3& targetPosition);
    void ClearStrongAttackDirectionOverride();

    void StartKnockBack(const glm::vec3& from) { mKnockBackFrom = from; }
    void StartDodgeLock(float seconds) { mDodgeCooldownRemaining = seconds; }
    void UpdateDodgeCooldown(float deltaTime);

    void SetHasUsedDodge(bool hasUsedDodge) { mHasUsedDodge = hasUsedDodge; }
    void SetCurrentPlanetNum(int currentPlanetNum) { mCurrentPlanetNum = currentPlanetNum; }
    void SetPlayerNum(int playerNum) { mPlayerNum = playerNum; }
    void SetMoveSpeed(float moveSpeed) { mMoveSpeed = moveSpeed; }
    void SetChargeMoveSpeed(float chargeMoveSpeed) { mChargeMoveSpeed = chargeMoveSpeed; }
    void SetDodgeDuration(float dodgeDuration) { mDodgeDuration = dodgeDuration; }
    void SetDodgeCooldownTime(float dodgeCooldownTime) { mDodgeCooldownDuration = dodgeCooldownTime; }
    void SetDodgeDistance(float dodgeDistance) { mDodgeDistance = dodgeDistance; }
    void SetKnockBackSpeed(float knockBackSpeed) { mKnockBackSpeed = knockBackSpeed; }
    void SetJumpHeight(float jumpHeight) { mJumpHeight = jumpHeight; }
    void SetJumpAscentDuration(float duration) { mJumpAscentDuration = duration; }
    void SetJumpFallDuration(float duration) { mJumpFallDuration = duration; }
    void SetDodgeCooldown(float dodgeCooldown) { mDodgeCooldownRemaining = dodgeCooldown; }

    int GetCurrentPlanetNum() const { return mCurrentPlanetNum; }
    int GetPlayerNum() const { return mPlayerNum; }
    float GetDodgeTimer() const { return mDodgeTimer; }
    float GetDodgeDuration() const { return mDodgeDuration; }
    float GetDodgeCooldown() const { return mDodgeCooldownRemaining; }
    float GetDodgeCooldownTime() const { return mDodgeCooldownDuration; }
    float GetDodgeDistance() const { return mDodgeDistance; }
    float GetMoveSpeed() const { return mMoveSpeed; }
    float GetChargeMoveSpeed() const { return mChargeMoveSpeed; }
    float GetKnockBackSpeed() const { return mKnockBackSpeed; }
    float GetJumpHeight() const { return mJumpHeight; }
    float GetJumpAscentDuration() const { return mJumpAscentDuration; }
    float GetJumpFallDuration() const { return mJumpFallDuration; }
    const glm::vec3& GetForwardVec() const { return mForwardVec; }

    void ReduceDodgeTimer(float deltaTime) { mDodgeTimer -= deltaTime; }

private:
    bool mHasUsedDodge = false;
    bool mHasStrongAttackDirectionOverride = false;

    int mCurrentPlanetNum = 0;
    int mPlayerNum = 1;

    float mDodgeTimer = 0.0f;
    float mDodgeDuration = 0.1f;
    float mDodgeCooldownRemaining = 0.0f;
    float mDodgeCooldownDuration = 0.3f;
    float mDodgeDistance = 3.0f;
    float mDodgeStartHeight = 0.0f;
    float mMoveSpeed = 10.2f;
    float mChargeMoveSpeed = 6.0f;
    float mKnockBackSpeed = 0.0f;
    // The old 6.0 m/s jump under 9.8 m/s^2 gravity reached about 1.84 m.
    // Durations independently control the faster rise and slower fall.
    float mJumpHeight = 1.8367347f;
    float mJumpAscentDuration = 0.4f;
    float mJumpFallDuration = 0.85f;

    glm::vec3 mForwardVec = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 mLeftVec = glm::vec3(-1.0f, 0.0f, 0.0f);
    glm::vec3 mKnockBackFrom = glm::vec3(0.0f);
    glm::vec3 mDodgeDir = glm::vec3(0.0f);
    glm::vec3 mStrongAttackDirectionOverride = glm::vec3(0.0f);
};
