#pragma once

#include "gfx/debug/DebugEditorContext.h"

#include <cstddef>
#include <iostream>
#include <string>
#include <yaml-cpp/yaml.h>

class StageYamlRepository {
public:
    static std::string GetCurrentStageYamlPath(DebugEditorContext& context);

    static bool LoadCurrentStage(DebugEditorContext& context, YAML::Node& outConfig);
    static bool SaveCurrentStage(DebugEditorContext& context, const YAML::Node& config);

    static bool ReadCurrentStageText(DebugEditorContext& context, std::string& outYamlText);
    static bool WriteCurrentStageTextAtomically(DebugEditorContext& context, const std::string& yamlText);

    static bool SaveYamlFile(const std::string& filePath, const YAML::Node& config);

    static bool RemoveSequenceElement(YAML::Node& config, const std::string& sequenceName, int index);

    template <typename T>
    static bool SetSequenceValue(YAML::Node& config, const std::string& sequenceName, std::size_t index,
                                 const std::string& key, const T& value)
    {
        if (!config[sequenceName] || !config[sequenceName].IsSequence()) {
            std::cerr << "Invalid yaml sequence: " << sequenceName << std::endl;
            return false;
        }

        if (index >= config[sequenceName].size()) {
            std::cerr << "Index out of range: " << index << std::endl;
            return false;
        }

        config[sequenceName][index][key] = value;
        return true;
    }
};