#include "actor/enemy/EnemyConfigLoader.h"

#include <yaml-cpp/yaml.h>

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

    return config;
}
