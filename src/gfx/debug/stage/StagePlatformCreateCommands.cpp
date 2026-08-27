#include "gfx/debug/stage/StageActorCreateService.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "component/PlatformBehaviorComponents.h"
#include "gfx/debug/stage/StagePlatformConnections.h"
#include "gfx/debug/stage/StagePlatformIdentifiers.h"
#include "gfx/debug/stage/StageYamlRepository.h"

#include <string>
#include <unordered_set>
#include <utility>

namespace {

std::vector<std::string> ReadPressureSwitchTargetPlatformIds(
    const YAML::Node& targetsNode)
{
    std::vector<std::string> targetPlatformIds;
    if (!targetsNode || !targetsNode.IsSequence()) {
        return targetPlatformIds;
    }

    for (const YAML::Node& targetNode : targetsNode) {
        if (targetNode && targetNode.IsScalar()) {
            targetPlatformIds.push_back(targetNode.as<std::string>());
        }
    }
    return targetPlatformIds;
}

std::vector<PlatformRevealTarget> ReadLatchedSwitchTargets(
    const YAML::Node& targetsNode)
{
    std::vector<PlatformRevealTarget> targets;
    if (!targetsNode || !targetsNode.IsSequence()) {
        return targets;
    }

    for (const YAML::Node& targetNode : targetsNode) {
        if (!targetNode || !targetNode.IsMap() ||
            !targetNode["sequence"] || !targetNode["index"]) {
            continue;
        }

        PlatformRevealTarget target;
        target.sequenceName =
            targetNode["sequence"].as<std::string>();
        target.yamlIndex = targetNode["index"].as<int>();
        target.platformId =
            targetNode["platformId"].as<std::string>("");
        if (target.IsValid()) {
            targets.push_back(std::move(target));
        }
    }
    return targets;
}

std::string CreateUniqueTwoPlayerSwitchGroupId(
    const YAML::Node& platformNodes)
{
    std::unordered_set<std::string> existingGroupIds;
    if (platformNodes && platformNodes.IsSequence()) {
        for (const YAML::Node& platformNode : platformNodes) {
            const YAML::Node components = platformNode["components"];
            if (!components || !components.IsMap()) {
                continue;
            }

            const YAML::Node groupSwitch =
                components["latchedGroupSwitch"];
            if (!groupSwitch || !groupSwitch.IsMap()) {
                continue;
            }

            const std::string groupId =
                groupSwitch["groupId"].as<std::string>("");
            if (!groupId.empty()) {
                existingGroupIds.insert(groupId);
            }
        }
    }

    int pairNumber = 1;
    while (true) {
        const std::string candidateGroupId =
            "ugc_two_player_pair_" + std::to_string(pairNumber);
        if (!existingGroupIds.contains(candidateGroupId)) {
            return candidateGroupId;
        }
        ++pairNumber;
    }
}

}

void StageActorCreateService::SynchronizeRuntimeSwitchTargets(
    const YAML::Node& stageConfig) const
{
    Stage* stage = mContext.game
        ? mContext.game->GetCurrentStage()
        : nullptr;
    const YAML::Node platformNodes = stageConfig["platforms"];
    if (!stage || !platformNodes || !platformNodes.IsSequence()) {
        return;
    }

    for (Planet* planet : stage->GetPlanets()) {
        if (!planet) {
            continue;
        }
        for (Platform* platform : planet->GetPlatforms()) {
            if (!platform) {
                continue;
            }

            const int yamlIndex = platform->GetStageYamlIndex();
            if (yamlIndex < 0 ||
                yamlIndex >= static_cast<int>(platformNodes.size())) {
                continue;
            }

            const YAML::Node components =
                platformNodes[yamlIndex]["components"];
            if (!components || !components.IsMap()) {
                continue;
            }

            if (PlatformPressureSwitchComponent* pressureSwitch =
                    platform->GetPressureSwitchComponent()) {
                pressureSwitch->SetTargetPlatformIds(
                    ReadPressureSwitchTargetPlatformIds(
                        components["pressureSwitch"]["targets"]));
            }
            if (PlatformLatchedGroupSwitchComponent* groupSwitch =
                    platform->GetLatchedGroupSwitchComponent()) {
                groupSwitch->SetRevealTargets(
                    ReadLatchedSwitchTargets(
                        components["latchedGroupSwitch"]["targets"]));
            }
        }
    }
}

