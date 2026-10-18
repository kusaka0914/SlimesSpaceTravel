#include "actor/player/PlayerConfigLoader.h"

#include "utils/YamlRead.h"

#include <cctype>
#include <string>
#include <utility>
#include <yaml-cpp/yaml.h>

namespace {
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

PlayerAnimationPlaybackMode ParsePlaybackMode(const YAML::Node& animationNode, PlayerAnimationPlaybackMode defaultMode)
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
            definition.clipName = YamlRead::ReadOr<std::string>(animationNode, "clip", definition.clipName);
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
} // namespace

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
        config.hp = YamlRead::ReadOr<float>(playerNode, "hp", config.hp);
        config.scale = YamlRead::ReadOr<float>(playerNode, "scale", config.scale);

        config.attackSpeed = YamlRead::ReadOr<float>(playerNode, "attackSpeed", config.attackSpeed);
        config.attack = YamlRead::ReadOr<float>(playerNode, "attack", config.attack);
        config.moveSpeed = YamlRead::ReadOr<float>(playerNode, "moveSpeed", config.moveSpeed);
        config.maximumStepHeight = YamlRead::ReadOr<float>(playerNode, "maximumStepHeight", config.maximumStepHeight);
        config.jumpHeight = YamlRead::ReadOr<float>(playerNode, "jumpHeight", config.jumpHeight);
        config.jumpAscentDuration =
            YamlRead::ReadOr<float>(playerNode, "jumpAscentDuration", config.jumpAscentDuration);
        config.jumpFallDuration = YamlRead::ReadOr<float>(playerNode, "jumpFallDuration", config.jumpFallDuration);
        config.jumpApexHoverDurationSeconds =
            YamlRead::ReadOr<float>(playerNode, "jumpApexHoverDurationSeconds", config.jumpApexHoverDurationSeconds);
        config.airWeakAttackPostHoverDurationSeconds = YamlRead::ReadOr<float>(
            playerNode, "airWeakAttackPostHoverDurationSeconds", config.airWeakAttackPostHoverDurationSeconds);
        config.airDodgePostHoverDurationSeconds = YamlRead::ReadOr<float>(
            playerNode, "airDodgePostHoverDurationSeconds", config.airDodgePostHoverDurationSeconds);
        config.groundNormalRayLength =
            YamlRead::ReadOr<float>(playerNode, "groundNormalRayLength", config.groundNormalRayLength);
        config.overheadGravityRayLength =
            YamlRead::ReadOr<float>(playerNode, "overheadGravityRayLength", config.overheadGravityRayLength);
        const float legacyCollisionRadius =
            YamlRead::ReadOr<float>(playerNode, "collisionRadius", config.collisionHeight * 0.5f);
        const float legacyCollisionDiameter = 2.0f * legacyCollisionRadius;
        config.collisionWidth = YamlRead::ReadOr<float>(playerNode, "collisionWidth", config.collisionWidth);
        config.collisionHeight = YamlRead::ReadOr<float>(playerNode, "collisionHeight", legacyCollisionDiameter);
        config.collisionDepth = YamlRead::ReadOr<float>(playerNode, "collisionDepth", legacyCollisionDiameter);
        config.collisionCenterHeight =
            YamlRead::ReadOr<float>(playerNode, "collisionCenterHeight", config.collisionCenterHeight);

        config.dodgeDuration = YamlRead::ReadOr<float>(playerNode, "dodgeDuration", config.dodgeDuration);
        config.dodgeCooldownTime = YamlRead::ReadOr<float>(playerNode, "dodgeCooldownTime", config.dodgeCooldownTime);
        config.dodgeDistance = YamlRead::ReadOr<float>(playerNode, "dodgeDistance", config.dodgeDistance);
        config.airDodgeAttackDamage =
            YamlRead::ReadOr<float>(playerNode, "airDodgeAttackDamage", config.airDodgeAttackDamage);
        config.airDodgeGuardDamage =
            YamlRead::ReadOr<float>(playerNode, "airDodgeGuardDamage", config.airDodgeGuardDamage);
        config.airDodgeHorizontalHitboxScale =
            YamlRead::ReadOr<float>(playerNode, "airDodgeHorizontalHitboxScale", config.airDodgeHorizontalHitboxScale);
        config.airDodgeVerticalHitboxScale =
            YamlRead::ReadOr<float>(playerNode, "airDodgeVerticalHitboxScale", config.airDodgeVerticalHitboxScale);
        config.airDodgeEnemyPushSpeed =
            YamlRead::ReadOr<float>(playerNode, "airDodgeEnemyPushSpeed", config.airDodgeEnemyPushSpeed);
        config.airDodgeEnemyPushDampingPerSecond = YamlRead::ReadOr<float>(
            playerNode, "airDodgeEnemyPushDampingPerSecond", config.airDodgeEnemyPushDampingPerSecond);
        config.airWeakEnemyLiftHeight =
            YamlRead::ReadOr<float>(playerNode, "airWeakEnemyLiftHeight", config.airWeakEnemyLiftHeight);
        config.airComboDodgePlayerLiftHeight =
            YamlRead::ReadOr<float>(playerNode, "airComboDodgePlayerLiftHeight", config.airComboDodgePlayerLiftHeight);
        config.airComboDodgeEnemyLiftHeight =
            YamlRead::ReadOr<float>(playerNode, "airComboDodgeEnemyLiftHeight", config.airComboDodgeEnemyLiftHeight);

        config.normalAttackRange = YamlRead::ReadOr<float>(playerNode, "normalAttackRange", config.normalAttackRange);
        config.normalAttackAngle = YamlRead::ReadOr<float>(playerNode, "normalAttackAngle", config.normalAttackAngle);
        config.normalAttack = YamlRead::ReadOr<float>(playerNode, "normalAttack", config.normalAttack);
        config.groundCombo1GuardDamage =
            YamlRead::ReadOr<float>(playerNode, "groundCombo1GuardDamage", config.groundCombo1GuardDamage);
        config.groundCombo2GuardDamage =
            YamlRead::ReadOr<float>(playerNode, "groundCombo2GuardDamage", config.groundCombo2GuardDamage);
        config.groundCombo3GuardDamage =
            YamlRead::ReadOr<float>(playerNode, "groundCombo3GuardDamage", config.groundCombo3GuardDamage);

        config.wideAttackRange = YamlRead::ReadOr<float>(playerNode, "wideAttackRange", config.wideAttackRange);
        config.wideAttackAngle = YamlRead::ReadOr<float>(playerNode, "wideAttackAngle", config.wideAttackAngle);
        config.wideAttack = YamlRead::ReadOr<float>(playerNode, "wideAttack", config.wideAttack);
        config.groundWideGuardDamage =
            YamlRead::ReadOr<float>(playerNode, "groundWideGuardDamage", config.groundWideGuardDamage);
        config.airWeakGuardDamage =
            YamlRead::ReadOr<float>(playerNode, "airWeakGuardDamage", config.airWeakGuardDamage);

        config.strongAttackRange = YamlRead::ReadOr<float>(playerNode, "strongAttackRange", config.strongAttackRange);
        config.strongAttack = YamlRead::ReadOr<float>(playerNode, "strongAttack", config.strongAttack);
        config.strongAttackGuardDamage =
            YamlRead::ReadOr<float>(playerNode, "strongAttackGuardDamage", config.strongAttackGuardDamage);
        config.strongAttackSpeed = YamlRead::ReadOr<float>(playerNode, "strongAttackSpeed", config.strongAttackSpeed);
        config.chargedAttackRange =
            YamlRead::ReadOr<float>(playerNode, "chargedAttackRange", config.chargedAttackRange);
        config.chargedAttackAngle =
            YamlRead::ReadOr<float>(playerNode, "chargedAttackAngle", config.chargedAttackAngle);
        config.chargedAttackDamage =
            YamlRead::ReadOr<float>(playerNode, "chargedAttackDamage", config.chargedAttackDamage);
        config.chargedAttackChargeDurationSeconds = YamlRead::ReadOr<float>(
            playerNode, "chargedAttackChargeDurationSeconds", config.chargedAttackChargeDurationSeconds);
        config.continuousAttackRange =
            YamlRead::ReadOr<float>(playerNode, "continuousAttackRange", config.continuousAttackRange);
        config.continuousAttackAngle =
            YamlRead::ReadOr<float>(playerNode, "continuousAttackAngle", config.continuousAttackAngle);
        config.continuousAttackDamage =
            YamlRead::ReadOr<float>(playerNode, "continuousAttackDamage", config.continuousAttackDamage);
        config.continuousAttackGuardDamage =
            YamlRead::ReadOr<float>(playerNode, "continuousAttackGuardDamage", config.continuousAttackGuardDamage);
        config.continuousAttackIntervalSeconds = YamlRead::ReadOr<float>(playerNode, "continuousAttackIntervalSeconds",
                                                                         config.continuousAttackIntervalSeconds);
        config.continuousAttackDurationSeconds = YamlRead::ReadOr<float>(playerNode, "continuousAttackDurationSeconds",
                                                                         config.continuousAttackDurationSeconds);
        config.airSlamRiseHeight = YamlRead::ReadOr<float>(playerNode, "airSlamRiseHeight", config.airSlamRiseHeight);
        config.airSlamRiseDurationSeconds =
            YamlRead::ReadOr<float>(playerNode, "airSlamRiseDurationSeconds", config.airSlamRiseDurationSeconds);
        config.airSlamHoverDurationSeconds =
            YamlRead::ReadOr<float>(playerNode, "airSlamHoverDurationSeconds", config.airSlamHoverDurationSeconds);
        config.airSlamEnemyDownwardSpeed =
            YamlRead::ReadOr<float>(playerNode, "airSlamEnemyDownwardSpeed", config.airSlamEnemyDownwardSpeed);
        config.airSlamFullDamageHeight =
            YamlRead::ReadOr<float>(playerNode, "airSlamFullDamageHeight", config.airSlamFullDamageHeight);
        config.airSlamMinimumDamageRatio =
            YamlRead::ReadOr<float>(playerNode, "airSlamMinimumDamageRatio", config.airSlamMinimumDamageRatio);
        config.airSlamGuardDamage =
            YamlRead::ReadOr<float>(playerNode, "airSlamGuardDamage", config.airSlamGuardDamage);

        config.specialAttackCooldown =
            YamlRead::ReadOr<float>(playerNode, "specialAttackCooldown", config.specialAttackCooldown);
        config.defaultInvincibleTimer =
            YamlRead::ReadOr<float>(playerNode, "defaultInvincibleTimer", config.defaultInvincibleTimer);
        config.defaultDamageTimer =
            YamlRead::ReadOr<float>(playerNode, "defaultDamageTimer", config.defaultDamageTimer);
        config.defaultAttackMotionTimer =
            YamlRead::ReadOr<float>(playerNode, "defaultAttackMotionTimer", config.defaultAttackMotionTimer);
        config.attackHitDelay = YamlRead::ReadOr<float>(playerNode, "attackHitDelay", config.attackHitDelay);
        config.groundWeakAttackCooldownSeconds = YamlRead::ReadOr<float>(playerNode, "groundWeakAttackCooldownSeconds",
                                                                         config.groundWeakAttackCooldownSeconds);
        config.airWeakAttackCooldownSeconds =
            YamlRead::ReadOr<float>(playerNode, "airWeakAttackCooldownSeconds", config.airWeakAttackCooldownSeconds);
        config.lastAttackCooldown =
            YamlRead::ReadOr<float>(playerNode, "lastAttackCooldown", config.lastAttackCooldown);
        config.defaultStrongAttackTimer =
            YamlRead::ReadOr<float>(playerNode, "defaultStrongAttackTimer", config.defaultStrongAttackTimer);
        config.knockBackSpeed = YamlRead::ReadOr<float>(playerNode, "knockBackSpeed", config.knockBackSpeed);

        config.modelPath = YamlRead::ReadOr<std::string>(playerNode, "modelPath", config.modelPath);

        ReadLegacyAnimationNames(playerNode, config.animations);
        ReadAnimationDefinitions(playerNode, config.animations);
    }

    return config;
}
