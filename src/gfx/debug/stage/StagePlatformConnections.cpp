#include "gfx/debug/stage/StagePlatformConnections.h"

#include <unordered_map>
#include <vector>

namespace {

bool IsSwitchPlatformNode(const YAML::Node& platformNode)
{
    const YAML::Node components = platformNode["components"];
    if (!components || !components.IsMap()) {
        return false;
    }

    const YAML::Node pressureSwitch = components["pressureSwitch"];
    const YAML::Node latchedGroupSwitch = components["latchedGroupSwitch"];
    return (pressureSwitch && pressureSwitch.IsMap()) ||
           (latchedGroupSwitch && latchedGroupSwitch.IsMap());
}

bool IsConnectableTarget(
    const YAML::Node& platforms,
    int switchYamlIndex,
    const std::string& targetPlatformId,
    int& targetPlatformYamlIndex)
{
    for (std::size_t platformIndex = 0;
         platformIndex < platforms.size();
         ++platformIndex) {
        const YAML::Node platformNode = platforms[platformIndex];
        const YAML::Node platformId = platformNode["platformId"];
        if (!platformId || !platformId.IsScalar() ||
            platformId.as<std::string>() != targetPlatformId) {
            continue;
        }

        targetPlatformYamlIndex = static_cast<int>(platformIndex);
        return targetPlatformYamlIndex != switchYamlIndex &&
               !IsSwitchPlatformNode(platformNode);
    }
    return false;
}

bool RemoveTargetFromLatchedGroupSwitch(
    YAML::Node& platformNode,
    const std::string& targetPlatformId,
    int targetPlatformYamlIndex)
{
    const YAML::Node components = platformNode["components"];
    if (!components || !components.IsMap()) {
        return false;
    }

    YAML::Node latchedGroupSwitch = components["latchedGroupSwitch"];
    if (!latchedGroupSwitch || !latchedGroupSwitch.IsMap()) {
        return false;
    }

    const YAML::Node targets = latchedGroupSwitch["targets"];
    if (!targets || !targets.IsSequence()) {
        return false;
    }

    YAML::Node remainingTargets(YAML::NodeType::Sequence);
    bool wasTargetRemoved = false;
    for (const YAML::Node& target : targets) {
        const bool isMatchingPlatform = target && target.IsMap() &&
            target["sequence"].as<std::string>("") == "platforms" &&
            (target["platformId"].as<std::string>("") ==
                 targetPlatformId ||
             target["index"].as<int>(-1) == targetPlatformYamlIndex);
        if (!isMatchingPlatform) {
            remainingTargets.push_back(YAML::Clone(target));
        } else {
            wasTargetRemoved = true;
        }
    }
    latchedGroupSwitch["targets"] = remainingTargets;
    return wasTargetRemoved;
}

bool AssignTargetToLatchedGroup(
    YAML::Node& platforms,
    const YAML::Node& selectedLatchedGroupSwitch,
    const std::string& targetPlatformId,
    int targetPlatformYamlIndex)
{
    const std::string groupId =
        selectedLatchedGroupSwitch["groupId"].as<std::string>("");
    if (groupId.empty()) {
        return false;
    }

    int settingsOwnerYamlIndex = -1;
    for (std::size_t platformIndex = 0;
         platformIndex < platforms.size();
         ++platformIndex) {
        YAML::Node platformNode = platforms[platformIndex];
        YAML::Node groupSwitch =
            platformNode["components"]["latchedGroupSwitch"];
        if (!groupSwitch || !groupSwitch.IsMap() ||
            groupSwitch["groupId"].as<std::string>("") != groupId) {
            continue;
        }

        if (settingsOwnerYamlIndex < 0) {
            settingsOwnerYamlIndex = static_cast<int>(platformIndex);
        }
        const YAML::Node targets = groupSwitch["targets"];
        if (targets && targets.IsSequence() && targets.size() > 0) {
            settingsOwnerYamlIndex = static_cast<int>(platformIndex);
            break;
        }
    }
    if (settingsOwnerYamlIndex < 0) {
        return false;
    }

    YAML::Node settingsOwner = platforms[settingsOwnerYamlIndex]
        ["components"]["latchedGroupSwitch"];
    YAML::Node targets = settingsOwner["targets"];
    if (!targets || !targets.IsSequence()) {
        settingsOwner["targets"] = YAML::Node(YAML::NodeType::Sequence);
        targets = settingsOwner["targets"];
    }
    YAML::Node target;
    target["sequence"] = "platforms";
    target["index"] = targetPlatformYamlIndex;
    target["platformId"] = targetPlatformId;
    targets.push_back(target);
    return true;
}

bool RemoveTargetFromPressureSwitch(
    YAML::Node& platformNode,
    const std::string& targetPlatformId)
{
    const YAML::Node components = platformNode["components"];
    if (!components || !components.IsMap()) {
        return false;
    }

    YAML::Node pressureSwitch = components["pressureSwitch"];
    if (!pressureSwitch || !pressureSwitch.IsMap()) {
        return false;
    }

    const YAML::Node targets = pressureSwitch["targets"];
    if (!targets || !targets.IsSequence()) {
        return false;
    }

    YAML::Node remainingTargets(YAML::NodeType::Sequence);
    bool wasTargetRemoved = false;
    for (const YAML::Node& target : targets) {
        if (!target || !target.IsScalar() ||
            target.as<std::string>() != targetPlatformId) {
            remainingTargets.push_back(YAML::Clone(target));
        } else {
            wasTargetRemoved = true;
        }
    }
    pressureSwitch["targets"] = remainingTargets;
    return wasTargetRemoved;
}

}

