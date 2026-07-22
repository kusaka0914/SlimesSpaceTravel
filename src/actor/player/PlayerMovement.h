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
    void MoveFromInput(Player& player, const PlayerInput& input, float deltaTime);
    void UpdateFacingDirectionFromInput(Player& player, const PlayerInput& input);

    void ApplyDodgeMovement(Player& player, const PlayerCombat& combat, PlayerGrounding& grounding, float deltaTime);
    void ApplyAttackMovement(Player& player, const PlayerCombat& combat, float deltaTime);
    void ApplyChargeMovement(Player& player, float deltaTime);
    void ApplyStrongAttackMovement(Player& player, const PlayerCombat& combat, float deltaTime);
    void ApplyKnockBackMovement(Player& player, float deltaTime);

    void StartDodgeMovement(Player& player, const PlayerInput& input);
    void StartJumpMovement(Player& player, float deltaTime);

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
    const glm::vec3& GetForwardVec() const { return mForwardVec; }

    void ReduceDodgeTimer(float deltaTime) { mDodgeTimer -= deltaTime; }

private:
    bool mHasUsedDodge = false;

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
    float mJumpSpeed = 6.0f;

    glm::vec3 mForwardVec = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 mLeftVec = glm::vec3(-1.0f, 0.0f, 0.0f);
    glm::vec3 mKnockBackFrom = glm::vec3(0.0f);
    glm::vec3 mDodgeDir = glm::vec3(0.0f);
};
