#include "TestSupport.h"

#include "gfx/debug/stage/StagePlatformIdentifiers.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

void EmptyStageUsesFirstPlatformIdentifier()
{
    const YAML::Node stageConfig;

    ExpectEqual(
        std::string("platform_1"),
        StagePlatformIdentifiers::CreateUniqueId(stageConfig),
        "identifier for an empty stage");
}

void ExistingPlatformIdentifiersAdvanceSuffix()
{
    YAML::Node stageConfig;
    stageConfig["platform"][0]["platformId"] = "platform_1";
    stageConfig["platform"][1]["platformId"] = "platform_2";

    ExpectEqual(
        std::string("platform_3"),
        StagePlatformIdentifiers::CreateUniqueId(stageConfig),
        "identifier after consecutive used identifiers");
}

void IdentifierSelectionUsesFirstAvailableGap()
{
    YAML::Node stageConfig;
    stageConfig["platform"][0]["platformId"] = "platform_1";
    stageConfig["platform"][1]["platformId"] = "platform_3";

    ExpectEqual(
        std::string("platform_2"),
        StagePlatformIdentifiers::CreateUniqueId(stageConfig),
        "first available identifier");
}

void PressureSwitchTargetsReserveReferencedIdentifiers()
{
    YAML::Node stageConfig;
    stageConfig["platform"][0]["components"]["pressureSwitch"]
        ["targets"].push_back("platform_1");
    stageConfig["platform"][0]["components"]["pressureSwitch"]
        ["targets"].push_back("platform_2");

    ExpectEqual(
        std::string("platform_3"),
        StagePlatformIdentifiers::CreateUniqueId(stageConfig),
        "identifier after referenced targets");
}

void NonSequenceStageEntriesDoNotReserveIdentifiers()
{
    YAML::Node stageConfig;
    stageConfig["metadata"]["platformId"] = "platform_1";
    stageConfig["platform"][0] = "invalid actor node";

    ExpectEqual(
        std::string("platform_1"),
        StagePlatformIdentifiers::CreateUniqueId(stageConfig),
        "identifier when unrelated entries contain matching text");
}

}

void RegisterStagePlatformIdentifiersTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.emplace_back(
        "StagePlatformIdentifiers.EmptyStageUsesFirstPlatformIdentifier",
        EmptyStageUsesFirstPlatformIdentifier);
    tests.emplace_back(
        "StagePlatformIdentifiers.ExistingPlatformIdentifiersAdvanceSuffix",
        ExistingPlatformIdentifiersAdvanceSuffix);
    tests.emplace_back(
        "StagePlatformIdentifiers.IdentifierSelectionUsesFirstAvailableGap",
        IdentifierSelectionUsesFirstAvailableGap);
    tests.emplace_back(
        "StagePlatformIdentifiers.PressureSwitchTargetsReserveReferencedIdentifiers",
        PressureSwitchTargetsReserveReferencedIdentifiers);
    tests.emplace_back(
        "StagePlatformIdentifiers.NonSequenceStageEntriesDoNotReserveIdentifiers",
        NonSequenceStageEntriesDoNotReserveIdentifiers);
}
