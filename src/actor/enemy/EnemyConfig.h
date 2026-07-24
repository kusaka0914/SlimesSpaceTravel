#pragma once

#include <string>
#include <unordered_map>
#include <vector>

struct EnemyBehaviorActionConfig {
    std::string type;
    float weight = 1.0f;
    std::unordered_map<std::string, float> parameters;

    float GetParameter(const std::string& name, float fallback) const
    {
        const auto found = parameters.find(name);
        return found != parameters.end() ? found->second : fallback;
    }
};

struct EnemyBehaviorConfig {
    std::string profileName = "legacyMelee";
    std::vector<EnemyBehaviorActionConfig> actions;
};

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

    EnemyBehaviorConfig behavior;
};
