#include "gfx/debug/stage/StagePlatformConnections.h"

#include "TestSupport.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

YAML::Node CreatePlatform(const std::string& platformId)
{
    YAML::Node platformNode;
    platformNode["platformId"] = platformId;
    return platformNode;
}

YAML::Node CreatePressureSwitch(
    const std::string& platformId,
    const std::vector<std::string>& targetPlatformIds)
{
    YAML::Node platformNode = CreatePlatform(platformId);
    YAML::Node targets(YAML::NodeType::Sequence);
    for (const std::string& targetPlatformId : targetPlatformIds) {
        targets.push_back(targetPlatformId);
    }
    platformNode["components"]["pressureSwitch"]["targets"] = targets;
    return platformNode;
}

YAML::Node CreateTwoPlayerSwitch(
    const std::string& platformId,
    const std::string& groupId,
    const std::vector<int>& targetPlatformYamlIndices)
{
    YAML::Node platformNode = CreatePlatform(platformId);
    YAML::Node targets(YAML::NodeType::Sequence);
    for (const int targetPlatformYamlIndex : targetPlatformYamlIndices) {
        YAML::Node target;
        target["sequence"] = "platforms";
        target["index"] = targetPlatformYamlIndex;
        targets.push_back(target);
    }
    platformNode["components"]["latchedGroupSwitch"]["groupId"] = groupId;
    platformNode["components"]["latchedGroupSwitch"]["targets"] = targets;
    return platformNode;
}

int CountTarget(
    const YAML::Node& switchPlatformNode,
    const std::string& targetPlatformId)
{
    const YAML::Node targets = switchPlatformNode["components"]
        ["pressureSwitch"]["targets"];
    int targetCount = 0;
    for (const YAML::Node& target : targets) {
        if (target.as<std::string>("") == targetPlatformId) {
            ++targetCount;
        }
    }
    return targetCount;
}

int CountTwoPlayerSwitchTarget(
    const YAML::Node& switchPlatformNode,
    int targetPlatformYamlIndex)
{
    const YAML::Node targets = switchPlatformNode["components"]
        ["latchedGroupSwitch"]["targets"];
    int targetCount = 0;
    for (const YAML::Node& target : targets) {
        if (target["sequence"].as<std::string>("") == "platforms" &&
            target["index"].as<int>(-1) == targetPlatformYamlIndex) {
            ++targetCount;
        }
    }
    return targetCount;
}

int CountTwoPlayerSwitchTargetId(
    const YAML::Node& switchPlatformNode,
    const std::string& targetPlatformId)
{
    const YAML::Node targets = switchPlatformNode["components"]
        ["latchedGroupSwitch"]["targets"];
    int targetCount = 0;
    for (const YAML::Node& target : targets) {
        if (target["platformId"].as<std::string>("") ==
            targetPlatformId) {
            ++targetCount;
        }
    }
    return targetCount;
}

void AssigningTargetToSecondSwitchRemovesItFromFirstSwitch()
{
    YAML::Node stageConfig;
    stageConfig["platforms"].push_back(CreatePlatform("target_platform"));
    stageConfig["platforms"].push_back(CreatePlatform("other_platform"));
    stageConfig["platforms"].push_back(CreatePressureSwitch(
        "first_switch",
        {"target_platform", "other_platform"}));
    stageConfig["platforms"].push_back(CreatePressureSwitch(
        "second_switch",
        {}));

    ExpectTrue(
        StagePlatformConnections::AssignExclusiveSwitchTarget(
            stageConfig,
            3,
            "target_platform"),
        "assign target to second switch");
    ExpectEqual(
        0,
        CountTarget(stageConfig["platforms"][2], "target_platform"),
        "first switch target count");
    ExpectEqual(
        1,
        CountTarget(stageConfig["platforms"][2], "other_platform"),
        "first switch unrelated target count");
    ExpectEqual(
        1,
        CountTarget(stageConfig["platforms"][3], "target_platform"),
        "second switch target count");
}

void SwitchPlatformCannotBeAssignedAsTarget()
{
    YAML::Node stageConfig;
    stageConfig["platforms"].push_back(CreatePressureSwitch(
        "first_switch",
        {}));
    stageConfig["platforms"].push_back(CreatePressureSwitch(
        "second_switch",
        {}));

    ExpectFalse(
        StagePlatformConnections::AssignExclusiveSwitchTarget(
            stageConfig,
            0,
            "second_switch"),
        "assign switch platform as target");
}

