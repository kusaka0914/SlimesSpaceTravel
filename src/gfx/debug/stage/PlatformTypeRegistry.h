#pragma once

#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

struct PlatformTypeDefinition {
    std::string id;
    std::string displayName;
    std::string sequenceName;
    YAML::Node defaults;
};

class PlatformTypeRegistry {
public:
    static const std::vector<PlatformTypeDefinition>& GetDefinitions();
    static const PlatformTypeDefinition* FindBySequenceName(const std::string& sequenceName);

    static void ApplyDefaults(YAML::Node& platformNode, const PlatformTypeDefinition& definition);

private:
    static std::vector<PlatformTypeDefinition> LoadDefinitions();
    static void MergeMissingValues(YAML::Node& target, const YAML::Node& defaults);
};
