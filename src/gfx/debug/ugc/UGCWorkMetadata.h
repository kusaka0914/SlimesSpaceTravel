#pragma once

#include <string>
#include <yaml-cpp/yaml.h>

namespace UGCWorkMetadata {

void PrepareForSave(
    YAML::Node& stageYaml,
    const std::string& displayName);
void InvalidateClearVerification(YAML::Node& stageYaml);
void MarkClearVerified(YAML::Node& stageYaml);
bool IsClearVerified(const YAML::Node& stageYaml);
bool HasGoal(const YAML::Node& stageYaml);

}
