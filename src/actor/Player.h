#pragma once

#include "actor/CharacterActor.h"
#include "actor/player/PlayerCombat.h"
#include "actor/player/PlayerInput.h"
#include "actor/player/PlayerInteraction.h"
#include "actor/player/PlayerModuleContext.h"
#include "actor/player/PlayerMovement.h"
#include "actor/player/PlayerRespawn.h"
#include "actor/player/PlayerStateMachine.h"
#include "actor/player/PlayerStatus.h"
#include "actor/player/PlayerTypes.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

class Game;
class NPC;
class Boat;
class Enemy;

class Player : public CharacterActor {
public:
    using ActionState = PlayerActionState;
    using AttackKind = PlayerAttackKind;
    using PlayerRaySegment = PlayerRaySegment;

    Player(Game* game);

    void ApplyConfig();

    void Initialize() override;
    void ProcessActor() override;
    void UpdateActor(float deltaTime) override;

    void ApplyDamage(Enemy* enemy, float deltaTime);
    void ApplyFallDamageAndRespawn(float damage);
    void OnBoatArrived(Boat* boat);
    void Restart();
    bool IsInvincible() const { return mStatus.IsInvincible(); };
    bool IsAlive() const { return mStatus.IsAlive(); };
    bool IsAttacking() const { return mCombat.IsAttacking(); }

    void SetIsDodged(bool isDodged) { mMovement.isDodged = isDodged; }

    void SetCurrentPlanetNum(int currentPlanetNum) { mMovement.currentPlanetNum = currentPlanetNum; }
    void SetPlayerNum(int playerNum) { mMovement.playerNum = playerNum; }

    void SetCameraYaw(float cameraYaw) { mInput.cameraYaw = cameraYaw; }
    void SetAttack(float attack) { mCombat.attack = attack; }
    void SetMoveSpeed(float moveSpeed) { mMovement.moveSpeed = moveSpeed; }
    void SetAttackSpeed(float attackSpeed) { mCombat.attackSpeed = attackSpeed; }
    void SetChargeMoveSpeed(float chargeMoveSpeed) { mMovement.chargeMoveSpeed = chargeMoveSpeed; }
    void SetHp(float hp) { mStatus.hp = hp; }
    void SetMaxHp(float maxHp) { mStatus.maxHp = maxHp; }
    void SetDefaultDamageTimer(float defaultDamageTimer) { mStatus.defaultDamageTimer = defaultDamageTimer; }
    void SetAttackCooldown(float attackCooldown) { mCombat.attackCooldown = attackCooldown; }
    void SetLastAttackCooldown(float lastAttackCooldown) { mCombat.lastAttackCooldown = lastAttackCooldown; }
    void SetDefaultAttackPressTimer(float defaultAttackPressTimer)
    {
        mCombat.defaultAttackPressTimer = defaultAttackPressTimer;
    }
    void SetSpecialAttackCooldown(float specialAttackCooldown)
    {
        mCombat.specialAttackCooldown = specialAttackCooldown;
    }
    void SetDodgeDuration(float dodgeDuration) { mMovement.dodgeDuration = dodgeDuration; }
    void SetDefaultInvincibleTimer(float defaultInvincibleTimer)
    {
        mStatus.defaultInvincibleTimer = defaultInvincibleTimer;
    }
    void SetDodgeCooldownTime(float dodgeCooldownTime) { mMovement.dodgeCooldownTime = dodgeCooldownTime; }
    void SetDodgeDistance(float dodgeDistance) { mMovement.dodgeDistance = dodgeDistance; }
    void SetNormalAttackRange(float normalAttackRange) { mCombat.normalAttackRange = normalAttackRange; }
    void SetNormalAttackAngle(float normalAttackAngle) { mCombat.normalAttackAngle = normalAttackAngle; }
    void SetNormalAttack(float normalAttack) { mCombat.normalAttack = normalAttack; }
    void SetWideAttackRange(float wideAttackRange) { mCombat.wideAttackRange = wideAttackRange; }
    void SetWideAttackAngle(float wideAttackAngle) { mCombat.wideAttackAngle = wideAttackAngle; }
    void SetWideAttack(float wideAttack) { mCombat.wideAttack = wideAttack; }
    void SetStrongAttackRange(float strongAttackRange) { mCombat.strongAttackRange = strongAttackRange; }
    void SetStrongAttack(float strongAttack) { mCombat.strongAttack = strongAttack; }
    void SetStrongAttackSpeed(float strongAttackSpeed) { mCombat.strongAttackSpeed = strongAttackSpeed; }
    void SetDefaultStrongAttackTimer(float defaultStrongAttackTimer)
    {
        mCombat.defaultStrongAttackTimer = defaultStrongAttackTimer;
    }
    void SetDefaultAttackMotionTimer(float defaultAttackMotionTimer)
    {
        mCombat.defaultAttackMotionTimer = defaultAttackMotionTimer;
    }
    void SetRayCastTimer(float rayCastTimer) { mCombat.rayCastTimer = rayCastTimer; }
    void SetInputAvailableTimer(float inputAvailableTimer) { mInput.inputAvailableTimer = inputAvailableTimer; }
    void SetKnockBackSpeed(float knockBackSpeed) { mMovement.knockBackSpeed = knockBackSpeed; }

    void SetVelocity(const glm::vec3& velocity) { mVelocity = velocity; }

