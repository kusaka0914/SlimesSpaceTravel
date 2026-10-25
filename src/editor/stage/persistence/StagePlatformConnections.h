#pragma once

#include <string>
#include <yaml-cpp/yaml.h>

namespace StagePlatformConnections {

// 他スイッチから対象足場への接続を外し、指定スイッチへ割り当てる。無効な対象には false。
bool AssignExclusiveSwitchTarget(YAML::Node& stageConfig, int switchYamlIndex, const std::string& targetPlatformId);

// 指定スイッチから接続を外す。LatchedGroupSwitchでは同一グループ全体から外す。
bool DisconnectSwitchTarget(YAML::Node& stageConfig, int switchYamlIndex, const std::string& targetPlatformId,
                            int targetPlatformYamlIndex);

// 添字だけの足場参照へplatformIdを補い、足場の再生成後も参照先を追跡できるようにする。
void AddStableIdsToLatchedGroupSwitchTargets(YAML::Node& stageConfig);

// 足場の再生成前後でplatformIdを照合し、対象の参照添字を更新する。
void RemapLatchedGroupSwitchTargetIndices(YAML::Node& stageConfig, const YAML::Node& previousPlatformNodes);

} // namespace StagePlatformConnections
