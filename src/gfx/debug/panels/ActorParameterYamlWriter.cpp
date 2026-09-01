#include "gfx/debug/panels/ActorParameterYamlWriter.h"

#include "Game.h"
#include "actor/Enemy.h"
#include "actor/Player.h"
#include "actor/Star.h"
#include "system/PhysicsSystem.h"

#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <yaml-cpp/yaml.h>

namespace {
bool SaveYamlFile(const std::string& filePath, const YAML::Node& config)
{
    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open yaml for writing: "
                  << filePath << std::endl;
        return false;
    }
    file << config;
    return true;
}

template <typename T>
bool SetSequenceValue(
    YAML::Node& config,
    const std::string& sequenceName,
    std::size_t index,
    const std::string& key,
    const T& value)
{
    const YAML::Node sequence = config[sequenceName];
    if (!sequence || !sequence.IsSequence() || index >= sequence.size()) {
        std::cerr << "Invalid yaml location: " << sequenceName
                  << '[' << index << "]." << key << std::endl;
        return false;
    }
    config[sequenceName][index][key] = value;
    return true;
}

std::optional<std::size_t> FindEntryIndex(
    const YAML::Node& config,
    const std::string& sequenceName,
    const std::string& key,
    const std::string& expectedValue)
{
    const YAML::Node sequence = config[sequenceName];
    if (!sequence || !sequence.IsSequence()) {
        return std::nullopt;
    }
    for (std::size_t index = 0; index < sequence.size(); ++index) {
        if (sequence[index][key] &&
            sequence[index][key].as<std::string>() == expectedValue) {
            return index;
        }
    }
    return std::nullopt;
}

bool LoadYaml(const std::string& filePath, YAML::Node& outConfig)
{
    try {
        outConfig = YAML::LoadFile(filePath);
        return true;
    } catch (const YAML::Exception& exception) {
        std::cerr << "Failed to load yaml: " << filePath << std::endl;
        std::cerr << exception.what() << std::endl;
        return false;
    }
}

void WriteEnemyValues(
    YAML::Node& config,
    std::size_t index,
    const Enemy& enemy)
{
    constexpr const char* sequenceName = "enemies";
    SetSequenceValue(config, sequenceName, index, "hp", enemy.GetMaxHp());
    SetSequenceValue(config, sequenceName, index, "modelPath", enemy.GetModelPath());
    SetSequenceValue(config, sequenceName, index, "scale", enemy.GetScale().x);
    SetSequenceValue(config, sequenceName, index, "speed", enemy.GetMoveSpeed());
    SetSequenceValue(config, sequenceName, index, "attack", enemy.GetAttack());
    SetSequenceValue(config, sequenceName, index, "breakCountMax", enemy.GetBreakCountMax());
    SetSequenceValue(config, sequenceName, index, "radius", enemy.GetRadius());
    SetSequenceValue(config, sequenceName, index, "defaultStandByAttackTimer", enemy.GetDefaultStandByAttackTimer());
    SetSequenceValue(config, sequenceName, index, "defaultAttackMotionTimer", enemy.GetDefaultAttackMotionTimer());
    SetSequenceValue(config, sequenceName, index, "attackSpeed", enemy.GetAttackSpeed());
}
}

ActorParameterYamlWriter::ActorParameterYamlWriter(
    DebugEditorContext& context)
    : mContext(context)
{
}

