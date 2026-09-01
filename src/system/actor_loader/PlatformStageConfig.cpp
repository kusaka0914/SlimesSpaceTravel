#include "system/actor_loader/PlatformStageConfig.h"

#include <utility>

namespace {

std::optional<glm::vec3> ReadOptionalVec3(
    const YAML::Node& node,
    const char* key)
{
    if (!node || !node[key] || !node[key].IsSequence() ||
        node[key].size() < 3) {
        return std::nullopt;
    }

    return glm::vec3(
        node[key][0].as<float>(),
        node[key][1].as<float>(),
        node[key][2].as<float>());
}

glm::vec3 ReadVec3(
    const YAML::Node& node,
    const char* key,
    const glm::vec3& fallback)
{
    const std::optional<glm::vec3> parsedValue =
        ReadOptionalVec3(node, key);
    return parsedValue.value_or(fallback);
}

std::vector<PlatformRevealTarget> ParseActorTargets(
    const YAML::Node& targetsNode)
{
    std::vector<PlatformRevealTarget> targets;
    if (!targetsNode || !targetsNode.IsSequence()) {
        return targets;
    }

    targets.reserve(targetsNode.size());
    for (const YAML::Node& targetNode : targetsNode) {
        if (!targetNode || !targetNode.IsMap() ||
            !targetNode["sequence"] || !targetNode["index"]) {
            continue;
        }

        PlatformRevealTarget target;
        target.sequenceName =
            targetNode["sequence"].as<std::string>("");
        target.yamlIndex = targetNode["index"].as<int>(-1);
        target.platformId =
            targetNode["platformId"].as<std::string>("");
        if (target.IsValid()) {
            targets.emplace_back(std::move(target));
        }
    }
    return targets;
}

PlatformMovementStageConfig ParseMovementConfig(
    const YAML::Node& movementNode)
{
    PlatformMovementStageConfig config;
    config.startLocalPos =
        ReadOptionalVec3(movementNode, "startLocalPos");
    config.moveOffset = ReadVec3(
        movementNode,
        "moveOffset",
        config.moveOffset);
    config.endLocalPos =
        ReadOptionalVec3(movementNode, "endLocalPos");
    config.moveDurationSeconds =
        movementNode["moveDuration"].as<float>(3.0f);
    config.shouldMoveOnPlayer =
        movementNode["moveOnPlayer"].as<bool>(false);
    config.returnDelaySeconds =
        movementNode["returnDelay"].as<float>(1.0f);

    const YAML::Node endpointWaitNode =
        movementNode["endpointWaitSeconds"]
            ? movementNode["endpointWaitSeconds"]
            : movementNode["destinationWaitSeconds"];
    config.endpointWaitSeconds = endpointWaitNode.as<float>(0.0f);
    return config;
}

PlatformBehaviorStageConfig ParseBehaviorConfig(
    const YAML::Node& components)
{
    PlatformBehaviorStageConfig config;
    if (!components || !components.IsMap()) {
        return config;
    }

    if (const YAML::Node node = components["fadeOnStand"];
        node && node.IsMap()) {
        config.fadeOnStand = PlatformFadeOnStandStageConfig{
            .fadeOutDurationSeconds =
                node["fadeOutDuration"].as<float>(1.0f),
            .reappearDelaySeconds =
                node["reappearDelay"].as<float>(2.0f),
        };
    }

    if (const YAML::Node node = components["jumpToggle"];
        node && node.IsMap()) {
        config.jumpToggleInitiallyVisible =
            node["initiallyVisible"].as<bool>(true);
    }

    if (const YAML::Node node = components["intervalToggle"];
        node && node.IsMap()) {
        config.intervalToggle = PlatformIntervalToggleStageConfig{
            .intervalSeconds = node["interval"].as<float>(3.0f),
            .warningDurationSeconds =
                node["warningDuration"].as<float>(1.0f),
            .blinkIntervalSeconds =
                node["blinkInterval"].as<float>(0.15f),
            .isInitiallyVisible =
                node["initiallyVisible"].as<bool>(true),
        };
    }

    if (const YAML::Node node = components["directionalMovement"];
        node && node.IsMap()) {
        config.directionalMovementSpeed =
            node["speed"].as<float>(2.0f);
    }

    if (const YAML::Node node = components["rotation"];
        node && node.IsMap()) {
        config.rotation = PlatformRotationStageConfig{
            .localAxis = ReadVec3(
                node,
                "axis",
                glm::vec3(0.0f, 1.0f, 0.0f)),
            .degreesPerSecond =
                node["degreesPerSecond"].as<float>(45.0f),
        };
    }

    if (const YAML::Node node = components["conveyor"];
        node && node.IsMap()) {
        config.conveyor = PlatformConveyorStageConfig{
            .localDirection = ReadVec3(
                node,
                "direction",
                glm::vec3(0.0f, 0.0f, 1.0f)),
            .speed = node["speed"].as<float>(2.0f),
        };
    }

    config.hasAdhesion =
        components["adhesion"] && components["adhesion"].IsMap();
    config.hasEnemyClearUnlock =
        components["enemyClearUnlock"] &&
        components["enemyClearUnlock"].IsMap();

    if (const YAML::Node node = components["pressureSwitch"];
        node && node.IsMap()) {
        PlatformPressureSwitchStageConfig pressureSwitch;
        if (const YAML::Node targets = node["targets"];
            targets && targets.IsSequence()) {
            pressureSwitch.targetPlatformIds.reserve(targets.size());
            for (const YAML::Node& target : targets) {
                if (target && target.IsScalar()) {
                    pressureSwitch.targetPlatformIds.emplace_back(
                        target.as<std::string>());
                }
            }
        }
        pressureSwitch.targetEnemyRefs =
            ParseActorTargets(node["enemyTargets"]);
        pressureSwitch.hideTargets =
            ParseActorTargets(node["hideTargets"]);
        pressureSwitch.inactiveOpacity =
            node["inactiveOpacity"].as<float>(0.2f);
        pressureSwitch.shouldRemainOnAfterPressed =
            node["remainsOnAfterPressed"].as<bool>(false);
        config.pressureSwitch = std::move(pressureSwitch);
    }

    if (const YAML::Node node = components["latchedGroupSwitch"];
        node && node.IsMap()) {
        config.latchedGroupSwitch =
            PlatformLatchedGroupSwitchStageConfig{
                .groupId = node["groupId"].as<std::string>(""),
                .revealTargets = ParseActorTargets(node["targets"]),
                .hideTargets = ParseActorTargets(node["hideTargets"]),
            };
    }

    return config;
}

}

PlatformStageConfig ParsePlatformStageConfig(
    const YAML::Node& platformNode,
    bool shouldUseLegacyMovementFields)
{
    PlatformStageConfig config;
    const YAML::Node components = platformNode["components"];
    const YAML::Node movementNode =
        components && components.IsMap() &&
                components["movement"] &&
                components["movement"].IsMap()
            ? components["movement"]
            : YAML::Node();

    if (movementNode && movementNode.IsMap()) {
        config.movement = ParseMovementConfig(movementNode);
    } else if (shouldUseLegacyMovementFields) {
        config.movement = ParseMovementConfig(platformNode);
    }

    config.behavior = ParseBehaviorConfig(components);
    return config;
}
