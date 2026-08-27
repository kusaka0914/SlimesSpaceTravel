#include "TestSupport.h"

#include "gfx/debug/ugc/UGCWorkMetadata.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

void PrepareForSaveAddsDisplayNameAndUnverifiedDefault()
{
    YAML::Node stageYaml;

    UGCWorkMetadata::PrepareForSave(
        stageYaml,
        "テスト作品",
        "テスト作品.yaml");

    ExpectEqual(
        std::string("テスト作品"),
        stageYaml["ugcMetadata"]["displayName"].as<std::string>(),
        "display name");
    ExpectEqual(
        std::string("テスト作品.yaml"),
        UGCWorkMetadata::FindFileName(stageYaml).value_or(""),
        "file name");
    ExpectFalse(
        UGCWorkMetadata::IsClearVerified(stageYaml),
        "new work verification state");
}

void PrepareForSavePreservesCompletedVerification()
{
    YAML::Node stageYaml;
    UGCWorkMetadata::MarkClearVerified(stageYaml);

    UGCWorkMetadata::PrepareForSave(
        stageYaml,
        "検証済み作品",
        "検証済み作品.yaml");

    ExpectTrue(
        UGCWorkMetadata::IsClearVerified(stageYaml),
        "saved verification state");
}

void EditInvalidatesCompletedVerification()
{
    YAML::Node stageYaml;
    UGCWorkMetadata::MarkClearVerified(stageYaml);

    UGCWorkMetadata::InvalidateClearVerification(stageYaml);

    ExpectFalse(
        UGCWorkMetadata::IsClearVerified(stageYaml),
        "verification state after edit");
}

void GoalDetectionRequiresNonEmptySequence()
{
    YAML::Node missingGoalStage;
    YAML::Node emptyGoalStage;
    emptyGoalStage["star"] = YAML::Node(YAML::NodeType::Sequence);
    YAML::Node goalStage;
    YAML::Node goal;
    goal["currentPlanetNum"] = 0;
    goalStage["star"].push_back(goal);

    ExpectFalse(UGCWorkMetadata::HasGoal(missingGoalStage), "missing goal");
    ExpectFalse(UGCWorkMetadata::HasGoal(emptyGoalStage), "empty goal list");
    ExpectTrue(UGCWorkMetadata::HasGoal(goalStage), "goal exists");
}

void MalformedVerificationFlagIsTreatedAsUnverified()
{
    YAML::Node stageYaml;
    stageYaml["ugcMetadata"]["isClearVerified"] = "invalid";

    ExpectFalse(
        UGCWorkMetadata::IsClearVerified(stageYaml),
        "malformed verification state");
}

void MissingOrMalformedIdentityIsIgnored()
{
    YAML::Node stageYaml;
    ExpectFalse(
        UGCWorkMetadata::FindDisplayName(stageYaml).has_value(),
        "missing display name");
    ExpectFalse(
        UGCWorkMetadata::FindFileName(stageYaml).has_value(),
        "missing file name");

    stageYaml["ugcMetadata"]["displayName"] = YAML::Node(YAML::NodeType::Map);
    stageYaml["ugcMetadata"]["fileName"] = YAML::Node(YAML::NodeType::Sequence);
    ExpectFalse(
        UGCWorkMetadata::FindDisplayName(stageYaml).has_value(),
        "malformed display name");
    ExpectFalse(
        UGCWorkMetadata::FindFileName(stageYaml).has_value(),
        "malformed file name");
}

}

void RegisterUGCWorkMetadataTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.emplace_back("UGCWorkMetadata.PrepareForSaveAddsDisplayNameAndUnverifiedDefault", PrepareForSaveAddsDisplayNameAndUnverifiedDefault);
    tests.emplace_back("UGCWorkMetadata.PrepareForSavePreservesCompletedVerification", PrepareForSavePreservesCompletedVerification);
    tests.emplace_back("UGCWorkMetadata.EditInvalidatesCompletedVerification", EditInvalidatesCompletedVerification);
    tests.emplace_back("UGCWorkMetadata.GoalDetectionRequiresNonEmptySequence", GoalDetectionRequiresNonEmptySequence);
    tests.emplace_back("UGCWorkMetadata.MalformedVerificationFlagIsTreatedAsUnverified", MalformedVerificationFlagIsTreatedAsUnverified);
    tests.emplace_back("UGCWorkMetadata.MissingOrMalformedIdentityIsIgnored", MissingOrMalformedIdentityIsIgnored);
}
