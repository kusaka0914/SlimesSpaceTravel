#pragma once

#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace StagePlatformIdentifiers {

// 既存の足場ID・PressureSwitchの参照先・reservedIdsに重複しない platform_N を返す。
// 再生成中に一時的に除いた足場のIDも予約できる。stageConfigは変更しない。
std::string CreateUniqueId(const YAML::Node& stageConfig, const std::vector<std::string>& reservedIds = {});

} // namespace StagePlatformIdentifiers
