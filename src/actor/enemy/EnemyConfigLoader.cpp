#include "actor/enemy/EnemyConfigLoader.h"

#include "utils/YamlRead.h"

#include <utility>
#include <yaml-cpp/yaml.h>

namespace {
void ApplyCommonConfig(EnemyConfig& config, const YAML::Node& enemyNode)
{
    config.guardValuePerSegment =
        YamlRead::ReadOr<float>(enemyNode, "guardValuePerSegment", config.guardValuePerSegment);
    config.knockBackSpeed = YamlRead::ReadOr<float>(enemyNode, "knockBackSpeed", config.knockBackSpeed);
    config.defaultLaunchedTimer =
        YamlRead::ReadOr<float>(enemyNode, "defaultLaunchedTimer", config.defaultLaunchedTimer);
    config.launchHeight = YamlRead::ReadOr<float>(enemyNode, "launchHeight", config.launchHeight);
    config.detectionRange = YamlRead::ReadOr<float>(enemyNode, "detectionRange", config.detectionRange);
}

void ApplyTypeConfig(EnemyConfig& config, const YAML::Node& enemyNode)
{
    config.isBoss = YamlRead::ReadOr<bool>(enemyNode, "isBoss", config.isBoss);
    config.isBossEncounter = YamlRead::ReadOr<bool>(enemyNode, "isBossEncounter", config.isBoss);
    config.isNormalHitKnockBackEnabled =
        YamlRead::ReadOr<bool>(enemyNode, "normalHitKnockBackEnabled", config.isNormalHitKnockBackEnabled);
    config.hp = YamlRead::ReadOr<float>(enemyNode, "hp", config.hp);
    config.scale = YamlRead::ReadOr<float>(enemyNode, "scale", config.scale);
    config.moveSpeed = YamlRead::ReadOr<float>(enemyNode, "speed", config.moveSpeed);
    config.attack = YamlRead::ReadOr<float>(enemyNode, "attack", config.attack);
    config.radius = YamlRead::ReadOr<float>(enemyNode, "radius", config.radius);

    config.guardSegmentCount = YamlRead::ReadOr<int>(enemyNode, "breakCountMax", config.guardSegmentCount);
    config.modelPath = YamlRead::ReadOr<std::string>(enemyNode, "modelPath", config.modelPath);

    config.defaultStandByAttackTimer =
        YamlRead::ReadOr<float>(enemyNode, "defaultStandByAttackTimer", config.defaultStandByAttackTimer);
    config.defaultAttackMotionTimer =
        YamlRead::ReadOr<float>(enemyNode, "defaultAttackMotionTimer", config.defaultAttackMotionTimer);
    config.attackSpeed = YamlRead::ReadOr<float>(enemyNode, "attackSpeed", config.attackSpeed);
    config.attackPreparationRange = YamlRead::ReadOr<float>(enemyNode, "attackPreparationRange", config.radius + 1.5f);
    config.bossManeuver.preAttackApproachProbabilityPercent = YamlRead::ReadOr<float>(
        enemyNode, "preAttackApproachProbabilityPercent", config.bossManeuver.preAttackApproachProbabilityPercent);
    config.bossManeuver.preAttackApproachSpeed =
        YamlRead::ReadOr<float>(enemyNode, "preAttackApproachSpeed", config.bossManeuver.preAttackApproachSpeed);
    config.bossManeuver.preAttackApproachStopDistance = YamlRead::ReadOr<float>(
        enemyNode, "preAttackApproachStopDistance", config.bossManeuver.preAttackApproachStopDistance);
    config.bossManeuver.postAttackRetreatProbabilityPercent = YamlRead::ReadOr<float>(
        enemyNode, "postAttackRetreatProbabilityPercent", config.bossManeuver.postAttackRetreatProbabilityPercent);
    config.bossManeuver.postAttackRetreatDelaySeconds = YamlRead::ReadOr<float>(
        enemyNode, "postAttackRetreatDelaySeconds", config.bossManeuver.postAttackRetreatDelaySeconds);
    config.bossManeuver.postAttackRetreatSpeed =
        YamlRead::ReadOr<float>(enemyNode, "postAttackRetreatSpeed", config.bossManeuver.postAttackRetreatSpeed);
    config.bossManeuver.postAttackRetreatDistance =
        YamlRead::ReadOr<float>(enemyNode, "postAttackRetreatDistance", config.bossManeuver.postAttackRetreatDistance);
    config.bossManeuver.postRetreatRecoverySeconds = YamlRead::ReadOr<float>(
        enemyNode, "postRetreatRecoverySeconds", config.bossManeuver.postRetreatRecoverySeconds);
    config.bossManeuver.postRetreatFollowupApproachProbabilityPercent =
        YamlRead::ReadOr<float>(enemyNode, "postRetreatFollowupApproachProbabilityPercent",
                                config.bossManeuver.postRetreatFollowupApproachProbabilityPercent);
    config.behavior.profileName =
        YamlRead::ReadOr<std::string>(enemyNode, "behaviorProfile", config.behavior.profileName);
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

void AppendBehaviorActions(EnemyBehaviorConfig& behavior, const YAML::Node& actionsNode)
{
    for (const YAML::Node& actionNode : actionsNode) {
        if (!actionNode.IsMap()) {
            continue;
        }

        EnemyBehaviorActionConfig action;
        action.type = YamlRead::ReadOr<std::string>(actionNode, "type", "");
        action.weight = YamlRead::ReadOr<float>(actionNode, "weight", action.weight);

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
            }
        }

        behavior.actions.push_back(std::move(action));
    }
}

