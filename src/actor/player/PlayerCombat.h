#pragma once

#include "actor/player/PlayerTypes.h"
#include <vector>

class Enemy;
struct PlayerModuleContext;

class PlayerCombat {
public:
    PlayerActionState actionState = PlayerActionState::Idle;
    PlayerAttackKind attackKind = PlayerAttackKind::Normal;

    bool isStrongAttackHit = false;
    bool isStrongAttacked = false;
    bool isCharged = false;
    bool canSpecialAttack = false;
    bool isAirAttacking = false;

    int attackComboIndex = 0;
    int jewelCount = 2;

    float attackStartHeight = 0.0f;
    float attack = 10.0f;
    float attackSpeed = 5.0f;
    float attackCooldownRemaining = 0.0f;
    float attackCooldown = 0.3f;
    float lastAttackCooldown = 1.0f;
    float attackMoveLockRemaining = -1.0f;
    float attackDodgeLockRemaining = 0.0f;
    float attackMotionTimer = -1.0f;
    float defaultAttackMotionTimer = 0.3f;
    float airAttackFloatingTimer = -1.0f;
    float jewelTimer = -1.0f;
    float specialAttackCooldown = 30.0f;
    float attackPressTimer = -1.0f;
    float defaultAttackPressTimer = 0.0f;
    float strongAttackTimer = -1.0f;
    float defaultStrongAttackTimer = 0.06f;
    float comboKeepTimer = -1.0f;
    float attackRange = 2.8f;
    float attackAngle = 0.8f;
    float normalAttackRange = 2.8f;
    float normalAttackAngle = 0.8f;
    float normalAttack = 10.0f;
    float wideAttackRange = 2.8f;
    float wideAttackAngle = -0.2f;
    float wideAttack = 5.0f;
    float strongAttackRange = 6.0f;
    float strongAttack = 50.0f;
    float strongAttackSpeed = 100.0f;
    float rayCastTimer = 0.5f;
    float specialChargingTimer = -1.0f;
    float continuousAttackingTimer = -1.0f;
    float continuousAttackingCooldown = -1.0f;

    std::vector<PlayerRaySegment> rayCasts;

    bool IsAttacking() const;

    void StartAttacking(PlayerModuleContext& context, float deltaTime);
    void StartCharging(PlayerModuleContext& context, float deltaTime);
    void StartStrongAttacking(PlayerModuleContext& context, float deltaTime);
    void FinishCharging(PlayerModuleContext& context);
    void FinishSpecialAttackCharging(PlayerModuleContext& context);
    void Attack(PlayerModuleContext& context, float deltaTime);
    void WideAttack(PlayerModuleContext& context, float deltaTime);
    void StrongAttack(PlayerModuleContext& context, float deltaTime);
    void SpecialAttack(PlayerModuleContext& context, float deltaTime);
    void StartAfterAttackReaction(PlayerModuleContext& context);
    std::vector<Enemy*> FindHitEnemies(PlayerModuleContext& context);
    bool IsEnemyHitByAttack(PlayerModuleContext& context, float dist, float dot, float effectiveRange);
    void StartSpecialAttackCharging(PlayerModuleContext& context);
    void StartContinuousAttacking(PlayerModuleContext& context);
};