    void SetTalkableNPC(NPC* talkableNPC) { mInteraction.talkableNPC = talkableNPC; }

    bool GetIsStrongAttacked() const { return mCombat.isStrongAttacked; }
    bool GetIsSpecialAttackPressed() const { return mInput.specialAttackPressed; }
    bool GetCanSpecialAttack() const { return mCombat.canSpecialAttack; }
    bool GetIsTired() const { return mStatus.isTired; }

    int GetCurrentPlanetNum() const { return mMovement.currentPlanetNum; }
    int GetJewelCount() const { return mCombat.jewelCount; }
    int GetPlayerNum() const { return mMovement.playerNum; }

    float GetAttack() const { return mCombat.attack; }
    float GetHp() const { return mStatus.hp; }
    float GetAttackMotionTimer() const { return mCombat.attackMotionTimer; }
    float GetStrongAttackTimer() const { return mCombat.strongAttackTimer; }
    float GetInvincibleTimer() const { return mStatus.invincibleTimer; }
    float GetSpecialAttackCooldownRemaining() const { return mCombat.jewelTimer; }
    float GetAttackRange() const { return mCombat.attackRange; }
    float GetAttackAngle() const { return mCombat.attackAngle; }
    float GetRayCastTimer() const { return mCombat.rayCastTimer; }
    float GetMoveSpeed() const { return mMovement.moveSpeed; }
    float GetCameraYaw() const { return mInput.cameraYaw; }

    float GetAttackSpeed() const { return mCombat.attackSpeed; }
    float GetChargeMoveSpeed() const { return mMovement.chargeMoveSpeed; }

    float GetMaxHp() const { return mStatus.maxHp; }

    float GetDefaultDamageTimer() const { return mStatus.defaultDamageTimer; }

    float GetAttackCooldown() const { return mCombat.attackCooldown; }
    float GetLastAttackCooldown() const { return mCombat.lastAttackCooldown; }
    float GetDefaultAttackPressTimer() const { return mCombat.defaultAttackPressTimer; }

    float GetSpecialAttackCooldown() const { return mCombat.specialAttackCooldown; }

    float GetDodgeDuration() const { return mMovement.dodgeDuration; }
    float GetDodgeCooldownTime() const { return mMovement.dodgeCooldownTime; }
    float GetDodgeDistance() const { return mMovement.dodgeDistance; }

    float GetDefaultInvincibleTimer() const { return mStatus.defaultInvincibleTimer; }

    float GetNormalAttackRange() const { return mCombat.normalAttackRange; }
    float GetNormalAttackAngle() const { return mCombat.normalAttackAngle; }
    float GetNormalAttack() const { return mCombat.normalAttack; }

    float GetWideAttackRange() const { return mCombat.wideAttackRange; }
    float GetWideAttackAngle() const { return mCombat.wideAttackAngle; }
    float GetWideAttack() const { return mCombat.wideAttack; }

    float GetStrongAttackRange() const { return mCombat.strongAttackRange; }
    float GetStrongAttack() const { return mCombat.strongAttack; }
    float GetStrongAttackSpeed() const { return mCombat.strongAttackSpeed; }

    float GetDefaultStrongAttackTimer() const { return mCombat.defaultStrongAttackTimer; }
    float GetDefaultAttackMotionTimer() const { return mCombat.defaultAttackMotionTimer; }

    float GetInputAvailableTimer() const { return mInput.inputAvailableTimer; }
    float GetKnockBackSpeed() const { return mMovement.knockBackSpeed; }

    ActionState GetActionState() const { return mCombat.actionState; }

    const glm::vec3& GetForwardVec() const { return mMovement.forwardVec; }
    const std::vector<PlayerRaySegment>& GetRayCasts() const { return mCombat.rayCasts; }

    NPC* GetTalkableNPC() const { return mInteraction.talkableNPC; }

    glm::vec3& ModulePos() { return mPos; }
    glm::vec3& ModuleVelocity() { return mVelocity; }
    glm::vec3& ModuleUpVec() { return mUpVec; }
    glm::vec3& ModuleFacingForwardVec() { return mFacingForwardVec; }
    glm::vec3& ModuleBaseScale() { return mBaseScale; }
    bool& ModuleOnGround() { return mOnGround; }
    bool& ModuleShouldJudgeLanding() { return mShouldJudgeLanding; }
    float& ModuleRadius() { return mRadius; }
    float& ModuleFacingYaw() { return mFacingYaw; }
    bool& ModuleIsActive() { return mIsActive; }
    Planet*& ModuleCurrentPlanet() { return mCurrentPlanet; }
    void ModuleApplyGravity(float deltaTime) { ApplyGravity(deltaTime); }
    void ModuleUpdateDirectionVectors() { UpdateDirectionVectors(); }
    void ModuleUpdateFallbackUpVec() { UpdateFallbackUpVec(); }

private:
    PlayerModuleContext MakeModuleContext();

    void OnLanded() override;
    void OnUpVecUpdateFailed() override;
    void OnCastSucceeded() override;

private:
    PlayerInput mInput;
    PlayerMovement mMovement;
    PlayerCombat mCombat;
    PlayerStatus mStatus;
    PlayerRespawn mRespawn;
    PlayerInteraction mInteraction;
    PlayerStateMachine mStateMachine;
};