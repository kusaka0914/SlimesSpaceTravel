#pragma once

#include <glm/glm.hpp>

class Player;
class PlayerCombat;
class PlayerGrounding;
class PlayerInput;
class PlayerStatus;

class PlayerMovement {
public:
    bool CanWalk(const PlayerCombat& combat) const;
    bool CanDodge(const PlayerCombat& combat) const;

    void UpdateWorldVec(Player& player, const PlayerInput& input);
    void UpdateWalk(Player& player, const PlayerInput& input, float deltaTime);
    void ChangeFaceDir(Player& player, const PlayerInput& input);
    void UpdateFacingForwardVec(Player& player);

    void MoveDuringDodging(Player& player, const PlayerCombat& combat, PlayerGrounding& grounding, float deltaTime);
    void MoveDuringAttacking(Player& player, const PlayerCombat& combat, float deltaTime);
    void MoveDuringCharging(Player& player, float deltaTime);
    void MoveDuringStrongAttacking(Player& player, const PlayerCombat& combat, float deltaTime);
    void MoveDuringKnockBack(Player& player, float deltaTime);

    void StartDodging(Player& player, const PlayerInput& input, PlayerStatus& status);
    void StartJumping(Player& player, float deltaTime);

    void StartKnockBack(const glm::vec3& from) { mKnockBackFrom = from; }
    void StartDodgeLock(float seconds) { mDodgeCooldown = seconds; }
    void ReduceDodgeCooldown(float deltaTime);

    void SetIsDodged(bool isDodged) { mIsDodged = isDodged; }
    void SetCurrentPlanetNum(int currentPlanetNum) { mCurrentPlanetNum = currentPlanetNum; }
    void SetPlayerNum(int playerNum) { mPlayerNum = playerNum; }
    void SetMoveSpeed(float moveSpeed) { mMoveSpeed = moveSpeed; }
    void SetChargeMoveSpeed(float chargeMoveSpeed) { mChargeMoveSpeed = chargeMoveSpeed; }
    void SetDodgeDuration(float dodgeDuration) { mDodgeDuration = dodgeDuration; }
    void SetDodgeCooldownTime(float dodgeCooldownTime) { mDodgeCooldownTime = dodgeCooldownTime; }
    void SetDodgeDistance(float dodgeDistance) { mDodgeDistance = dodgeDistance; }
    void SetKnockBackSpeed(float knockBackSpeed) { mKnockBackSpeed = knockBackSpeed; }
    void SetDodgeCooldown(float dodgeCooldown) { mDodgeCooldown = dodgeCooldown; }

    bool GetIsDodged() const { return mIsDodged; }
    int GetCurrentPlanetNum() const { return mCurrentPlanetNum; }
    int GetPlayerNum() const { return mPlayerNum; }
    float GetDodgeTimer() const { return mDodgeTimer; }
    float GetDodgeDuration() const { return mDodgeDuration; }
    float GetDodgeCooldown() const { return mDodgeCooldown; }
    float GetDodgeCooldownTime() const { return mDodgeCooldownTime; }
    float GetDodgeDistance() const { return mDodgeDistance; }
    float GetMoveSpeed() const { return mMoveSpeed; }
    float GetChargeMoveSpeed() const { return mChargeMoveSpeed; }
    float GetKnockBackSpeed() const { return mKnockBackSpeed; }
    const glm::vec3& GetForwardVec() const { return mForwardVec; }

    void ReduceDodgeTimer(float deltaTime) { mDodgeTimer -= deltaTime; }

private:
    bool mIsDodged = true;

    int mCurrentPlanetNum = 0;
    int mPlayerNum = 1;

    float mDodgeTimer = 0.0f;
    float mDodgeDuration = 0.1f;
    float mDodgeCooldown = 0.0f;
    float mDodgeCooldownTime = 0.3f;
    float mDodgeDistance = 3.0f;
    float mDodgeStartHeight = 0.0f;
    float mMoveSpeed = 10.2f;
    float mChargeMoveSpeed = 6.0f;
    float mKnockBackSpeed = 0.0f;

    glm::vec3 mForwardVec = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 mLeftVec = glm::vec3(-1.0f, 0.0f, 0.0f);
    glm::vec3 mKnockBackFrom = glm::vec3(0.0f);
    glm::vec3 mDodgeDir = glm::vec3(0.0f);
};
