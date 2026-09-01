#pragma once

#include "actor/player/PlayerConfig.h"

#include <string>

namespace YAML {
class Node;
}

class PlayerConfigLoader {
public:
    static PlayerConfig Load(const std::string& filePath);
    static PlayerConfig Parse(const YAML::Node& playerRoot);
};
