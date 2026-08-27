#pragma once

#include <optional>
#include <string>
#include <yaml-cpp/yaml.h>

namespace UGCWorkMetadata {

void PrepareForSave(
    YAML::Node& stageYaml,
    const std::string& displayName,
    const std::string& fileName);
std::optional<std::string> FindDisplayName(const YAML::Node& stageYaml);
std::optional<std::string> FindFileName(const YAML::Node& stageYaml);
void InvalidateClearVerification(YAML::Node& stageYaml);
void MarkClearVerified(YAML::Node& stageYaml);
bool IsClearVerified(const YAML::Node& stageYaml);
bool HasGoal(const YAML::Node& stageYaml);

}
