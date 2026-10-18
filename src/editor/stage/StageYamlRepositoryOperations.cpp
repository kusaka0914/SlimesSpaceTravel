#include "editor/stage/StageYamlRepository.h"

#include "utils/AtomicFile.h"

bool StageYamlRepository::LoadYamlFile(const std::string& filePath, YAML::Node& outConfig)
{
    try {
        YAML::Node config = YAML::LoadFile(filePath);
        if (!config.IsMap()) {
            std::cerr << "Invalid stage yaml root: " << filePath << '\n';
            return false;
        }
        outConfig.reset(config);
        return true;
    } catch (const YAML::Exception& error) {
        std::cerr << "Failed to load stage yaml: " << filePath << ": " << error.what() << '\n';
        return false;
    }
}

bool StageYamlRepository::SaveYamlFile(const std::string& filePath, const YAML::Node& config)
{
    try {
        std::string errorMessage;
        if (!AtomicFile::WriteText(filePath, YAML::Dump(config), errorMessage)) {
            std::cerr << "Failed to save yaml: " << filePath << ": " << errorMessage << '\n';
            return false;
        }
        return true;
    } catch (const YAML::Exception& error) {
        std::cerr << "Failed to serialize stage yaml: " << filePath << ": " << error.what() << '\n';
        return false;
    }
}

bool StageYamlRepository::RemoveSequenceElement(YAML::Node& config, const std::string& sequenceName, int index)
{
    if (!config[sequenceName] || !config[sequenceName].IsSequence()) {
        std::cerr << "Invalid yaml sequence: " << sequenceName << std::endl;
        return false;
    }

    const YAML::Node oldSequence = config[sequenceName];
    if (index < 0 || index >= static_cast<int>(oldSequence.size())) {
        std::cerr << "Delete index out of range: " << index << std::endl;
        return false;
    }

    YAML::Node newSequence(YAML::NodeType::Sequence);
    for (int oldIndex = 0; oldIndex < static_cast<int>(oldSequence.size()); ++oldIndex) {
        if (oldIndex != index) {
            newSequence.push_back(oldSequence[oldIndex]);
        }
    }

    config[sequenceName] = newSequence;
    return true;
}
