#include "gfx/debug/stage/StageActorCreateService.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Planet.h"
#include "gfx/debug/stage/StagePlatformIdentifiers.h"
#include "gfx/debug/stage/StageYamlRepository.h"

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

bool StageActorCreateService::AddPressureSwitchPlatform(
    int currentPlanetNum,
    const std::string& modelPath,
    const glm::vec3& scale,
    const StageActorPlacement* placement)
{
    if (!CanCreateActor() ||
        !IsValidPlanetIndex(currentPlanetNum, "pressure switch")) {
        return false;
    }

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    EnsureSequence(config, "platforms");
    const int index = static_cast<int>(config["platforms"].size());
    YAML::Node platformNode =
        mNodeFactory.CreatePlatform(currentPlanetNum, modelPath, scale);
    ApplyPlacementToNode(platformNode, currentPlanetNum, placement);
    platformNode["platformId"] =
        StagePlatformIdentifiers::CreateUniqueId(config);

    YAML::Node pressureSwitch =
        platformNode["components"]["pressureSwitch"];
    pressureSwitch["remainsOnAfterPressed"] = false;
    pressureSwitch["inactiveOpacity"] = 0.25f;
    pressureSwitch["targets"] = YAML::Node(YAML::NodeType::Sequence);

    config["platforms"].push_back(platformNode);
    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mRuntimeCreationService.CreateActor(
        StageActorType::Platform, platformNode, index);
    mRuntimeCreationService.RefreshPhysicsWorld();
    return true;
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
    const StageActorPlacement& secondPlacement)
{
    if (!CanCreateActor() || !IsValidPlanetIndex(currentPlanetNum, "two-player switch")) return false;
    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) return false;
    EnsureSequence(config, "platforms");
    for (const YAML::Node& node : config["platforms"]) {
        const YAML::Node group = node["components"]["latchedGroupSwitch"];
        if (group && group["groupId"] && group["groupId"].as<std::string>() == "ugc_two_player_pair") {
            return false;
        }
    }
    for (const StageActorPlacement* placement : {&firstPlacement, &secondPlacement}) {
        YAML::Node node = mNodeFactory.CreatePlatform(
            currentPlanetNum,
            "platform.obj",
            glm::vec3(0.75f, 0.2f, 0.75f));
        ApplyPlacementToNode(node, currentPlanetNum, placement);
        node["platformId"] = StagePlatformIdentifiers::CreateUniqueId(config);
        YAML::Node group = node["components"]["latchedGroupSwitch"];
        group["groupId"] = "ugc_two_player_pair";
        group["targets"] = YAML::Node(YAML::NodeType::Sequence);
        group["hideTargets"] = YAML::Node(YAML::NodeType::Sequence);
        config["platforms"].push_back(node);
    }
    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) return false;





    // 配置コールバック中の全ステージ再読込は選択・配置状態を破棄する。次の更新前に2個を生成できるため、ペアとして完全な状態で観測される。
    const int firstIndex = static_cast<int>(config["platforms"].size()) - 2;
    const YAML::Node firstNode = config["platforms"][firstIndex];
    const YAML::Node secondNode = config["platforms"][firstIndex + 1];
    if (!mRuntimeCreationService.CreateActor(
            StageActorType::Platform, firstNode, firstIndex) ||
        !mRuntimeCreationService.CreateActor(
            StageActorType::Platform, secondNode, firstIndex + 1)) {
        return false;
    }
    mRuntimeCreationService.RefreshPhysicsWorld();
    return true;
}