bool StagePlatformConnections::AssignExclusiveSwitchTarget(
    YAML::Node& stageConfig,
    int switchYamlIndex,
    const std::string& targetPlatformId)
{
    YAML::Node platforms = stageConfig["platforms"];
    if (!platforms || !platforms.IsSequence() ||
        switchYamlIndex < 0 ||
        switchYamlIndex >= static_cast<int>(platforms.size()) ||
        targetPlatformId.empty()) {
        return false;
    }

    YAML::Node switchPlatformNode = platforms[switchYamlIndex];
    const YAML::Node switchComponents = switchPlatformNode["components"];
    if (!switchComponents || !switchComponents.IsMap()) {
        return false;
    }

    YAML::Node pressureSwitch = switchComponents["pressureSwitch"];
    YAML::Node latchedGroupSwitch =
        switchComponents["latchedGroupSwitch"];
    const bool isPressureSwitch =
        pressureSwitch && pressureSwitch.IsMap();
    const bool isLatchedGroupSwitch =
        latchedGroupSwitch && latchedGroupSwitch.IsMap();
    int targetPlatformYamlIndex = -1;
    if ((!isPressureSwitch && !isLatchedGroupSwitch) ||
        !IsConnectableTarget(
            platforms,
            switchYamlIndex,
            targetPlatformId,
            targetPlatformYamlIndex)) {
        return false;
    }

    for (YAML::Node platformNode : platforms) {
        RemoveTargetFromPressureSwitch(platformNode, targetPlatformId);
        RemoveTargetFromLatchedGroupSwitch(
            platformNode,
            targetPlatformId,
            targetPlatformYamlIndex);
    }

    if (isLatchedGroupSwitch) {
        return AssignTargetToLatchedGroup(
            platforms,
            latchedGroupSwitch,
            targetPlatformId,
            targetPlatformYamlIndex);
    }

    YAML::Node switchTargets = pressureSwitch["targets"];
    if (!switchTargets || !switchTargets.IsSequence()) {
        pressureSwitch["targets"] = YAML::Node(YAML::NodeType::Sequence);
        switchTargets = pressureSwitch["targets"];
    }
    switchTargets.push_back(targetPlatformId);
    return true;
}

bool StagePlatformConnections::DisconnectSwitchTarget(
    YAML::Node& stageConfig,
    int switchYamlIndex,
    const std::string& targetPlatformId,
    int targetPlatformYamlIndex)
{
    YAML::Node platforms = stageConfig["platforms"];
    if (!platforms || !platforms.IsSequence() ||
        switchYamlIndex < 0 ||
        switchYamlIndex >= static_cast<int>(platforms.size()) ||
        targetPlatformId.empty() || targetPlatformYamlIndex < 0 ||
        targetPlatformYamlIndex >= static_cast<int>(platforms.size())) {
        return false;
    }

    YAML::Node switchPlatformNode = platforms[switchYamlIndex];
    const YAML::Node switchComponents = switchPlatformNode["components"];
    if (!switchComponents || !switchComponents.IsMap()) {
        return false;
    }

    if (switchComponents["pressureSwitch"] &&
        switchComponents["pressureSwitch"].IsMap()) {
        return RemoveTargetFromPressureSwitch(
            switchPlatformNode,
            targetPlatformId);
    }

    const YAML::Node selectedGroupSwitch =
        switchComponents["latchedGroupSwitch"];
    if (!selectedGroupSwitch || !selectedGroupSwitch.IsMap()) {
        return false;
    }

    const std::string groupId =
        selectedGroupSwitch["groupId"].as<std::string>("");
    if (groupId.empty()) {
        return false;
    }

    bool wasTargetRemoved = false;
    for (YAML::Node platformNode : platforms) {
        const YAML::Node groupSwitch = platformNode["components"]
            ["latchedGroupSwitch"];
        if (!groupSwitch || !groupSwitch.IsMap() ||
            groupSwitch["groupId"].as<std::string>("") != groupId) {
            continue;
        }

        wasTargetRemoved = RemoveTargetFromLatchedGroupSwitch(
            platformNode,
            targetPlatformId,
            targetPlatformYamlIndex) || wasTargetRemoved;
    }
    return wasTargetRemoved;
}

