#pragma once

#include <string>

struct EnemyConfig {
    bool isBoss = false;

    float hp = 80.0f;
    float scale = 0.25f;
    float moveSpeed = 1.0f;
    float attack = 5.0f;
    float radius = 0.75f;

    int breakCountMax = 1;

    std::string modelPath;

    float defaultStandByAttackTimer = 0.0f;
    float defaultAttackMotionTimer = 0.0f;
    float attackSpeed = 0.0f;

    float knockBackSpeed = 0.0f;
    float defaultLaunchedTimer = 0.0f;
    float detectionRange = 0.0f;
};
