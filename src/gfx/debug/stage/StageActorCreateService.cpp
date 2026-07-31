#include "gfx/debug/stage/StageActorCreateService.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Planet.h"
#include "gfx/debug/stage/StageYamlRepository.h"
#include "system/ActorLoadSystem.h"
#include "system/PhysicsSystem.h"

#include <algorithm>
#include <iostream>
#include <unordered_set>

namespace {

std::string CreateUniquePlatformId(const YAML::Node& config)
{
    std::unordered_set<std::string> usedIds;
    if (config && config.IsMap()) {
        for (const auto& entry : config) {
            const YAML::Node sequence = entry.second;
            if (!sequence || !sequence.IsSequence()) {
                continue;
            }
            for (const YAML::Node& node : sequence) {
                if (!node || !node.IsMap()) {
                    continue;
                }
                if (node["platformId"]) {
                    usedIds.insert(node["platformId"].as<std::string>());
                }
                const YAML::Node targets =
                    node["components"]["pressureSwitch"]["targets"];
                if (targets && targets.IsSequence()) {
                    for (const YAML::Node& target : targets) {
                        if (target && target.IsScalar()) {
                            usedIds.insert(target.as<std::string>());
                        }
                    }
                }
            }
        }
    }

    for (int suffix = 1;; ++suffix) {
        const std::string candidate =
            "platform_" + std::to_string(suffix);
        if (!usedIds.contains(candidate)) {
            return candidate;
        }
    }
}

} // namespace

StageActorCreateService::StageActorCreateService(DebugEditorContext& context)
    : mContext(context)
{
}

bool StageActorCreateService::CanCreateActor() const
{
    return mContext.game && mContext.game->GetCurrentStage() && mContext.game->GetActorLoadSystem();
}

bool StageActorCreateService::IsValidPlanetIndex(int planetIndex, const char* label) const
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return false;
    }

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

    if (planetIndex < 0 || planetIndex >= static_cast<int>(planets.size())) {
        std::cerr << "Invalid " << label << " planet index: " << planetIndex << std::endl;
        return false;
    }

    return true;
}

void StageActorCreateService::RefreshPhysicsWorld() const
{
    if (mContext.game && mContext.game->GetPhysicsSystem()) {
        mContext.game->GetPhysicsSystem()->Initialize();
    }
}

void StageActorCreateService::EnsureSequence(YAML::Node& config, const std::string& sequenceName) const
{
    if (!config[sequenceName] || !config[sequenceName].IsSequence()) {
        config[sequenceName] = YAML::Node(YAML::NodeType::Sequence);
    }
}

