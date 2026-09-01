#include "gfx/debug/stage/StageYamlRepository.h"

#include <fstream>

bool StageYamlRepository::SaveYamlFile(
    const std::string& filePath,
    const YAML::Node& config)
{
    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open yaml for writing: "
                  << filePath << std::endl;
        return false;
    }

    file << config;
    return true;
}

bool StageYamlRepository::RemoveSequenceElement(
    YAML::Node& config,
    const std::string& sequenceName,
    int index)
{
    if (!config[sequenceName] ||
        !config[sequenceName].IsSequence()) {
        std::cerr << "Invalid yaml sequence: "
                  << sequenceName << std::endl;
        return false;
    }

    const YAML::Node oldSequence = config[sequenceName];
    if (index < 0 || index >= static_cast<int>(oldSequence.size())) {
        std::cerr << "Delete index out of range: "
                  << index << std::endl;
        return false;
    }

    YAML::Node newSequence(YAML::NodeType::Sequence);
    for (int oldIndex = 0;
         oldIndex < static_cast<int>(oldSequence.size());
         ++oldIndex) {
        if (oldIndex != index) {
            newSequence.push_back(oldSequence[oldIndex]);
        }
    }

    config[sequenceName] = newSequence;
    return true;
}
