#pragma once

#include <string>
#include <yaml-cpp/yaml.h>

namespace StagePlatformIdentifiers {

std::string CreateUniqueId(const YAML::Node& stageConfig);

}
