#pragma once

#include "actor/player/PlayerAnimationDefinition.h"

#include <string>

struct PlayerConfig {
    float hp = 100.0f;
    float scale = 0.25f;

    float attackSpeed = 5.0f;
    float attack = 10.0f;
    float moveSpeed = 10.2f;
    float jumpHeight = 1.8367347f;
    float jumpAscentDuration = 0.4f;
    float jumpFallDuration = 0.85f;
    float groundNormalRayLength = 5.0f;
    float collisionWidth = 1.6f;
    float collisionHeight = 0.8f;
    float collisionDepth = 0.8f;
    float collisionCenterHeight = 0.45f;

    float dodgeDuration = 0.1f;
    float dodgeCooldownTime = 0.3f;
    float dodgeDistance = 3.0f;

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

    float specialAttackCooldown = 30.0f;
    float defaultInvincibleTimer = 2.0f;
    float defaultDamageTimer = 1.0f;
    float defaultAttackMotionTimer = 0.3f;
    float attackHitDelay = 0.5f;
    float attackCooldown = 0.3f;
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
