#pragma once

#include "actor/enemy/EnemyConfig.h"

#include <string>

class EnemyConfigLoader {
public:
    static EnemyConfig Load(const std::string& path, const std::string& type);
};
