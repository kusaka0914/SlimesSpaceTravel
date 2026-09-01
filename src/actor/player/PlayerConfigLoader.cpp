#include "actor/player/PlayerConfigLoader.h"

#include <yaml-cpp/yaml.h>

#include <cctype>
#include <string>
#include <utility>

namespace {
float ReadFloat(const YAML::Node& node, const char* key, float defaultValue)
{
    return node[key] ? node[key].as<float>() : defaultValue;
}

std::string ReadString(const YAML::Node& node, const char* key, const std::string& defaultValue)
{
    return node[key] ? node[key].as<std::string>() : defaultValue;
}

std::string NormalizeAnimationId(const std::string& animationId)
{
    std::string normalizedId;
    normalizedId.reserve(animationId.size());

    for (const unsigned char character : animationId) {
        if (std::isspace(character) || character == '-' || character == '_') {
            continue;
        }
        normalizedId.push_back(static_cast<char>(std::tolower(character)));
    }

    return normalizedId;
}

PlayerAnimationPlaybackMode ParsePlaybackMode(const YAML::Node& animationNode,
                                              PlayerAnimationPlaybackMode defaultMode)
{
    if (animationNode["mode"]) {
        const std::string normalizedMode = NormalizeAnimationId(animationNode["mode"].as<std::string>());

        if (normalizedMode == "base" || normalizedMode == "baseloop" || normalizedMode == "loop") {
            return PlayerAnimationPlaybackMode::BaseLoop;
        }

        if (normalizedMode == "oneshot" || normalizedMode == "action") {
            return PlayerAnimationPlaybackMode::OneShot;
        }
    }

    if (animationNode["loop"]) {
        return animationNode["loop"].as<bool>() ? PlayerAnimationPlaybackMode::BaseLoop
                                                : PlayerAnimationPlaybackMode::OneShot;
    }

    return defaultMode;
}

void ReadAnimationDefinitions(const YAML::Node& playerNode, PlayerAnimationDefinitions& definitions)
{
    const YAML::Node animationsNode = playerNode["animations"];
    if (!animationsNode || !animationsNode.IsMap()) {
        return;
    }

    for (const auto& animationEntry : animationsNode) {
        const std::string animationId = NormalizeAnimationId(animationEntry.first.as<std::string>());
        if (animationId.empty()) {
            continue;
        }

        PlayerAnimationDefinition definition;
        const auto existingDefinition = definitions.find(animationId);
        if (existingDefinition != definitions.end()) {
            definition = existingDefinition->second;
        } else {
            definition.clipName = animationEntry.first.as<std::string>();
            definition.playbackMode = PlayerAnimationPlaybackMode::OneShot;
        }

        const YAML::Node animationNode = animationEntry.second;
        if (animationNode.IsScalar()) {
            definition.clipName = animationNode.as<std::string>();
        } else if (animationNode.IsMap()) {
            definition.clipName = ReadString(animationNode, "clip", definition.clipName);
            definition.playbackMode = ParsePlaybackMode(animationNode, definition.playbackMode);
        }

        definitions[animationId] = std::move(definition);
    }
}

void ReadLegacyAnimationNames(const YAML::Node& playerNode, PlayerAnimationDefinitions& definitions)
{

    if (playerNode["idleAnimationName"]) {
        definitions["idle"] = {
            playerNode["idleAnimationName"].as<std::string>(),
            PlayerAnimationPlaybackMode::BaseLoop,
        };
    }

    if (playerNode["walkAnimationName"]) {
        definitions["walk"] = {
            playerNode["walkAnimationName"].as<std::string>(),
            PlayerAnimationPlaybackMode::BaseLoop,
        };
    }

    if (playerNode["attackAnimationName"]) {
        definitions["attack"] = {
            playerNode["attackAnimationName"].as<std::string>(),
            PlayerAnimationPlaybackMode::OneShot,
        };
    }
}
}

PlayerConfig PlayerConfigLoader::Load(const std::string& filePath)
{
    const YAML::Node playerRoot = YAML::LoadFile(filePath);
    return Parse(playerRoot);
}

