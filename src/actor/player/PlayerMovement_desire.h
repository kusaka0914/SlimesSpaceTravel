#pragma once

#include <glm/glm.hpp>

class Player;
class PlayerCombat;
class PlayerGrounding;
class PlayerInput;

class PlayerMovement {
public:
    bool IsDodgeAvailable() const;

    void UpdateCameraRelativeMovementDirections(Player& player, const PlayerInput& input);

    void UpdateFacingDirectionFromInput(Player& player, const PlayerInput& input);

    void MoveFromInput(Player& player, const PlayerInput& input, float deltaTime);

    void ApplyDodgeMovement(Player& player, const PlayerCombat& combat, PlayerGrounding& grounding, float deltaTime);

    void ApplyAttackMovement(Player& player, const PlayerCombat& combat, float deltaTime);

    void ApplyChargingMovement(Player& player, float deltaTime);

    void ApplyStrongAttackMovement(Player& player, const PlayerCombat& combat, float deltaTime);

    void ApplyKnockBackMovement(Player& player, float deltaTime);

    void StartDodgeMovement(Player& player, const PlayerInput& input);

    void StartJump(Player& player, float deltaTime);

    void StartKnockBack(const glm::vec3& origin) { mKnockBackOrigin = origin; }

    void UpdateDodgeTimer(float deltaTime);
    void UpdateDodgeCooldown(float deltaTime);

    void SetHasUsedDodge(bool hasUsedDodge) { mHasUsedDodge = hasUsedDodge; }

    void SetMoveSpeed(float moveSpeed) { mMoveSpeed = moveSpeed; }

    void SetJumpSpeed(float jumpSpeed) { mJumpSpeed = jumpSpeed; }

    void SetChargingMoveSpeed(float chargingMoveSpeed) { mChargingMoveSpeed = chargingMoveSpeed; }

    void SetDodgeDuration(float dodgeDuration) { mDodgeDuration = dodgeDuration; }

    void SetDodgeCooldownDuration(float duration) { mDodgeCooldownDuration = duration; }

    void SetDodgeDistance(float dodgeDistance) { mDodgeDistance = dodgeDistance; }

    void SetKnockBackSpeed(float knockBackSpeed) { mKnockBackSpeed = knockBackSpeed; }

    float GetDodgeTimer() const { return mDodgeTimer; }

    float GetDodgeDuration() const { return mDodgeDuration; }

    float GetDodgeCooldownRemaining() const { return mDodgeCooldownRemaining; }

    float GetDodgeCooldownDuration() const { return mDodgeCooldownDuration; }

    const glm::vec3& GetMovementForwardDirection() const { return mForwardDirection; }

private:
    bool mHasUsedDodge = false;

    float mDodgeTimer = 0.0f;
    float mDodgeDuration = 0.1f;
    float mDodgeCooldownRemaining = 0.0f;
    float mDodgeCooldownDuration = 0.3f;
    float mDodgeDistance = 3.0f;

    float mMoveSpeed = 10.2f;
    float mChargingMoveSpeed = 6.0f;
    float mKnockBackSpeed = 0.0f;
    float mJumpSpeed = 6.0f;

    glm::vec3 mForwardDirection{0.0f, 0.0f, 1.0f};
    glm::vec3 mLeftDirection{-1.0f, 0.0f, 0.0f};
    glm::vec3 mKnockBackOrigin{0.0f};
    glm::vec3 mDodgeDirection{0.0f};
};