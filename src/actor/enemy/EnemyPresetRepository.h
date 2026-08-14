#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct EnemyAttackPresetDefinition {
    std::string type = "meleeAttack";
    float selectionProbabilityPercent = 100.0f;

    int chargeCount = 3;
    float repeatDelaySeconds = 1.0f;
    float range = 5.0f;
    float angleDegrees = 120.0f;
    float windUpDurationSeconds = 2.0f;
    float attackDurationSeconds = 0.5f;
};

struct EnemyPresetDefinition {
    std::string id;
    std::string displayName;
    std::string behaviorProfile = "legacyMelee";
    bool isBoss = false;

    float hp = 80.0f;
    std::string modelPath = "enemy.obj";
    float scale = 0.25f;
    float moveSpeed = 1.0f;
    float attack = 5.0f;
    int breakCountMax = 1;
    float radius = 0.75f;
    float attackIntervalSeconds = 0.0f;
    float attackMotionDurationSeconds = 0.0f;
    float attackSpeed = 0.0f;
    float attackPreparationRange = 2.25f;

    float preAttackApproachProbabilityPercent = 0.0f;
    float preAttackApproachSpeed = 12.0f;
    float preAttackApproachStopDistance = 2.5f;
    float postAttackRetreatProbabilityPercent = 0.0f;
    float postAttackRetreatDelaySeconds = 1.0f;
    float postAttackRetreatSpeed = 12.0f;
    float postAttackRetreatDistance = 4.0f;
    float postRetreatRecoverySeconds = 2.0f;
    float postRetreatFollowupApproachProbabilityPercent = 0.0f;
    std::vector<EnemyAttackPresetDefinition> attacks;
};

class EnemyPresetRepository {
public:
    static bool Load(
        const std::string& filePath,
        std::vector<EnemyPresetDefinition>& outPresets,
        std::string& outErrorMessage);

    static bool Save(
        const std::string& filePath,
        const std::string& originalPresetId,
        const EnemyPresetDefinition& preset,
        std::string& outErrorMessage);

    static std::string CreateUniqueId(
        const std::string& sourceId,
        const std::vector<EnemyPresetDefinition>& presets);

    static EnemyAttackPresetDefinition CreateDefaultAttack(
        const std::string& attackType);

    static void NormalizeAttackProbabilities(
        std::vector<EnemyAttackPresetDefinition>& attacks);

    static std::uint64_t GetRevision();
};