bool StageActorCreateService::AddPlatform(int currentPlanetNum, const std::string& modelPath,
                                          const glm::vec3& scale,
                                          const StageActorPlacement* placement)
{
    if (!CanCreateActor()) {
        return false;
    }

    if (!IsValidPlanetIndex(currentPlanetNum, "platform")) {
        return false;
    }

    YAML::Node config;

    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    EnsureSequence(config, "platforms");

    const int index = static_cast<int>(config["platforms"].size());
    YAML::Node platformNode = mNodeFactory.CreatePlatform(currentPlanetNum, modelPath, scale);
    ApplyPlacementToNode(platformNode, currentPlanetNum, placement);
    platformNode["platformId"] =
        StagePlatformIdentifiers::CreateUniqueId(config);

    config["platforms"].push_back(platformNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mRuntimeCreationService.CreateActor(
        StageActorType::Platform, platformNode, index);
    mRuntimeCreationService.RefreshPhysicsWorld();
    return true;
}

std::optional<std::string> StageActorCreateService::AddPressureSwitchPlatform(
    int currentPlanetNum,
    const std::string& modelPath,
    const glm::vec3& scale,
    const std::string& textureOverridePath,
    const std::string& targetPlatformId,
    const StageActorPlacement* placement)
{
    if (!CanCreateActor() ||
        !IsValidPlanetIndex(currentPlanetNum, "pressure switch")) {
        return std::nullopt;
    }

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return std::nullopt;
    }

    EnsureSequence(config, "platforms");
    const int index = static_cast<int>(config["platforms"].size());
    YAML::Node platformNode =
        mNodeFactory.CreatePlatform(currentPlanetNum, modelPath, scale);
    platformNode["textureOverride"] = textureOverridePath;
    ApplyPlacementToNode(platformNode, currentPlanetNum, placement);
    const std::string switchPlatformId =
        StagePlatformIdentifiers::CreateUniqueId(config);
    platformNode["platformId"] = switchPlatformId;

    YAML::Node pressureSwitch =
        platformNode["components"]["pressureSwitch"];
    pressureSwitch["remainsOnAfterPressed"] = false;
    pressureSwitch["inactiveOpacity"] = 0.25f;
    pressureSwitch["targets"] = YAML::Node(YAML::NodeType::Sequence);

    config["platforms"].push_back(platformNode);
    if (!targetPlatformId.empty() &&
        !StagePlatformConnections::AssignExclusiveSwitchTarget(
            config,
            index,
            targetPlatformId)) {
        return std::nullopt;
    }
    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return std::nullopt;
    }

    if (!mRuntimeCreationService.CreateActor(
            StageActorType::Platform, platformNode, index)) {
        return std::nullopt;
    }
    SynchronizeRuntimeSwitchTargets(config);
    mRuntimeCreationService.RefreshPhysicsWorld();
    return switchPlatformId;
}

