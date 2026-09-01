#pragma once

#include <string>
#include <yaml-cpp/yaml.h>

namespace StagePlatformConnections {

bool AssignExclusiveSwitchTarget(
    YAML::Node& stageConfig,
    int switchYamlIndex,
    const std::string& targetPlatformId);
bool DisconnectSwitchTarget(
    YAML::Node& stageConfig,
    int switchYamlIndex,
    const std::string& targetPlatformId,
    int targetPlatformYamlIndex);
void AddStableIdsToLatchedGroupSwitchTargets(
    YAML::Node& stageConfig);
void RemapLatchedGroupSwitchTargetIndices(
    YAML::Node& stageConfig,
    const YAML::Node& previousPlatformNodes);

}