PlayerConfig PlayerConfigLoader::Parse(const YAML::Node& playerRoot)
{
    PlayerConfig config;

    if (!playerRoot["players"] || !playerRoot["players"].IsSequence()) {
        return config;
    }

    for (const YAML::Node& playerNode : playerRoot["players"]) {
        config.hp = ReadFloat(playerNode, "hp", config.hp);
        config.scale = ReadFloat(playerNode, "scale", config.scale);

        config.attackSpeed = ReadFloat(playerNode, "attackSpeed", config.attackSpeed);
        config.attack = ReadFloat(playerNode, "attack", config.attack);
        config.moveSpeed = ReadFloat(playerNode, "moveSpeed", config.moveSpeed);
        config.maximumStepHeight =
            ReadFloat(
                playerNode,
                "maximumStepHeight",
                config.maximumStepHeight);
        config.jumpHeight = ReadFloat(playerNode, "jumpHeight", config.jumpHeight);
        config.jumpAscentDuration = ReadFloat(playerNode, "jumpAscentDuration", config.jumpAscentDuration);
        config.jumpFallDuration = ReadFloat(playerNode, "jumpFallDuration", config.jumpFallDuration);
        config.jumpApexHoverDurationSeconds =
            ReadFloat(
                playerNode,
                "jumpApexHoverDurationSeconds",
                config.jumpApexHoverDurationSeconds);
        config.airWeakAttackPostHoverDurationSeconds =
            ReadFloat(
                playerNode,
                "airWeakAttackPostHoverDurationSeconds",
                config.airWeakAttackPostHoverDurationSeconds);
        config.airDodgePostHoverDurationSeconds =
            ReadFloat(
                playerNode,
                "airDodgePostHoverDurationSeconds",
                config.airDodgePostHoverDurationSeconds);
        config.groundNormalRayLength =
            ReadFloat(
                playerNode,
                "groundNormalRayLength",
                config.groundNormalRayLength);
        config.overheadGravityRayLength =
            ReadFloat(
                playerNode,
                "overheadGravityRayLength",
                config.overheadGravityRayLength);
        const float legacyCollisionRadius =
            ReadFloat(
                playerNode,
                "collisionRadius",
                config.collisionHeight * 0.5f);
        const float legacyCollisionDiameter =
            2.0f * legacyCollisionRadius;
        config.collisionWidth =
            ReadFloat(playerNode, "collisionWidth", config.collisionWidth);
        config.collisionHeight =
            ReadFloat(
                playerNode,
                "collisionHeight",
                legacyCollisionDiameter);
        config.collisionDepth =
            ReadFloat(
                playerNode,
                "collisionDepth",
                legacyCollisionDiameter);
        config.collisionCenterHeight =
            ReadFloat(
                playerNode,
                "collisionCenterHeight",
                config.collisionCenterHeight);

        config.dodgeDuration = ReadFloat(playerNode, "dodgeDuration", config.dodgeDuration);
        config.dodgeCooldownTime = ReadFloat(playerNode, "dodgeCooldownTime", config.dodgeCooldownTime);
        config.dodgeDistance = ReadFloat(playerNode, "dodgeDistance", config.dodgeDistance);
        config.airDodgeAttackDamage =
            ReadFloat(
                playerNode,
                "airDodgeAttackDamage",
                config.airDodgeAttackDamage);
        config.airDodgeHorizontalHitboxScale =
            ReadFloat(
                playerNode,
                "airDodgeHorizontalHitboxScale",
                config.airDodgeHorizontalHitboxScale);
        config.airDodgeVerticalHitboxScale =
            ReadFloat(
                playerNode,
                "airDodgeVerticalHitboxScale",
                config.airDodgeVerticalHitboxScale);
        config.airDodgeEnemyPushSpeed =
            ReadFloat(
                playerNode,
                "airDodgeEnemyPushSpeed",
                config.airDodgeEnemyPushSpeed);
        config.airDodgeEnemyPushDampingPerSecond =
            ReadFloat(
                playerNode,
                "airDodgeEnemyPushDampingPerSecond",
                config.airDodgeEnemyPushDampingPerSecond);

        config.normalAttackRange = ReadFloat(playerNode, "normalAttackRange", config.normalAttackRange);
        config.normalAttackAngle = ReadFloat(playerNode, "normalAttackAngle", config.normalAttackAngle);
        config.normalAttack = ReadFloat(playerNode, "normalAttack", config.normalAttack);

        config.wideAttackRange = ReadFloat(playerNode, "wideAttackRange", config.wideAttackRange);
        config.wideAttackAngle = ReadFloat(playerNode, "wideAttackAngle", config.wideAttackAngle);
        config.wideAttack = ReadFloat(playerNode, "wideAttack", config.wideAttack);

        config.strongAttackRange = ReadFloat(playerNode, "strongAttackRange", config.strongAttackRange);
        config.strongAttack = ReadFloat(playerNode, "strongAttack", config.strongAttack);
        config.strongAttackSpeed = ReadFloat(playerNode, "strongAttackSpeed", config.strongAttackSpeed);
        config.chargedAttackRange = ReadFloat(playerNode, "chargedAttackRange", config.chargedAttackRange);
        config.chargedAttackAngle = ReadFloat(playerNode, "chargedAttackAngle", config.chargedAttackAngle);
        config.chargedAttackDamage = ReadFloat(playerNode, "chargedAttackDamage", config.chargedAttackDamage);
        config.chargedAttackChargeDurationSeconds =
            ReadFloat(
                playerNode,
                "chargedAttackChargeDurationSeconds",
                config.chargedAttackChargeDurationSeconds);
        config.continuousAttackRange =
            ReadFloat(playerNode, "continuousAttackRange", config.continuousAttackRange);
        config.continuousAttackAngle =
            ReadFloat(playerNode, "continuousAttackAngle", config.continuousAttackAngle);
        config.continuousAttackDamage =
            ReadFloat(playerNode, "continuousAttackDamage", config.continuousAttackDamage);
        config.continuousAttackIntervalSeconds =
            ReadFloat(
                playerNode,
                "continuousAttackIntervalSeconds",
                config.continuousAttackIntervalSeconds);
        config.continuousAttackDurationSeconds =
            ReadFloat(
                playerNode,
                "continuousAttackDurationSeconds",
                config.continuousAttackDurationSeconds);
        config.airSlamRiseHeight = ReadFloat(playerNode, "airSlamRiseHeight", config.airSlamRiseHeight);
        config.airSlamRiseDurationSeconds =
            ReadFloat(
                playerNode,
                "airSlamRiseDurationSeconds",
                config.airSlamRiseDurationSeconds);
        config.airSlamHoverDurationSeconds =
            ReadFloat(
                playerNode,
                "airSlamHoverDurationSeconds",
                config.airSlamHoverDurationSeconds);

        config.specialAttackCooldown = ReadFloat(playerNode, "specialAttackCooldown", config.specialAttackCooldown);
        config.defaultInvincibleTimer = ReadFloat(playerNode, "defaultInvincibleTimer", config.defaultInvincibleTimer);
        config.defaultDamageTimer = ReadFloat(playerNode, "defaultDamageTimer", config.defaultDamageTimer);
        config.defaultAttackMotionTimer = ReadFloat(playerNode, "defaultAttackMotionTimer", config.defaultAttackMotionTimer);
        config.attackHitDelay = ReadFloat(playerNode, "attackHitDelay", config.attackHitDelay);
        config.groundWeakAttackCooldownSeconds =
            ReadFloat(
                playerNode,
                "groundWeakAttackCooldownSeconds",
                config.groundWeakAttackCooldownSeconds);
        config.airWeakAttackCooldownSeconds =
            ReadFloat(
                playerNode,
                "airWeakAttackCooldownSeconds",
                config.airWeakAttackCooldownSeconds);
        config.lastAttackCooldown = ReadFloat(playerNode, "lastAttackCooldown", config.lastAttackCooldown);
        config.defaultStrongAttackTimer = ReadFloat(playerNode, "defaultStrongAttackTimer", config.defaultStrongAttackTimer);
        config.knockBackSpeed = ReadFloat(playerNode, "knockBackSpeed", config.knockBackSpeed);

        config.modelPath = ReadString(playerNode, "modelPath", config.modelPath);

        ReadLegacyAnimationNames(playerNode, config.animations);
        ReadAnimationDefinitions(playerNode, config.animations);
    }

    return config;
}
