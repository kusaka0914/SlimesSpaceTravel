#pragma once

#include "actor/enemy/EnemyConfig.h"

#include <string>

namespace YAML {
class Node;
}

class EnemyConfigLoader {
public:
    static EnemyConfig Parse(
        const YAML::Node& enemyRoot,
        const std::string& type);
};