bool ActorParameterYamlWriter::SaveStarCollectionAnimation(
    const Star& star) const
{
    const Star::CollectionAnimationSettings& settings =
        star.GetCollectionAnimationSettings();
    YAML::Node config;
    YAML::Node starConfig;
    starConfig["modelPath"] = star.GetModelPath();
    starConfig["scale"] = star.GetScale().x;
    starConfig["collectionAnimation"]["orbitDuration"] =
        settings.orbitDuration;
    starConfig["collectionAnimation"]["orbitStartRadius"] =
        settings.orbitStartRadius;
    starConfig["collectionAnimation"]["orbitSpinDegreesPerSecond"] =
        settings.orbitSpinDegreesPerSecond;
    starConfig["collectionAnimation"]["finalHeight"] =
        settings.finalHeight;
    starConfig["collectionAnimation"]["waitAbovePlayerDuration"] =
        settings.waitAbovePlayerDuration;
    starConfig["collectionAnimation"]["fallDuration"] =
        settings.fallDuration;
    config["stars"].push_back(starConfig);
    return SaveYamlFile("../assets/data/actor/stars.yaml", config);
}

bool ActorParameterYamlWriter::SavePlayer(const Player& player) const
{
    constexpr const char* filePath = "../assets/data/actor/players.yaml";
    constexpr const char* sequenceName = "players";
    constexpr std::size_t index = 0;
    YAML::Node config;
    if (!LoadYaml(filePath, config)) {
        return false;
    }

    const auto set = [&](const char* key, const auto& value) {
        SetSequenceValue(config, sequenceName, index, key, value);
    };
    set("hp", player.GetMaxHp());
    set("scale", player.GetBaseScale().x);
    set("attack", player.GetAttack());
    set("attackSpeed", player.GetAttackSpeed());
    set("moveSpeed", player.GetMoveSpeed());
    set("maximumStepHeight", player.GetMaximumStepHeight());
    set("jumpHeight", player.GetJumpHeight());
    set("jumpAscentDuration", player.GetJumpAscentDuration());
    set("jumpFallDuration", player.GetJumpFallDuration());
    set("jumpApexHoverDurationSeconds", player.GetJumpApexHoverDurationSeconds());
    set("airWeakAttackPostHoverDurationSeconds", player.GetAirWeakAttackPostHoverDurationSeconds());
    set("airDodgePostHoverDurationSeconds", player.GetAirDodgePostHoverDurationSeconds());
    if (mContext.game) {
        set("groundNormalRayLength", mContext.game->GetGroundNormalRayLength());
        set("overheadGravityRayLength", mContext.game->GetOverheadGravityRayLength());
        if (PhysicsSystem* physicsSystem = mContext.game->GetPhysicsSystem()) {
            config[sequenceName][index].remove("collisionRadius");
            set("collisionWidth", physicsSystem->GetPlayerCollisionWidth());
            set("collisionHeight", physicsSystem->GetPlayerCollisionHeight());
            set("collisionDepth", physicsSystem->GetPlayerCollisionDepth());
            set("collisionCenterHeight", physicsSystem->GetPlayerCollisionCenterHeight());
        }
    }
    set("dodgeDuration", player.GetDodgeDuration());
    set("dodgeCooldownTime", player.GetDodgeCooldownTime());
    set("dodgeDistance", player.GetDodgeDistance());
    set("airDodgeAttackDamage", player.GetAirDodgeAttackDamage());
    set("airDodgeHorizontalHitboxScale", player.GetAirDodgeHorizontalHitboxScale());
    set("airDodgeVerticalHitboxScale", player.GetAirDodgeVerticalHitboxScale());
    set("airDodgeEnemyPushSpeed", player.GetAirDodgeEnemyPushSpeed());
    set("airDodgeEnemyPushDampingPerSecond", player.GetAirDodgeEnemyPushDampingPerSecond());
    set("normalAttackRange", player.GetNormalAttackRange());
    set("normalAttackAngle", player.GetNormalAttackAngle());
    set("normalAttack", player.GetNormalAttack());
    set("wideAttackRange", player.GetWideAttackRange());
    set("wideAttackAngle", player.GetWideAttackAngle());
    set("wideAttack", player.GetWideAttack());
    set("strongAttackRange", player.GetStrongAttackRange());
    set("strongAttack", player.GetStrongAttack());
    set("strongAttackSpeed", player.GetStrongAttackSpeed());
    set("chargedAttackRange", player.GetChargedAttackRange());
    set("chargedAttackAngle", player.GetChargedAttackAngle());
    set("chargedAttackDamage", player.GetChargedAttackDamage());
    set("chargedAttackChargeDurationSeconds", player.GetChargedAttackChargeDurationSeconds());
    set("continuousAttackRange", player.GetContinuousAttackRange());
    set("continuousAttackAngle", player.GetContinuousAttackAngle());
    set("continuousAttackDamage", player.GetContinuousAttackDamage());
    set("continuousAttackIntervalSeconds", player.GetContinuousAttackIntervalSeconds());
    set("continuousAttackDurationSeconds", player.GetContinuousAttackDurationSeconds());
    set("airSlamRiseHeight", player.GetAirSlamRiseHeight());
    set("airSlamRiseDurationSeconds", player.GetAirSlamRiseDurationSeconds());
    set("airSlamHoverDurationSeconds", player.GetAirSlamHoverDurationSeconds());
    set("specialAttackCooldown", player.GetSpecialAttackCooldown());
    set("defaultInvincibleTimer", player.GetDefaultInvincibleTimer());
    set("defaultDamageTimer", player.GetDefaultDamageTimer());
    set("defaultAttackMotionTimer", player.GetDefaultAttackMotionTimer());
    set("attackHitDelay", player.GetAttackHitDelay());
    config[sequenceName][index].remove("attackCooldown");
    set("groundWeakAttackCooldownSeconds", player.GetGroundWeakAttackCooldownSeconds());
    set("airWeakAttackCooldownSeconds", player.GetAirWeakAttackCooldownSeconds());
    set("lastAttackCooldown", player.GetLastAttackCooldown());
    set("defaultStrongAttackTimer", player.GetDefaultStrongAttackTimer());
    set("knockBackSpeed", player.GetKnockBackSpeed());
    set("modelPath", player.GetModelPath());
    return SaveYamlFile(filePath, config);
}

