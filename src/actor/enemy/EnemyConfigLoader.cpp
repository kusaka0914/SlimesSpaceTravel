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

    for (const YAML::Node& actionNode : profileNode["actions"]) {
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
                // Numeric parameters are optional; non-numeric metadata is ignored.
            }
        }

        behavior.actions.push_back(std::move(action));
    }

    config.behavior = behavior.actions.empty() ? CreateLegacyMeleeBehavior() : std::move(behavior);
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

    for (const YAML::Node& enemyNode : enemyRoot["enemies"]) {
        const std::string enemyType = ReadString(enemyNode, "type", "");

        if (enemyType == "common") {
            ApplyCommonConfig(config, enemyNode);
            continue;
        }

        if (type == enemyType) {
            ApplyTypeConfig(config, enemyNode);
        }
    }

    ApplyBehaviorProfile(config, enemyRoot);
    return config;
}