void AssigningTargetToTwoPlayerSwitchRemovesItFromPressureSwitch()
{
    YAML::Node stageConfig;
    stageConfig["platforms"].push_back(CreatePlatform("target_platform"));
    stageConfig["platforms"].push_back(CreatePressureSwitch(
        "pressure_switch",
        {"target_platform"}));
    stageConfig["platforms"].push_back(CreateTwoPlayerSwitch(
        "two_player_switch_a",
        "pair_a",
        {}));
    stageConfig["platforms"].push_back(CreateTwoPlayerSwitch(
        "two_player_switch_b",
        "pair_a",
        {}));

    ExpectTrue(
        StagePlatformConnections::AssignExclusiveSwitchTarget(
            stageConfig,
            3,
            "target_platform"),
        "assign target to two-player switch");
    ExpectEqual(
        0,
        CountTarget(stageConfig["platforms"][1], "target_platform"),
        "pressure switch target count");
    const int pairTargetCount =
        CountTwoPlayerSwitchTarget(stageConfig["platforms"][2], 0) +
        CountTwoPlayerSwitchTarget(stageConfig["platforms"][3], 0);
    ExpectEqual(1, pairTargetCount, "two-player switch pair target count");
    const int pairTargetIdCount =
        CountTwoPlayerSwitchTargetId(
            stageConfig["platforms"][2], "target_platform") +
        CountTwoPlayerSwitchTargetId(
            stageConfig["platforms"][3], "target_platform");
    ExpectEqual(
        1,
        pairTargetIdCount,
        "two-player switch pair stable target id count");
}

void AssigningTargetToPressureSwitchRemovesItFromTwoPlayerSwitch()
{
    YAML::Node stageConfig;
    stageConfig["platforms"].push_back(CreatePlatform("target_platform"));
    stageConfig["platforms"].push_back(CreatePressureSwitch(
        "pressure_switch",
        {}));
    stageConfig["platforms"].push_back(CreateTwoPlayerSwitch(
        "two_player_switch_a",
        "pair_a",
        {0}));
    stageConfig["platforms"].push_back(CreateTwoPlayerSwitch(
        "two_player_switch_b",
        "pair_a",
        {}));

    ExpectTrue(
        StagePlatformConnections::AssignExclusiveSwitchTarget(
            stageConfig,
            1,
            "target_platform"),
        "assign target to pressure switch");
    ExpectEqual(
        1,
        CountTarget(stageConfig["platforms"][1], "target_platform"),
        "pressure switch target count");
    ExpectEqual(
        0,
        CountTwoPlayerSwitchTarget(stageConfig["platforms"][2], 0),
        "two-player switch target count");
}

void ReorderedPlatformsPreserveTwoPlayerSwitchTarget()
{
    YAML::Node previousPlatformNodes(YAML::NodeType::Sequence);
    previousPlatformNodes.push_back(CreatePlatform("generated_target"));
    previousPlatformNodes.push_back(CreatePlatform("authored_platform"));
    previousPlatformNodes.push_back(CreateTwoPlayerSwitch(
        "two_player_switch_a",
        "pair_a",
        {0}));

    YAML::Node stageConfig;
    stageConfig["platforms"].push_back(CreatePlatform("authored_platform"));
    stageConfig["platforms"].push_back(CreateTwoPlayerSwitch(
        "two_player_switch_a",
        "pair_a",
        {0}));
    stageConfig["platforms"].push_back(CreatePlatform("generated_target"));

    StagePlatformConnections::RemapLatchedGroupSwitchTargetIndices(
        stageConfig,
        previousPlatformNodes);

    ExpectEqual(
        1,
        CountTwoPlayerSwitchTarget(stageConfig["platforms"][1], 2),
        "remapped two-player switch target count");
    ExpectEqual(
        0,
        CountTwoPlayerSwitchTarget(stageConfig["platforms"][1], 0),
        "stale two-player switch target count");
    ExpectEqual(
        1,
        CountTwoPlayerSwitchTargetId(
            stageConfig["platforms"][1], "generated_target"),
        "stable two-player switch target id count");
}