bool ActorParameterYamlWriter::SaveEnemies(
    const Enemy* normalEnemy,
    const Enemy* bossEnemy) const
{
    if (!normalEnemy && !bossEnemy) {
        return false;
    }
    constexpr const char* filePath = "../assets/data/actor/enemies.yaml";
    constexpr const char* sequenceName = "enemies";
    YAML::Node config;
    if (!LoadYaml(filePath, config)) {
        return false;
    }

    const auto commonIndex = FindEntryIndex(config, sequenceName, "type", "common");
    const auto normalIndex = FindEntryIndex(config, sequenceName, "type", "normal");
    const auto bossIndex = FindEntryIndex(config, sequenceName, "type", "boss");
    const Enemy* commonEnemy = normalEnemy ? normalEnemy : bossEnemy;
    if (commonEnemy && commonIndex) {
        SetSequenceValue(config, sequenceName, *commonIndex, "knockBackSpeed", commonEnemy->GetKnockBackSpeed());
        SetSequenceValue(config, sequenceName, *commonIndex, "defaultLaunchedTimer", commonEnemy->GetDefaultLaunchedTimer());
        SetSequenceValue(config, sequenceName, *commonIndex, "launchHeight", commonEnemy->GetLaunchHeight());
        SetSequenceValue(config, sequenceName, *commonIndex, "detectionRange", commonEnemy->GetDetectionRange());
    }
    if (normalEnemy && normalIndex) {
        WriteEnemyValues(config, *normalIndex, *normalEnemy);
    }
    if (bossEnemy && bossIndex) {
        WriteEnemyValues(config, *bossIndex, *bossEnemy);
    }

    const bool hasRequiredEntries = commonIndex.has_value() &&
        (!normalEnemy || normalIndex.has_value()) &&
        (!bossEnemy || bossIndex.has_value());
    if (!hasRequiredEntries) {
        std::cerr << "Required enemy type was not found in "
                  << filePath << std::endl;
        return false;
    }
    return SaveYamlFile(filePath, config);
}