void StagePlatformConnections::AddStableIdsToLatchedGroupSwitchTargets(
    YAML::Node& stageConfig)
{
    YAML::Node platforms = stageConfig["platforms"];
    if (!platforms || !platforms.IsSequence()) {
        return;
    }

    for (YAML::Node platformNode : platforms) {
        YAML::Node targets = platformNode["components"]
            ["latchedGroupSwitch"]["targets"];
        if (!targets || !targets.IsSequence()) {
            continue;
        }

        for (YAML::Node target : targets) {
            if (!target || !target.IsMap() ||
                target["sequence"].as<std::string>("") != "platforms" ||
                !target["platformId"].as<std::string>("").empty()) {
                continue;
            }

            const int targetYamlIndex = target["index"].as<int>(-1);
            if (targetYamlIndex < 0 ||
                targetYamlIndex >= static_cast<int>(platforms.size())) {
                continue;
            }

            const std::string platformId = platforms[targetYamlIndex]
                ["platformId"].as<std::string>("");
            if (!platformId.empty()) {
                target["platformId"] = platformId;
            }
        }
    }
}

void StagePlatformConnections::RemapLatchedGroupSwitchTargetIndices(
    YAML::Node& stageConfig,
    const YAML::Node& previousPlatformNodes)
{
    YAML::Node currentPlatformNodes = stageConfig["platforms"];
    if (!previousPlatformNodes || !previousPlatformNodes.IsSequence() ||
        !currentPlatformNodes || !currentPlatformNodes.IsSequence()) {
        return;
    }

    std::vector<std::string> previousPlatformIds;
    previousPlatformIds.reserve(previousPlatformNodes.size());
    for (const YAML::Node& platformNode : previousPlatformNodes) {
        previousPlatformIds.emplace_back(
            platformNode["platformId"].as<std::string>(""));
    }

    std::unordered_map<std::string, int> currentYamlIndexByPlatformId;
    for (std::size_t platformIndex = 0;
         platformIndex < currentPlatformNodes.size();
         ++platformIndex) {
        const std::string platformId = currentPlatformNodes[platformIndex]
            ["platformId"].as<std::string>("");
        if (!platformId.empty()) {
            currentYamlIndexByPlatformId.emplace(
                platformId, static_cast<int>(platformIndex));
        }
    }

    for (YAML::Node platformNode : currentPlatformNodes) {
        YAML::Node targets = platformNode["components"]
            ["latchedGroupSwitch"]["targets"];
        if (!targets || !targets.IsSequence()) {
            continue;
        }

        for (YAML::Node target : targets) {
            if (!target || !target.IsMap() ||
                target["sequence"].as<std::string>("") != "platforms") {
                continue;
            }

            const int previousYamlIndex = target["index"].as<int>(-1);
            std::string targetPlatformId =
                target["platformId"].as<std::string>("");
            if (targetPlatformId.empty() &&
                previousYamlIndex >= 0 &&
                previousYamlIndex <
                    static_cast<int>(previousPlatformIds.size())) {
                targetPlatformId = previousPlatformIds[previousYamlIndex];
            }
            if (targetPlatformId.empty()) {
                continue;
            }

            int currentTargetYamlIndex = -1;
            if (previousYamlIndex >= 0 &&
                previousYamlIndex <
                    static_cast<int>(currentPlatformNodes.size()) &&
                currentPlatformNodes[previousYamlIndex]["platformId"]
                    .as<std::string>("") == targetPlatformId) {
                currentTargetYamlIndex = previousYamlIndex;
            } else {
                const auto currentIndex =
                    currentYamlIndexByPlatformId.find(targetPlatformId);
                if (currentIndex == currentYamlIndexByPlatformId.end()) {
                    continue;
                }
                currentTargetYamlIndex = currentIndex->second;
            }
            target["platformId"] = targetPlatformId;
            target["index"] = currentTargetYamlIndex;
        }
    }
}