bool StageActorCreateService::AddPlatform(int currentPlanetNum, const std::string& modelPath, const glm::vec3& scale)
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
    YAML::Node platformNode = CreatePlatformNode(currentPlanetNum, modelPath, scale);
    platformNode["platformId"] = CreateUniquePlatformId(config);

    config["platforms"].push_back(platformNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mContext.game->GetActorLoadSystem()->CreatePlatformFromStageNode(platformNode, index);
    RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::AddRideMovingPlatform(
    int currentPlanetNum,
    const std::string& modelPath,
    const glm::vec3& scale)
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
        CreateRideMovingPlatformNode(currentPlanetNum, modelPath, scale);
    platformNode["platformId"] = CreateUniquePlatformId(config);
    config["platforms"].push_back(platformNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mContext.game->GetActorLoadSystem()->CreatePlatformFromStageNode(
        platformNode, index);
    RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::AddPlanet(const std::string& modelPath)
{
    if (!CanCreateActor()) {
        return false;
    }

    YAML::Node config;

    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    EnsureSequence(config, "planets");

    const int planetIndex = static_cast<int>(config["planets"].size());
    YAML::Node planetNode = CreatePlanetNode(planetIndex, modelPath);

    config["planets"].push_back(planetNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mContext.game->GetActorLoadSystem()->CreatePlanetFromStageNode(planetNode);
    RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::AddEnemy(const std::string& type, int currentPlanetNum)
{
    if (!CanCreateActor()) {
        return false;
    }

    if (!IsValidPlanetIndex(currentPlanetNum, "enemy")) {
        return false;
    }

    YAML::Node config;

    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    EnsureSequence(config, "enemies");

    const int index = static_cast<int>(config["enemies"].size());
    YAML::Node enemyNode = CreateEnemyNode(type, currentPlanetNum);

    config["enemies"].push_back(enemyNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mContext.game->GetActorLoadSystem()->CreateEnemyFromStageNode(enemyNode, index);
    RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::AddNPC(
    const std::string& modelPath,
    int currentPlanetNum,
    const std::string& name,
    const std::vector<std::string>& talkTexts,
    float radius,
    float scale)
{
    if (!CanCreateActor()) {
        return false;
    }

    if (!IsValidPlanetIndex(currentPlanetNum, "NPC")) {
        return false;
    }

    YAML::Node config;

    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    EnsureSequence(config, "NPCs");

    const int index = static_cast<int>(config["NPCs"].size());
    YAML::Node npcNode =
        CreateNPCNode(
            modelPath,
            currentPlanetNum,
            name,
            talkTexts,
            radius,
            scale);

    config["NPCs"].push_back(npcNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mContext.game->GetActorLoadSystem()->CreateNPCFromStageNode(npcNode, index);
    RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::AddTutorialTrigger(
    int currentPlanetNum,
    const std::string& modelPath,
    const std::vector<std::string>& talkTexts,
    const glm::vec3& scale)
{
    if (!CanCreateActor() || modelPath.empty() ||
        !IsValidPlanetIndex(
            currentPlanetNum,
            "tutorial trigger")) {
        return false;
    }

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(
            mContext,
            config)) {
        return false;
    }

    EnsureSequence(config, "tutorialTriggers");
    const int index =
        static_cast<int>(
            config["tutorialTriggers"].size());
    YAML::Node triggerNode =
        CreateTutorialTriggerNode(
            currentPlanetNum,
            modelPath,
            talkTexts,
            scale);
    config["tutorialTriggers"].push_back(triggerNode);

    if (!StageYamlRepository::SaveCurrentStage(
            mContext,
            config)) {
        return false;
    }

    mContext.game
        ->GetActorLoadSystem()
        ->CreateTutorialTriggerFromStageNode(
            triggerNode,
            index);
    RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::AddCrystal(const std::string& type, int currentPlanetNum)
{
    if (!CanCreateActor()) {
        return false;
    }

    if (!IsValidPlanetIndex(currentPlanetNum, "crystal")) {
        return false;
    }

    YAML::Node config;

    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    EnsureSequence(config, "crystals");

    const int index = static_cast<int>(config["crystals"].size());
    YAML::Node crystalNode = CreateCrystalNode(type, currentPlanetNum);

    config["crystals"].push_back(crystalNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mContext.game->GetActorLoadSystem()->CreateCrystalFromStageNode(crystalNode, index);
    RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::AddBoatParts(const std::string& type, int currentPlanetNum)
{
    if (!CanCreateActor()) {
        return false;
    }

    if (!IsValidPlanetIndex(currentPlanetNum, "boat parts")) {
        return false;
    }

    YAML::Node config;

    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    EnsureSequence(config, "boatParts");

    const int index = static_cast<int>(config["boatParts"].size());
    YAML::Node partNode = CreateBoatPartsNode(type, currentPlanetNum);

    config["boatParts"].push_back(partNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mContext.game->GetActorLoadSystem()->CreateBoatPartsFromStageNode(partNode, index);
    RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::AddBoat(int startPlanetNum, int destPlanetNum, int destStage)
{
    if (!CanCreateActor()) {
        return false;
    }

    if (!IsValidPlanetIndex(startPlanetNum, "boat start")) {
        return false;
    }

    if (!IsValidPlanetIndex(destPlanetNum, "boat destination")) {
        return false;
    }

    YAML::Node config;

    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    EnsureSequence(config, "boats");

    const int index = static_cast<int>(config["boats"].size());
    YAML::Node boatNode = CreateBoatNode(startPlanetNum, destPlanetNum, destStage);

    config["boats"].push_back(boatNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mContext.game->GetActorLoadSystem()->CreateBoatFromStageNode(boatNode, index);
    RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::AddStar(int currentPlanetNum)
{
    if (!CanCreateActor()) {
        return false;
    }

    if (!IsValidPlanetIndex(currentPlanetNum, "star")) {
        return false;
    }

    YAML::Node config;

    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    EnsureSequence(config, "star");

    const int index = static_cast<int>(config["star"].size());
    YAML::Node starNode = CreateStarNode(currentPlanetNum);

    config["star"].push_back(starNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mContext.game->GetActorLoadSystem()->CreateStarFromStageNode(starNode, index);
    RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::AddStageObject(
    int currentPlanetNum,
    const std::string& modelPath,
    bool collisionEnabled)
{
    if (!CanCreateActor() || modelPath.empty()) {
        return false;
    }

    if (!IsValidPlanetIndex(currentPlanetNum, "stage object")) {
        return false;
    }

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    EnsureSequence(config, "stageObjects");

    const int index = static_cast<int>(config["stageObjects"].size());
    YAML::Node stageObjectNode =
        CreateStageObjectNode(currentPlanetNum, modelPath, collisionEnabled);
    config["stageObjects"].push_back(stageObjectNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mContext.game->GetActorLoadSystem()->CreateStageObjectFromStageNode(stageObjectNode, index);
    RefreshPhysicsWorld();
    return true;
}

YAML::Node StageActorCreateService::CreatePlatformNode(int currentPlanetNum, const std::string& modelPath,
                                                       const glm::vec3& scale) const
{
    YAML::Node node;

    node["currentPlanetNum"] = currentPlanetNum;
    node["theta"] = 0.0f;
    node["phi"] = 0.0f;
    node["height"] = 1.0f;

    node["facingYaw"] = 0.0f;

    node["rotation"][0] = 0.0f;
    node["rotation"][1] = 0.0f;
    node["rotation"][2] = 0.0f;

    node["scale"][0] = scale.x;
    node["scale"][1] = scale.y;
    node["scale"][2] = scale.z;

    node["modelPath"] = modelPath;

    return node;
}

YAML::Node StageActorCreateService::CreateRideMovingPlatformNode(
    int currentPlanetNum,
    const std::string& modelPath,
    const glm::vec3& scale) const
{
    YAML::Node node =
        CreatePlatformNode(currentPlanetNum, modelPath, scale);
    YAML::Node movement = node["components"]["movement"];
    movement["moveOnPlayer"] = true;
    movement["moveDuration"] = 3.0f;
    movement["returnDelay"] = 1.0f;
    movement["moveOffset"][0] = 0.0f;
    movement["moveOffset"][1] = 5.0f;
    movement["moveOffset"][2] = 0.0f;
    return node;
}

YAML::Node StageActorCreateService::CreatePlanetNode(int planetIndex, const std::string& modelPath) const
{
    YAML::Node node;

    node["center"][0] = static_cast<float>(planetIndex) * 32.0f;
    node["center"][1] = 0.0f;
    node["center"][2] = 0.0f;

    node["scale"][0] = 4.0f;
    node["scale"][1] = 4.0f;
    node["scale"][2] = 4.0f;

    node["color"][0] = 1.0f;
    node["color"][1] = 1.0f;
    node["color"][2] = 1.0f;
    node["color"][3] = 1.0f;

    node["model"] = modelPath;
    node["shape"] = "Sphere";
    node["stageNum"] = planetIndex;
    node["rocketSpawnCondition"] = "";

    return node;
}

YAML::Node StageActorCreateService::CreateEnemyNode(const std::string& type, int currentPlanetNum) const
{
    YAML::Node node;

    node["editorName"] = type == "boss" ? "新しいボス敵" : "新しい通常敵";
    node["type"] = type;
    node["currentPlanetNum"] = currentPlanetNum;
    node["theta"] = 0.0f;
    node["phi"] = 0.0f;
    node["height"] = 1.0f;

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();
    const Planet* planet = planets[currentPlanetNum];
    const float initialHeight = 1.0f;
    const float initialDistance = planet ? planet->GetRadius() + initialHeight : 1.0f;

    node["pos"][0] = initialDistance;
    node["pos"][1] = 0.0f;
    node["pos"][2] = 0.0f;

    return node;
}

YAML::Node StageActorCreateService::CreateNPCNode(
    const std::string& modelPath,
    int currentPlanetNum,
    const std::string& name,
    const std::vector<std::string>& talkTexts,
    float radius,
    float scale) const
{
    YAML::Node node;

    node["modelPath"] = modelPath;
    node["currentPlanetNum"] = currentPlanetNum;
    node["theta"] = 0.0f;
    node["phi"] = 0.0f;
    node["height"] = 1.0f;
    node["facingYaw"] = 0.0f;
    node["radius"] = std::max(0.1f, radius);
    const float safeScale = std::max(0.01f, scale);
    node["scale"][0] = safeScale;
    node["scale"][1] = safeScale;
    node["scale"][2] = safeScale;
    node["name"] = name;
    for (const std::string& talkText : talkTexts) {
        node["talkTexts"].push_back(talkText);
    }
    if (talkTexts.empty()) {
        node["talkTexts"].push_back("");
    }

    return node;
}

YAML::Node
StageActorCreateService::CreateTutorialTriggerNode(
    int currentPlanetNum,
    const std::string& modelPath,
    const std::vector<std::string>& talkTexts,
    const glm::vec3& scale) const
{
    YAML::Node node;
    node["modelPath"] = modelPath;
    node["currentPlanetNum"] = currentPlanetNum;
    node["theta"] = 0.0f;
    node["phi"] = 0.0f;
    node["height"] = 1.0f;
    node["scale"][0] = std::max(0.01f, scale.x);
    node["scale"][1] = std::max(0.01f, scale.y);
    node["scale"][2] = std::max(0.01f, scale.z);

    for (const std::string& talkText : talkTexts) {
        node["talkTexts"].push_back(talkText);
    }
    if (talkTexts.empty()) {
        node["talkTexts"].push_back("");
    }

    return node;
}

YAML::Node StageActorCreateService::CreateCrystalNode(const std::string& type, int currentPlanetNum) const
{
    YAML::Node node;

    node["type"] = type;
    node["currentPlanetNum"] = currentPlanetNum;
    node["theta"] = 0.0f;
    node["phi"] = 0.0f;
    node["height"] = 1.0f;

    return node;
}

YAML::Node StageActorCreateService::CreateBoatPartsNode(const std::string& type, int currentPlanetNum) const
{
    YAML::Node node;

    node["type"] = type;
    node["currentPlanetNum"] = currentPlanetNum;
    node["theta"] = 0.0f;
    node["phi"] = 0.0f;
    node["height"] = 1.0f;

    return node;
}

YAML::Node StageActorCreateService::CreateBoatNode(int startPlanetNum, int destPlanetNum, int destStage) const
{
    YAML::Node node;

    node["startPlanet"] = startPlanetNum;
    node["destPlanet"] = destPlanetNum;
    node["destStage"] = destStage;

    node["theta"] = 0.0f;
    node["phi"] = 0.0f;
    node["height"] = 1.0f;
    node["facingYaw"] = 0.0f;

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();
    const Planet* planet = planets[startPlanetNum];
    const float initialHeight = 1.0f;
    const float initialDistance = planet ? planet->GetRadius() + initialHeight : 1.0f;

    node["pos"][0] = initialDistance;
    node["pos"][1] = 0.0f;
    node["pos"][2] = 0.0f;

    return node;
}

YAML::Node StageActorCreateService::CreateStarNode(int currentPlanetNum) const
{
    YAML::Node node;

    node["currentPlanetNum"] = currentPlanetNum;
    node["theta"] = 0.0f;
    node["phi"] = 0.0f;
    node["height"] = 1.0f;
    node["isActive"] = true;

    return node;
}

YAML::Node StageActorCreateService::CreateStageObjectNode(
    int currentPlanetNum,
    const std::string& modelPath,
    bool collisionEnabled) const
{
    YAML::Node node;

    node["currentPlanetNum"] = currentPlanetNum;
    node["theta"] = 0.0f;
    node["phi"] = 0.0f;
    node["height"] = 1.0f;
    node["facingYaw"] = 0.0f;
    node["modelPath"] = modelPath;
    node["collision"] = collisionEnabled;

    node["rotation"][0] = 0.0f;
    node["rotation"][1] = 0.0f;
    node["rotation"][2] = 0.0f;

    node["scale"][0] = 1.0f;
    node["scale"][1] = 1.0f;
    node["scale"][2] = 1.0f;

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();
    const Planet* planet = planets[currentPlanetNum];
    const float initialDistance = planet ? planet->GetRadius() + 1.0f : 1.0f;

    node["pos"][0] = initialDistance;
    node["pos"][1] = 0.0f;
    node["pos"][2] = 0.0f;

    return node;
}