void ApplyBehaviorProfile(EnemyConfig& config, const YAML::Node& root)
{
    const YAML::Node profiles = root["behaviorProfiles"];
    const YAML::Node profileNode = profiles && profiles.IsMap() ? profiles[config.behavior.profileName] : YAML::Node();

    if (!profileNode || !profileNode.IsMap() || !profileNode["actions"] || !profileNode["actions"].IsSequence()) {
        config.behavior = CreateLegacyMeleeBehavior();
        return;
    }

    EnemyBehaviorConfig behavior;
    behavior.profileName = config.behavior.profileName;
    AppendBehaviorActions(behavior, profileNode["actions"]);

    config.behavior = behavior.actions.empty() ? CreateLegacyMeleeBehavior() : std::move(behavior);
}

bool ApplyPresetAttacks(EnemyConfig& config, const YAML::Node& enemyNode, const std::string& enemyType)
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

    if (behavior.actions.size() == 2) {
        behavior.actions.push_back(EnemyBehaviorActionConfig{"meleeAttack", 1.0f, {}});
    }

    config.behavior = std::move(behavior);
    return true;
}
} // namespace

EnemyConfig EnemyConfigLoader::Parse(const YAML::Node& enemyRoot, const std::string& type)
{
    EnemyConfig config;
    config.isBoss = type == "boss";
    config.isBossEncounter = config.isBoss;

    if (!enemyRoot["enemies"] || !enemyRoot["enemies"].IsSequence()) {
        return config;
    }

    YAML::Node selectedEnemyNode;
    for (const YAML::Node& enemyNode : enemyRoot["enemies"]) {
        const std::string enemyType = YamlRead::ReadOr<std::string>(enemyNode, "type", "");

        if (enemyType == "common") {
            ApplyCommonConfig(config, enemyNode);
            continue;
        }

        if (type == enemyType) {
            ApplyTypeConfig(config, enemyNode);
            selectedEnemyNode = enemyNode;
        }
    }

    if (!selectedEnemyNode || !ApplyPresetAttacks(config, selectedEnemyNode, type)) {
        ApplyBehaviorProfile(config, enemyRoot);
    }
    return config;
}
