#include "gfx/debug/stage/PlatformTypeRegistry.h"

#include <array>
#include <filesystem>
#include <iostream>
#include <unordered_set>

namespace {

constexpr std::array<const char*, 3> kDefinitionPaths = {
    "../assets/data/debug/platform_types.yaml",
    "assets/data/debug/platform_types.yaml",
    "../../assets/data/debug/platform_types.yaml",
};

std::vector<PlatformTypeDefinition> CreateFallbackDefinitions()
{
    std::vector<PlatformTypeDefinition> definitions;

    PlatformTypeDefinition normal;
    normal.id = "normal";
    normal.displayName = "通常足場";
    normal.sequenceName = "platforms";
    definitions.emplace_back(std::move(normal));

    PlatformTypeDefinition moving;
    moving.id = "moving";
    moving.displayName = "動く足場";
    moving.sequenceName = "movingPlatforms";
    moving.defaults["moveOnPlayer"] = false;
    moving.defaults["moveDuration"] = 3.0f;
    moving.defaults["returnDelay"] = 1.0f;
    moving.defaults["endpointWaitSeconds"] = 0.0f;
    moving.defaults["moveOffset"][0] = 4.0f;
    moving.defaults["moveOffset"][1] = 0.0f;
    moving.defaults["moveOffset"][2] = 0.0f;
    definitions.emplace_back(std::move(moving));

    return definitions;
}

}

const std::vector<PlatformTypeDefinition>& PlatformTypeRegistry::GetDefinitions()
{
    static const std::vector<PlatformTypeDefinition> definitions = LoadDefinitions();
    return definitions;
}

const PlatformTypeDefinition*
PlatformTypeRegistry::FindBySequenceName(const std::string& sequenceName)
{
    for (const PlatformTypeDefinition& definition : GetDefinitions()) {
        if (definition.sequenceName == sequenceName) {
            return &definition;
        }
    }

    return nullptr;
}

void PlatformTypeRegistry::ApplyDefaults(
    YAML::Node& platformNode,
    const PlatformTypeDefinition& definition)
{
    MergeMissingValues(platformNode, definition.defaults);
}

std::vector<PlatformTypeDefinition> PlatformTypeRegistry::LoadDefinitions()
{
    std::filesystem::path definitionPath;
    for (const char* candidate : kDefinitionPaths) {
        std::error_code error;
        if (std::filesystem::exists(candidate, error) && !error) {
            definitionPath = candidate;
            break;
        }
    }

    if (definitionPath.empty()) {
        std::cerr << "Platform type definitions were not found. Using built-in defaults." << std::endl;
        return CreateFallbackDefinitions();
    }

    try {
        const YAML::Node root = YAML::LoadFile(definitionPath.string());
        const YAML::Node typeNodes = root["platformTypes"];
        if (!typeNodes || !typeNodes.IsSequence()) {
            std::cerr << "platformTypes must be a sequence: " << definitionPath << std::endl;
            return CreateFallbackDefinitions();
        }

        std::vector<PlatformTypeDefinition> definitions;
        std::unordered_set<std::string> ids;
        std::unordered_set<std::string> sequenceNames;

        for (const YAML::Node& typeNode : typeNodes) {
            if (!typeNode["id"] || !typeNode["displayName"] || !typeNode["sequenceName"]) {
                continue;
            }

            PlatformTypeDefinition definition;
            definition.id = typeNode["id"].as<std::string>();
            definition.displayName = typeNode["displayName"].as<std::string>();
            definition.sequenceName = typeNode["sequenceName"].as<std::string>();

            if (definition.id.empty() || definition.displayName.empty() ||
                definition.sequenceName.empty() || ids.contains(definition.id) ||
                sequenceNames.contains(definition.sequenceName)) {
                continue;
            }

            if (typeNode["defaults"] && typeNode["defaults"].IsMap()) {
                definition.defaults = YAML::Clone(typeNode["defaults"]);
            }

            ids.insert(definition.id);
            sequenceNames.insert(definition.sequenceName);
            definitions.emplace_back(std::move(definition));
        }

        if (!definitions.empty()) {
            return definitions;
        }
    } catch (const YAML::Exception& error) {
        std::cerr << "Failed to load platform type definitions: " << error.what() << std::endl;
    }

    return CreateFallbackDefinitions();
}

void PlatformTypeRegistry::MergeMissingValues(YAML::Node& target, const YAML::Node& defaults)
{
    if (!defaults || !defaults.IsMap()) {
        return;
    }

    for (const auto& entry : defaults) {
        const std::string key = entry.first.as<std::string>();
        const YAML::Node defaultValue = entry.second;
        YAML::Node targetValue = target[key];

        if (!targetValue) {
            target[key] = YAML::Clone(defaultValue);
            continue;
        }

        if (targetValue.IsMap() && defaultValue.IsMap()) {
            MergeMissingValues(targetValue, defaultValue);
        }
    }
}