bool StageActorCreateService::AddRideMovingPlatform(
    int currentPlanetNum,
    const std::string& modelPath,
    const glm::vec3& scale,
    const StageActorPlacement* placement)
{
    if (!CanCreateActor() ||
        !IsValidPlanetIndex(currentPlanetNum, "ride moving platform")) {
        return false;
    }

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    EnsureSequence(config, "platforms");
    const int index = static_cast<int>(config["platforms"].size());
    YAML::Node platformNode =
        mNodeFactory.CreateRideMovingPlatform(currentPlanetNum, modelPath, scale);
    ApplyPlacementToNode(platformNode, currentPlanetNum, placement);
    platformNode["platformId"] =
        StagePlatformIdentifiers::CreateUniqueId(config);
    config["platforms"].push_back(platformNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mRuntimeCreationService.CreateActor(
        StageActorType::Platform, platformNode, index);
    mRuntimeCreationService.RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::AddMovingPlatform(
    int currentPlanetNum,
    const StageActorPlacement& startPlacement,
    const StageActorPlacement& endPlacement,
    const glm::vec3& scale)
{
    if (!CanCreateActor() || !IsValidPlanetIndex(currentPlanetNum, "moving platform")) {
        return false;
    }
    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) return false;
    EnsureSequence(config, "platforms");
    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();
    const Planet* planet = planets[currentPlanetNum];
    if (!planet) return false;

    YAML::Node node = mNodeFactory.CreatePlatform(currentPlanetNum, "platform.obj", scale);
    ApplyPlacementToNode(node, currentPlanetNum, &startPlacement);
    node["platformId"] = StagePlatformIdentifiers::CreateUniqueId(config);
    const glm::vec3 startLocal = startPlacement.worldPosition - planet->GetPos();
    const glm::vec3 endLocal = endPlacement.worldPosition - planet->GetPos();
    YAML::Node movement = node["components"]["movement"];
    for (int axis = 0; axis < 3; ++axis) {
        movement["startLocalPos"][axis] = startLocal[axis];
        movement["endLocalPos"][axis] = endLocal[axis];
        movement["moveOffset"][axis] = endLocal[axis] - startLocal[axis];
    }
    movement["moveDuration"] = 3.0f;
    movement["moveOnPlayer"] = false;
    movement["returnDelay"] = 0.0f;
    movement["endpointWaitSeconds"] = 0.5f;
    const int index = static_cast<int>(config["platforms"].size());
    config["platforms"].push_back(node);
    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) return false;
    mRuntimeCreationService.CreateActor(
        StageActorType::Platform, node, index);
    mRuntimeCreationService.RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::AddFadingPlatform(
    int currentPlanetNum, const glm::vec3& scale,
    const StageActorPlacement* placement)
{
    if (!CanCreateActor() || !IsValidPlanetIndex(currentPlanetNum, "fading platform")) return false;
    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) return false;
    EnsureSequence(config, "platforms");
    YAML::Node node = mNodeFactory.CreatePlatform(currentPlanetNum, "platform.obj", scale);
    ApplyPlacementToNode(node, currentPlanetNum, placement);
    node["platformId"] = StagePlatformIdentifiers::CreateUniqueId(config);
    node["components"]["fadeOnStand"]["fadeOutDuration"] = 1.0f;
    node["components"]["fadeOnStand"]["reappearDelay"] = 2.0f;
    const int index = static_cast<int>(config["platforms"].size());
    config["platforms"].push_back(node);
    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) return false;
    mRuntimeCreationService.CreateActor(
        StageActorType::Platform, node, index);
    mRuntimeCreationService.RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::AddAdhesivePlatform(
    int currentPlanetNum, const glm::vec3& scale,
    const StageActorPlacement* placement)
{
    if (!CanCreateActor() || !IsValidPlanetIndex(currentPlanetNum, "adhesive platform")) return false;
    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) return false;
    EnsureSequence(config, "platforms");
    YAML::Node node = mNodeFactory.CreatePlatform(currentPlanetNum, "platform.obj", scale);
    ApplyPlacementToNode(node, currentPlanetNum, placement);
    node["platformId"] = StagePlatformIdentifiers::CreateUniqueId(config);
    node["components"]["adhesion"] = YAML::Node(YAML::NodeType::Map);
    const int index = static_cast<int>(config["platforms"].size());
    config["platforms"].push_back(node);
    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) return false;
    mRuntimeCreationService.CreateActor(
        StageActorType::Platform, node, index);
    mRuntimeCreationService.RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::AddTwoPlayerSwitchPair(
    int currentPlanetNum,
    const StageActorPlacement& firstPlacement,
    const StageActorPlacement& secondPlacement,
    const glm::vec3& scale,
    const std::string& textureOverridePath,
    const std::string& targetPlatformId)
{
    if (!CanCreateActor() ||
        !IsValidPlanetIndex(currentPlanetNum, "two-player switch")) {
        return false;
    }
    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) return false;
    EnsureSequence(config, "platforms");
    const std::string switchGroupId =
        CreateUniqueTwoPlayerSwitchGroupId(config["platforms"]);
    for (const StageActorPlacement* placement : {&firstPlacement, &secondPlacement}) {
        YAML::Node node = mNodeFactory.CreatePlatform(
            currentPlanetNum,
            "platform.obj",
            scale);
        node["textureOverride"] = textureOverridePath;
        ApplyPlacementToNode(node, currentPlanetNum, placement);
        node["platformId"] = StagePlatformIdentifiers::CreateUniqueId(config);
        YAML::Node group = node["components"]["latchedGroupSwitch"];
        group["groupId"] = switchGroupId;
        group["targets"] = YAML::Node(YAML::NodeType::Sequence);
        group["hideTargets"] = YAML::Node(YAML::NodeType::Sequence);
        config["platforms"].push_back(node);
    }
    const int firstIndex = static_cast<int>(config["platforms"].size()) - 2;
    if (!targetPlatformId.empty() &&
        !StagePlatformConnections::AssignExclusiveSwitchTarget(
            config,
            firstIndex,
            targetPlatformId)) {
        return false;
    }
    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) return false;





    // 配置コールバック中の全ステージ再読込は選択・配置状態を破棄する。次の更新前に2個を生成できるため、ペアとして完全な状態で観測される。
    const YAML::Node firstNode = config["platforms"][firstIndex];
    const YAML::Node secondNode = config["platforms"][firstIndex + 1];
    if (!mRuntimeCreationService.CreateActor(
            StageActorType::Platform, firstNode, firstIndex) ||
        !mRuntimeCreationService.CreateActor(
            StageActorType::Platform, secondNode, firstIndex + 1)) {
        return false;
    }
    SynchronizeRuntimeSwitchTargets(config);
    mRuntimeCreationService.RefreshPhysicsWorld();
    return true;
}
