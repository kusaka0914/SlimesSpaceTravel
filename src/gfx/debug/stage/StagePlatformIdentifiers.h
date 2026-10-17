#pragma once

#include <string>
#include <yaml-cpp/yaml.h>

namespace StagePlatformIdentifiers {

// 既存の足場IDとPressureSwitchの参照先に重複しない platform_N を返す。configは変更しない。
std::string CreateUniqueId(const YAML::Node& stageConfig);

} // namespace StagePlatformIdentifiers
