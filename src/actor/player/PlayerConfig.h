#pragma once

#include "actor/player/PlayerAnimationDefinition.h"

#include <string>

struct PlayerConfig {
    float hp = 100.0f;
    float scale = 0.25f;

    float attackSpeed = 5.0f;
    float attack = 10.0f;
    float moveSpeed = 10.2f;
    float maximumStepHeight = 0.3f;
    float jumpHeight = 1.8367347f;
    float jumpAscentDuration = 0.4f;
    float jumpFallDuration = 0.85f;
    float jumpApexHoverDurationSeconds = 0.5f;
    float airWeakAttackPostHoverDurationSeconds = 0.2f;
    float airDodgePostHoverDurationSeconds = 0.2f;
    float groundNormalRayLength = 5.0f;
    float overheadGravityRayLength = 15.0f;
    float collisionWidth = 1.6f;
    float collisionHeight = 0.8f;
    float collisionDepth = 0.8f;
    float collisionCenterHeight = 0.45f;

    float dodgeDuration = 0.1f;
    float dodgeCooldownTime = 0.3f;
    float dodgeDistance = 3.0f;
    float airDodgeAttackDamage = 5.0f;
    float airDodgeHorizontalHitboxScale = 1.0f;
    float airDodgeVerticalHitboxScale = 2.0f;
    float airDodgeEnemyPushSpeed = 6.0f;
    float airDodgeEnemyPushDampingPerSecond = 8.0f;
    float airWeakEnemyLiftHeight = 0.45f;
    float airComboDodgePlayerLiftHeight = 0.8f;
    float airComboDodgeEnemyLiftHeight = 0.8f;

    float normalAttackRange = 2.8f;
    float normalAttackAngle = 0.8f;
    float normalAttack = 10.0f;

    float wideAttackRange = 2.8f;
    float wideAttackAngle = -0.2f;
    float wideAttack = 5.0f;

    float strongAttackRange = 6.0f;
    float strongAttack = 50.0f;
    float strongAttackSpeed = 100.0f;
    float airSlamRiseHeight = 1.0f;
    float airSlamRiseDurationSeconds = 0.5f;
    float airSlamHoverDurationSeconds = 0.3f;
    float airSlamEnemyDownwardSpeed = 18.0f;
    float airSlamFullDamageHeight = 5.0f;
    float airSlamMinimumDamageRatio = 0.3f;

    float chargedAttackRange = 2.6f;
    float chargedAttackAngle = 6.283f;
    float chargedAttackDamage = 50.0f;
    float chargedAttackChargeDurationSeconds = 3.0f;

    float continuousAttackRange = 2.0f;
    float continuousAttackAngle = 6.283f;
    float continuousAttackDamage = 2.5f;
    float continuousAttackIntervalSeconds = 0.25f;
    float continuousAttackDurationSeconds = 6.0f;

    float specialAttackCooldown = 30.0f;
    float defaultInvincibleTimer = 2.0f;
    float defaultDamageTimer = 1.0f;
    float defaultAttackMotionTimer = 0.3f;
    float attackHitDelay = 0.5f;
    float groundWeakAttackCooldownSeconds = 0.0f;
    float airWeakAttackCooldownSeconds = 0.19f;
    float lastAttackCooldown = 1.0f;
    float defaultStrongAttackTimer = 0.06f;
    float knockBackSpeed = 0.0f;

    std::string modelPath = "player.obj";

    PlayerAnimationDefinitions animations = {
        {"idle", {"Idle", PlayerAnimationPlaybackMode::BaseLoop}},
        {"walk", {"Walk", PlayerAnimationPlaybackMode::BaseLoop}},
        {"attack", {"Attack", PlayerAnimationPlaybackMode::OneShot}},
    };
};
