#include "actor/player/PlayerConfigLoader.h"

#include <yaml-cpp/yaml.h>

#include <string>

namespace {
float ReadFloat(const YAML::Node& node, const char* key, float defaultValue)
{
    return node[key] ? node[key].as<float>() : defaultValue;
}

std::string ReadString(const YAML::Node& node, const char* key, const std::string& defaultValue)
{
    return node[key] ? node[key].as<std::string>() : defaultValue;
}
} // namespace

PlayerConfig PlayerConfigLoader::Load(const std::string& filePath)
{
    PlayerConfig config;

    const YAML::Node playerRoot = YAML::LoadFile(filePath);
    if (!playerRoot["players"] || !playerRoot["players"].IsSequence()) {
        return config;
    }

    // Preserve the old behavior: if multiple player nodes exist, later values overwrite earlier ones.
    for (const YAML::Node& playerNode : playerRoot["players"]) {
        config.hp = ReadFloat(playerNode, "hp", config.hp);
        config.scale = ReadFloat(playerNode, "scale", config.scale);

        config.attackSpeed = ReadFloat(playerNode, "attackSpeed", config.attackSpeed);
        config.attack = ReadFloat(playerNode, "attack", config.attack);
        config.moveSpeed = ReadFloat(playerNode, "moveSpeed", config.moveSpeed);

        config.dodgeDuration = ReadFloat(playerNode, "dodgeDuration", config.dodgeDuration);
        config.dodgeCooldownTime = ReadFloat(playerNode, "dodgeCooldownTime", config.dodgeCooldownTime);
        config.dodgeDistance = ReadFloat(playerNode, "dodgeDistance", config.dodgeDistance);

        config.normalAttackRange = ReadFloat(playerNode, "normalAttackRange", config.normalAttackRange);
        config.normalAttackAngle = ReadFloat(playerNode, "normalAttackAngle", config.normalAttackAngle);
        config.normalAttack = ReadFloat(playerNode, "normalAttack", config.normalAttack);

        config.wideAttackRange = ReadFloat(playerNode, "wideAttackRange", config.wideAttackRange);
        config.wideAttackAngle = ReadFloat(playerNode, "wideAttackAngle", config.wideAttackAngle);
        config.wideAttack = ReadFloat(playerNode, "wideAttack", config.wideAttack);

        config.strongAttackRange = ReadFloat(playerNode, "strongAttackRange", config.strongAttackRange);
        config.strongAttack = ReadFloat(playerNode, "strongAttack", config.strongAttack);
        config.strongAttackSpeed = ReadFloat(playerNode, "strongAttackSpeed", config.strongAttackSpeed);

        config.specialAttackCooldown = ReadFloat(playerNode, "specialAttackCooldown", config.specialAttackCooldown);
        config.defaultInvincibleTimer = ReadFloat(playerNode, "defaultInvincibleTimer", config.defaultInvincibleTimer);
        config.defaultDamageTimer = ReadFloat(playerNode, "defaultDamageTimer", config.defaultDamageTimer);
        config.defaultAttackMotionTimer = ReadFloat(playerNode, "defaultAttackMotionTimer", config.defaultAttackMotionTimer);
        config.attackCooldown = ReadFloat(playerNode, "attackCooldown", config.attackCooldown);
        config.lastAttackCooldown = ReadFloat(playerNode, "lastAttackCooldown", config.lastAttackCooldown);
        config.defaultAttackPressTimer = ReadFloat(playerNode, "defaultAttackPressTimer", config.defaultAttackPressTimer);
        config.chargeMoveSpeed = ReadFloat(playerNode, "chargeMoveSpeed", config.chargeMoveSpeed);
        config.defaultStrongAttackTimer = ReadFloat(playerNode, "defaultStrongAttackTimer", config.defaultStrongAttackTimer);
        config.knockBackSpeed = ReadFloat(playerNode, "knockBackSpeed", config.knockBackSpeed);

        config.modelPath = ReadString(playerNode, "modelPath", config.modelPath);
        config.idleAnimationName = ReadString(playerNode, "idleAnimationName", config.idleAnimationName);
        config.walkAnimationName = ReadString(playerNode, "walkAnimationName", config.walkAnimationName);
        config.attackAnimationName = ReadString(playerNode, "attackAnimationName", config.attackAnimationName);
    }

    return config;
}
