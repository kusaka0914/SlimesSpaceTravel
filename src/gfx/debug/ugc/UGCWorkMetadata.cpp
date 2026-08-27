#include "gfx/debug/ugc/UGCWorkMetadata.h"

void UGCWorkMetadata::PrepareForSave(
    YAML::Node& stageYaml,
    const std::string& displayName,
    const std::string& fileName)
{
    stageYaml["ugcMetadata"]["displayName"] = displayName;
    stageYaml["ugcMetadata"]["fileName"] = fileName;
    if (!stageYaml["ugcMetadata"]["isClearVerified"]) {
        stageYaml["ugcMetadata"]["isClearVerified"] = false;
    }
}

std::optional<std::string> UGCWorkMetadata::FindDisplayName(
    const YAML::Node& stageYaml)
{
    try {
        const YAML::Node displayName =
            stageYaml["ugcMetadata"]["displayName"];
        if (!displayName || !displayName.IsScalar()) {
            return std::nullopt;
        }
        return displayName.as<std::string>();
    } catch (const YAML::Exception&) {
        return std::nullopt;
    }
}

std::optional<std::string> UGCWorkMetadata::FindFileName(
    const YAML::Node& stageYaml)
{
    try {
        const YAML::Node fileName =
            stageYaml["ugcMetadata"]["fileName"];
        if (!fileName || !fileName.IsScalar()) {
            return std::nullopt;
        }
        return fileName.as<std::string>();
    } catch (const YAML::Exception&) {
        return std::nullopt;
    }
}

void UGCWorkMetadata::InvalidateClearVerification(YAML::Node& stageYaml)
{
    stageYaml["ugcMetadata"]["isClearVerified"] = false;
}

void UGCWorkMetadata::MarkClearVerified(YAML::Node& stageYaml)
{
    stageYaml["ugcMetadata"]["isClearVerified"] = true;
}

bool UGCWorkMetadata::IsClearVerified(const YAML::Node& stageYaml)
{
    try {
        return stageYaml["ugcMetadata"] &&
            stageYaml["ugcMetadata"]["isClearVerified"].as<bool>(false);
    } catch (const YAML::Exception&) {
        return false;
    }
}

bool UGCWorkMetadata::HasGoal(const YAML::Node& stageYaml)
{
    const YAML::Node goals = stageYaml["star"];
    return goals && goals.IsSequence() && goals.size() > 0;
}
