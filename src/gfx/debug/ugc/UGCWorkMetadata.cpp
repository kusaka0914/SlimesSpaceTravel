#include "gfx/debug/ugc/UGCWorkMetadata.h"

void UGCWorkMetadata::PrepareForSave(
    YAML::Node& stageYaml,
    const std::string& displayName)
{
    stageYaml["ugcMetadata"]["displayName"] = displayName;
    if (!stageYaml["ugcMetadata"]["isClearVerified"]) {
        stageYaml["ugcMetadata"]["isClearVerified"] = false;
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
