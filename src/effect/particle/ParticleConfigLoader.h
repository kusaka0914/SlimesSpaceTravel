#pragma once

#include "effect/particle/ParticleTypes.h"

#include <string>
#include <unordered_map>

class ParticleConfigLoader {
public:
    static bool Load(
        const std::string& filePath,
        std::unordered_map<std::string, ParticleEffectDefinition>& outDefinitions);

    static bool Save(
        const std::string& filePath,
        const std::unordered_map<std::string, ParticleEffectDefinition>& definitions);
};
