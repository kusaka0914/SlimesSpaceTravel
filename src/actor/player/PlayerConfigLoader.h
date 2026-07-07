#pragma once

#include "actor/player/PlayerConfig.h"

#include <string>

class PlayerConfigLoader {
public:
    static PlayerConfig Load(const std::string& filePath);
};
