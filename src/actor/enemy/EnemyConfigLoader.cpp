#include "actor/enemy/EnemyConfigLoader.h"

#include <yaml-cpp/yaml.h>

#include <utility>

namespace {
float ReadFloat(const YAML::Node& node, const char* key, float defaultValue)
{
    return node[key] ? node[key].as<float>() : defaultValue;
}

int ReadInt(const YAML::Node& node, const char* key, int defaultValue)
{
    return node[key] ? node[key].as<int>() : defaultValue;
}

bool ReadBool(const YAML::Node& node, const char* key, bool defaultValue)
{
    return node[key] ? node[key].as<bool>() : defaultValue;
}

std::string ReadString(const YAML::Node& node, const char* key, const std::string& defaultValue)
{
    return node[key] ? node[key].as<std::string>() : defaultValue;
}

void ApplyCommonConfig(EnemyConfig& config, const YAML::Node& enemyNode)
{
    config.knockBackSpeed = ReadFloat(enemyNode, "knockBackSpeed", config.knockBackSpeed);
    config.defaultLaunchedTimer = ReadFloat(enemyNode, "defaultLaunchedTimer", config.defaultLaunchedTimer);
    config.launchHeight = ReadFloat(enemyNode, "launchHeight", config.launchHeight);
    config.detectionRange = ReadFloat(enemyNode, "detectionRange", config.detectionRange);
}

void ApplyTypeConfig(EnemyConfig& config, const YAML::Node& enemyNode)
{
    config.isBoss = ReadBool(enemyNode, "isBoss", config.isBoss);
    config.isNormalHitKnockBackEnabled = ReadBool(
        enemyNode,
        "normalHitKnockBackEnabled",
        config.isNormalHitKnockBackEnabled);
    config.hp = ReadFloat(enemyNode, "hp", config.hp);
    config.scale = ReadFloat(enemyNode, "scale", config.scale);
    config.moveSpeed = ReadFloat(enemyNode, "speed", config.moveSpeed);
    config.attack = ReadFloat(enemyNode, "attack", config.attack);
    config.radius = ReadFloat(enemyNode, "radius", config.radius);

    config.breakCountMax = ReadInt(enemyNode, "breakCountMax", config.breakCountMax);
    config.modelPath = ReadString(enemyNode, "modelPath", config.modelPath);

    config.defaultStandByAttackTimer =
        ReadFloat(enemyNode, "defaultStandByAttackTimer", config.defaultStandByAttackTimer);
    config.defaultAttackMotionTimer =
        ReadFloat(enemyNode, "defaultAttackMotionTimer", config.defaultAttackMotionTimer);
    config.attackSpeed = ReadFloat(enemyNode, "attackSpeed", config.attackSpeed);
    config.attackPreparationRange = ReadFloat(
        enemyNode,
        "attackPreparationRange",
        config.radius + 1.5f);
    config.bossManeuver.preAttackApproachProbabilityPercent = ReadFloat(
        enemyNode,
        "preAttackApproachProbabilityPercent",
        config.bossManeuver.preAttackApproachProbabilityPercent);
    config.bossManeuver.preAttackApproachSpeed = ReadFloat(
        enemyNode,
        "preAttackApproachSpeed",
        config.bossManeuver.preAttackApproachSpeed);
    config.bossManeuver.preAttackApproachStopDistance = ReadFloat(
        enemyNode,
        "preAttackApproachStopDistance",
        config.bossManeuver.preAttackApproachStopDistance);
    config.bossManeuver.postAttackRetreatProbabilityPercent = ReadFloat(
        enemyNode,
        "postAttackRetreatProbabilityPercent",
        config.bossManeuver.postAttackRetreatProbabilityPercent);
    config.bossManeuver.postAttackRetreatDelaySeconds = ReadFloat(
        enemyNode,
        "postAttackRetreatDelaySeconds",
        config.bossManeuver.postAttackRetreatDelaySeconds);
    config.bossManeuver.postAttackRetreatSpeed = ReadFloat(
        enemyNode,
        "postAttackRetreatSpeed",
        config.bossManeuver.postAttackRetreatSpeed);
    config.bossManeuver.postAttackRetreatDistance = ReadFloat(
        enemyNode,
        "postAttackRetreatDistance",
        config.bossManeuver.postAttackRetreatDistance);
    config.bossManeuver.postRetreatRecoverySeconds = ReadFloat(
        enemyNode,
        "postRetreatRecoverySeconds",
        config.bossManeuver.postRetreatRecoverySeconds);
    config.bossManeuver.postRetreatFollowupApproachProbabilityPercent =
        ReadFloat(
            enemyNode,
            "postRetreatFollowupApproachProbabilityPercent",
            config.bossManeuver
                .postRetreatFollowupApproachProbabilityPercent);
    config.behavior.profileName =
        ReadString(enemyNode, "behaviorProfile", config.behavior.profileName);
}

EnemyBehaviorConfig CreateLegacyMeleeBehavior()
{
    EnemyBehaviorConfig behavior;
    behavior.profileName = "legacyMelee";
    behavior.actions = {
        EnemyBehaviorActionConfig{"idle", 1.0f, {}},
        EnemyBehaviorActionConfig{"chase", 1.0f, {}},
        EnemyBehaviorActionConfig{"meleeAttack", 1.0f, {}},
    };
    return behavior;
}

void AppendBehaviorActions(
    EnemyBehaviorConfig& behavior,
    const YAML::Node& actionsNode)
{
    for (const YAML::Node& actionNode : actionsNode) {
        if (!actionNode.IsMap()) {
            continue;
        }

        EnemyBehaviorActionConfig action;
        action.type = ReadString(actionNode, "type", "");
        action.weight = ReadFloat(actionNode, "weight", action.weight);

        if (action.type.empty()) {
            continue;
        }

        for (const auto& entry : actionNode) {
            const std::string key = entry.first.as<std::string>();
            if (key == "type" || key == "weight" || !entry.second.IsScalar()) {
                continue;
            }

            try {
                action.parameters[key] = entry.second.as<float>();
            } catch (const YAML::BadConversion&) {
                // Attack metadata that is not numeric is not consumed at runtime.
            }
        }

        behavior.actions.push_back(std::move(action));
    }
}

void ApplyBehaviorProfile(EnemyConfig& config, const YAML::Node& root)
{
    const YAML::Node profiles = root["behaviorProfiles"];
    const YAML::Node profileNode =
        profiles && profiles.IsMap() ? profiles[config.behavior.profileName] : YAML::Node();

    if (!profileNode || !profileNode.IsMap() ||
        !profileNode["actions"] || !profileNode["actions"].IsSequence()) {
        config.behavior = CreateLegacyMeleeBehavior();
        return;
    }

    EnemyBehaviorConfig behavior;
    behavior.profileName = config.behavior.profileName;
    AppendBehaviorActions(behavior, profileNode["actions"]);

    config.behavior = behavior.actions.empty() ? CreateLegacyMeleeBehavior() : std::move(behavior);
}

bool ApplyPresetAttacks(
    EnemyConfig& config,
    const YAML::Node& enemyNode,
    const std::string& enemyType)
{
    const YAML::Node attacksNode = enemyNode["attacks"];
    if (!attacksNode || !attacksNode.IsSequence()) {
        return false;
    }

    EnemyBehaviorConfig behavior;
    behavior.profileName = "preset:" + enemyType;
    behavior.actions = {
        EnemyBehaviorActionConfig{"idle", 1.0f, {}},
        EnemyBehaviorActionConfig{"chase", 1.0f, {}},
    };
    AppendBehaviorActions(behavior, attacksNode);

    // A saved preset must contain at least one attack. Keep the legacy attack
    // as a safe fallback for hand-edited or incomplete YAML files.
    if (behavior.actions.size() == 2) {
        behavior.actions.push_back(
            EnemyBehaviorActionConfig{"meleeAttack", 1.0f, {}});
    }

    config.behavior = std::move(behavior);
    return true;
}
} // namespace

EnemyConfig EnemyConfigLoader::Load(const std::string& path, const std::string& type)
{
    EnemyConfig config;
    config.isBoss = type == "boss";

    YAML::Node enemyRoot = YAML::LoadFile(path);

    if (!enemyRoot["enemies"] || !enemyRoot["enemies"].IsSequence()) {
        return config;
    }

    YAML::Node selectedEnemyNode;
    for (const YAML::Node& enemyNode : enemyRoot["enemies"]) {
        const std::string enemyType = ReadString(enemyNode, "type", "");

        if (enemyType == "common") {
            ApplyCommonConfig(config, enemyNode);
            continue;
        }

        if (type == enemyType) {
            ApplyTypeConfig(config, enemyNode);
            selectedEnemyNode = enemyNode;
        }
    }

    if (!selectedEnemyNode ||
        !ApplyPresetAttacks(config, selectedEnemyNode, type)) {
        ApplyBehaviorProfile(config, enemyRoot);
    }
    return config;
}