void ExistingTwoPlayerSwitchTargetReceivesStablePlatformId()
{
    YAML::Node stageConfig;
    stageConfig["platforms"].push_back(CreatePlatform("target_platform"));
    stageConfig["platforms"].push_back(CreateTwoPlayerSwitch(
        "two_player_switch",
        "pair_a",
        {0}));

    StagePlatformConnections::AddStableIdsToLatchedGroupSwitchTargets(
        stageConfig);

    ExpectEqual(
        1,
        CountTwoPlayerSwitchTargetId(
            stageConfig["platforms"][1], "target_platform"),
        "migrated stable target id count");
}

void DisconnectingPressureSwitchTargetPreservesOtherTargets()
{
    YAML::Node stageConfig;
    stageConfig["platforms"].push_back(CreatePlatform("first_target"));
    stageConfig["platforms"].push_back(CreatePlatform("second_target"));
    stageConfig["platforms"].push_back(CreatePressureSwitch(
        "pressure_switch",
        {"first_target", "second_target"}));

    ExpectTrue(
        StagePlatformConnections::DisconnectSwitchTarget(
            stageConfig,
            2,
            "first_target",
            0),
        "disconnect selected pressure switch target");
    ExpectEqual(
        0,
        CountTarget(stageConfig["platforms"][2], "first_target"),
        "disconnected pressure switch target count");
    ExpectEqual(
        1,
        CountTarget(stageConfig["platforms"][2], "second_target"),
        "preserved pressure switch target count");
}

void DisconnectingTwoPlayerSwitchTargetPreservesOtherGroupTargets()
{
    YAML::Node stageConfig;
    stageConfig["platforms"].push_back(CreatePlatform("first_target"));
    stageConfig["platforms"].push_back(CreatePlatform("second_target"));
    stageConfig["platforms"].push_back(CreateTwoPlayerSwitch(
        "two_player_switch_a",
        "pair_a",
        {0, 1}));
    stageConfig["platforms"].push_back(CreateTwoPlayerSwitch(
        "two_player_switch_b",
        "pair_a",
        {}));

    ExpectTrue(
        StagePlatformConnections::DisconnectSwitchTarget(
            stageConfig,
            3,
            "first_target",
            0),
        "disconnect selected two-player switch target");
    ExpectEqual(
        0,
        CountTwoPlayerSwitchTarget(stageConfig["platforms"][2], 0),
        "disconnected two-player switch target count");
    ExpectEqual(
        1,
        CountTwoPlayerSwitchTarget(stageConfig["platforms"][2], 1),
        "preserved two-player switch target count");
}

}

void RegisterStagePlatformConnectionsTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.emplace_back(
        "StagePlatformConnections.AssigningTargetToSecondSwitchRemovesItFromFirstSwitch",
        AssigningTargetToSecondSwitchRemovesItFromFirstSwitch);
    tests.emplace_back(
        "StagePlatformConnections.SwitchPlatformCannotBeAssignedAsTarget",
        SwitchPlatformCannotBeAssignedAsTarget);
    tests.emplace_back(
        "StagePlatformConnections.AssigningTargetToTwoPlayerSwitchRemovesItFromPressureSwitch",
        AssigningTargetToTwoPlayerSwitchRemovesItFromPressureSwitch);
    tests.emplace_back(
        "StagePlatformConnections.AssigningTargetToPressureSwitchRemovesItFromTwoPlayerSwitch",
        AssigningTargetToPressureSwitchRemovesItFromTwoPlayerSwitch);
    tests.emplace_back(
        "StagePlatformConnections.ReorderedPlatformsPreserveTwoPlayerSwitchTarget",
        ReorderedPlatformsPreserveTwoPlayerSwitchTarget);
    tests.emplace_back(
        "StagePlatformConnections.ExistingTwoPlayerSwitchTargetReceivesStablePlatformId",
        ExistingTwoPlayerSwitchTargetReceivesStablePlatformId);
    tests.emplace_back(
        "StagePlatformConnections.DisconnectingPressureSwitchTargetPreservesOtherTargets",
        DisconnectingPressureSwitchTargetPreservesOtherTargets);
    tests.emplace_back(
        "StagePlatformConnections.DisconnectingTwoPlayerSwitchTargetPreservesOtherGroupTargets",
        DisconnectingTwoPlayerSwitchTargetPreservesOtherGroupTargets);
}
