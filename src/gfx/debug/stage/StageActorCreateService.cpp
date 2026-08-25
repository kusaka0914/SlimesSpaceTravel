#include "gfx/debug/stage/StageActorCreateService.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "gfx/debug/stage/StageYamlRepository.h"
#include "system/ActorLoadSystem.h"
#include "system/PhysicsSystem.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <tuple>
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

                if (node["platformId"] && node["platformId"].IsScalar()) {
                    usedIds.insert(node["platformId"].as<std::string>());
                }

                const YAML::Node components = node["components"];
                if (!components || !components.IsMap()) {
                    continue;
                }

                const YAML::Node pressureSwitch = components["pressureSwitch"];
                if (!pressureSwitch || !pressureSwitch.IsMap()) {
                    continue;
                }

                const YAML::Node targets = pressureSwitch["targets"];
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

bool TryReadVec3(
    const YAML::Node& node,
    const char* key,
    glm::vec3& outValue)
{
    const YAML::Node value = node[key];
    if (!value || !value.IsSequence() || value.size() < 3) {
        return false;
    }

    try {
        outValue = glm::vec3(
            value[0].as<float>(),
            value[1].as<float>(),
            value[2].as<float>());
        return true;
    } catch (const YAML::Exception&) {
        return false;
    }
}

void ShiftVec3IfPresent(
    YAML::Node node,
    const char* key,
    const glm::vec3& offset)
{
    glm::vec3 value;
    if (!TryReadVec3(node, key, value)) {
        return;
    }

    value += offset;
    node[key][0] = value.x;
    node[key][1] = value.y;
    node[key][2] = value.z;
}

void ShiftPlatformMovementEndpoints(
    YAML::Node& platformNode,
    const glm::vec3& offset)
{
    ShiftVec3IfPresent(platformNode, "startLocalPos", offset);
    ShiftVec3IfPresent(platformNode, "endLocalPos", offset);

    const YAML::Node components = platformNode["components"];
    if (!components || !components.IsMap()) {
        return;
    }

    YAML::Node movement = components["movement"];
    if (!movement || !movement.IsMap()) {
        return;
    }

    ShiftVec3IfPresent(movement, "startLocalPos", offset);
    ShiftVec3IfPresent(movement, "endLocalPos", offset);
}

constexpr const char* UGCPlatformCellsKey = "ugcPlatformCells";
constexpr const char* UGCGeneratedPlatformKey = "ugcGeneratedPlatform";

struct UGCPlatformCell {
    int planetIndex = 0;
    float gridSize = 1.0f;
    glm::ivec3 gridPosition{0};
    std::string behavior = "normal";
    glm::ivec3 movementDeltaCells{0};
};

bool TryReadUGCPlatformCell(
    const YAML::Node& node,
    UGCPlatformCell& outCell)
{
    if (!node || !node.IsMap() ||
        !node["planetIndex"] ||
        !node["gridSize"] ||
        !node["gridPosition"] ||
        !node["gridPosition"].IsSequence() ||
        node["gridPosition"].size() < 3) {
        return false;
    }

    try {
        outCell.planetIndex = node["planetIndex"].as<int>();
        outCell.gridSize = std::max(0.01f, node["gridSize"].as<float>());
        outCell.gridPosition = glm::ivec3(
            node["gridPosition"][0].as<int>(),
            node["gridPosition"][1].as<int>(),
            node["gridPosition"][2].as<int>());
        if (node["behavior"] && node["behavior"].IsScalar()) {
            outCell.behavior = node["behavior"].as<std::string>();
        }
        if (node["movementDeltaCells"] &&
            node["movementDeltaCells"].IsSequence() &&
            node["movementDeltaCells"].size() >= 3) {
            outCell.movementDeltaCells = glm::ivec3(
                node["movementDeltaCells"][0].as<int>(),
                node["movementDeltaCells"][1].as<int>(),
                node["movementDeltaCells"][2].as<int>());
        }
        return true;
    } catch (const YAML::Exception&) {
        return false;
    }
}

YAML::Node CreateUGCPlatformCellNode(const UGCPlatformCell& cell)
{
    YAML::Node node;
    node["planetIndex"] = cell.planetIndex;
    node["gridSize"] = cell.gridSize;
    node["gridPosition"][0] = cell.gridPosition.x;
    node["gridPosition"][1] = cell.gridPosition.y;
    node["gridPosition"][2] = cell.gridPosition.z;
    node["behavior"] = cell.behavior;
    if (cell.behavior == "moving") {
        node["movementDeltaCells"][0] = cell.movementDeltaCells.x;
        node["movementDeltaCells"][1] = cell.movementDeltaCells.y;
        node["movementDeltaCells"][2] = cell.movementDeltaCells.z;
    }
    return node;
}

glm::vec3 CalculateUGCCellWorldPosition(const UGCPlatformCell& cell)
{
    return glm::vec3(
        (static_cast<float>(cell.gridPosition.x) + 0.5f) * cell.gridSize,
        static_cast<float>(cell.gridPosition.y) * cell.gridSize,
        (static_cast<float>(cell.gridPosition.z) + 0.5f) * cell.gridSize);
}

}

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

void StageActorCreateService::ApplyPlacementToNode(
    YAML::Node& node,
    int planetIndex,
    const StageActorPlacement* placement) const
{
    if (!placement || !IsValidPlanetIndex(planetIndex, "placement")) {
        return;
    }

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();
    const Planet* planet = planets[planetIndex];
    if (!planet) {
        return;
    }

    const glm::vec3 localPosition = placement->worldPosition - planet->GetPos();
    node["pos"][0] = localPosition.x;
    node["pos"][1] = localPosition.y;
    node["pos"][2] = localPosition.z;

    const float localDistance = glm::length(localPosition);
    if (localDistance > 1e-6f) {
        const glm::vec3 radialDirection = localPosition / localDistance;
        node["theta"] = std::atan2(radialDirection.z, radialDirection.x);
        node["phi"] = std::asin(std::clamp(radialDirection.y, -1.0f, 1.0f));
        node["height"] = localDistance - planet->GetRadius();
    }

    glm::vec3 surfaceNormal = placement->surfaceNormal;
    if (glm::length(surfaceNormal) < 1e-6f) {
        surfaceNormal = localDistance > 1e-6f
                            ? localPosition / localDistance
                            : glm::vec3(0.0f, 1.0f, 0.0f);
    }
    surfaceNormal = glm::normalize(surfaceNormal);
    node["upVec"][0] = surfaceNormal.x;
    node["upVec"][1] = surfaceNormal.y;
    node["upVec"][2] = surfaceNormal.z;
}

void StageActorCreateService::EnsureSequence(YAML::Node& config, const std::string& sequenceName) const
{
    if (!config[sequenceName] || !config[sequenceName].IsSequence()) {
        config[sequenceName] = YAML::Node(YAML::NodeType::Sequence);
    }
}

bool StageActorCreateService::DuplicateActorAtPlacement(
    const StageActorRef& sourceRef,
    const YAML::Node& sourceNode,
    int targetPlanetIndex,
    const StageActorPlacement& placement)
{
    if (!CanCreateActor() ||
        !sourceNode ||
        !sourceNode.IsMap() ||
        sourceRef.sequenceName.empty() ||
        !IsValidPlanetIndex(targetPlanetIndex, "duplicated actor")) {
        return false;
    }

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    EnsureSequence(config, sourceRef.sequenceName);

    YAML::Node duplicatedNode = YAML::Clone(sourceNode);
    glm::vec3 previousLocalPosition(0.0f);
    const bool hadPreviousLocalPosition =
        TryReadVec3(duplicatedNode, "pos", previousLocalPosition);

    if (sourceRef.type == StageActorType::Boat) {
        duplicatedNode["startPlanet"] = targetPlanetIndex;
        duplicatedNode.remove("currentPlanetNum");
    } else {
        duplicatedNode["currentPlanetNum"] = targetPlanetIndex;
    }

    ApplyPlacementToNode(
        duplicatedNode,
        targetPlanetIndex,
        &placement);




    // 保存済みQuaternionは複製元の地表法線を含む。クリック先の法線で再構成してもローカルの向きを保つため削除する。
    duplicatedNode.remove("rotationQuat");

    if (sourceRef.type == StageActorType::Platform) {
        duplicatedNode["platformId"] =
            CreateUniquePlatformId(config);

        glm::vec3 newLocalPosition(0.0f);
        if (hadPreviousLocalPosition &&
            TryReadVec3(duplicatedNode, "pos", newLocalPosition)) {
            ShiftPlatformMovementEndpoints(
                duplicatedNode,
                newLocalPosition - previousLocalPosition);
        }
    }

    YAML::Node targetSequence = config[sourceRef.sequenceName];
    const int newYamlIndex =
        static_cast<int>(targetSequence.size());
    targetSequence.push_back(duplicatedNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    if (!CreateActorFromStageNode(
            sourceRef,
            duplicatedNode,
            newYamlIndex)) {
        return false;
    }

    RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::CreateActorFromStageNode(
    const StageActorRef& actorRef,
    const YAML::Node& actorNode,
    int stageYamlIndex) const
{
    ActorLoadSystem* actorLoadSystem =
        mContext.game ? mContext.game->GetActorLoadSystem() : nullptr;
    if (!actorLoadSystem) {
        return false;
    }

    switch (actorRef.type) {
    case StageActorType::Enemy:
        return actorLoadSystem->CreateEnemyFromStageNode(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::Platform: {
        Platform* platform =
            actorRef.sequenceName == "movingPlatforms"
                ? actorLoadSystem->CreateLegacyMovingPlatformFromStageNode(
                      actorNode, stageYamlIndex)
                : actorLoadSystem->CreatePlatformFromStageNode(
                      actorNode, stageYamlIndex);
        if (!platform) {
            return false;
        }
        platform->SetStageSequenceName(actorRef.sequenceName);
        return true;
    }
    case StageActorType::Crystal:
        return actorLoadSystem->CreateCrystalFromStageNode(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::NPC:
        return actorLoadSystem->CreateNPCFromStageNode(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::BoatParts:
        return actorLoadSystem->CreateBoatPartsFromStageNode(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::Boat:
        return actorLoadSystem->CreateBoatFromStageNode(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::BoatArrivalPoint:
        return actorLoadSystem->CreateBoatArrivalPointFromStageNode(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::FallRespawnPoint:
        return actorLoadSystem->CreateFallRespawnPointFromStageNode(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::Key:
        return actorLoadSystem->CreateKeyFromStageNode(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::Star:
        return actorLoadSystem->CreateStarFromStageNode(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::StageObject:
        return actorLoadSystem->CreateStageObjectFromStageNode(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::TutorialTrigger:
        return actorLoadSystem->CreateTutorialTriggerFromStageNode(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::JewelItem:
        return actorLoadSystem->CreateJewelItemFromStageNode(
                   actorNode, stageYamlIndex) != nullptr;
    case StageActorType::HazardActor:
        return actorLoadSystem->CreateHazardActorFromStageNode(
                   actorNode, stageYamlIndex) != nullptr;
    }

    return false;
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
    YAML::Node platformNode = CreatePlatformNode(currentPlanetNum, modelPath, scale);
    ApplyPlacementToNode(platformNode, currentPlanetNum, placement);
    platformNode["platformId"] = CreateUniquePlatformId(config);

    config["platforms"].push_back(platformNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mContext.game->GetActorLoadSystem()->CreatePlatformFromStageNode(platformNode, index);
    RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::AddUGCPlatformCell(
    int currentPlanetNum,
    const StageActorPlacement& placement,
    float gridSize,
    int footprintSideLength,
    const std::string& behavior,
    const glm::ivec3& movementDeltaCells)
{
    if (!CanCreateActor() ||
        !IsValidPlanetIndex(currentPlanetNum, "UGC platform cell")) {
        return false;
    }

    const float safeGridSize = std::max(0.01f, gridSize);
    UGCPlatformCell newCell;
    newCell.planetIndex = currentPlanetNum;
    newCell.gridSize = safeGridSize;
    newCell.gridPosition = glm::ivec3(
        static_cast<int>(std::floor(
            placement.worldPosition.x / safeGridSize)),
        static_cast<int>(std::round(
            placement.worldPosition.y / safeGridSize)),
        static_cast<int>(std::floor(
            placement.worldPosition.z / safeGridSize)));
    newCell.behavior = behavior;
    newCell.movementDeltaCells = movementDeltaCells;

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    EnsureSequence(config, UGCPlatformCellsKey);
    const int safeSideLength = std::clamp(footprintSideLength, 1, 3);
    for (int offsetX = 0; offsetX < safeSideLength; ++offsetX) {
        for (int offsetZ = 0; offsetZ < safeSideLength; ++offsetZ) {
            UGCPlatformCell cell = newCell;

            cell.gridPosition.x -= offsetX;
            cell.gridPosition.z -= offsetZ;
            bool alreadyExists = false;
            for (const YAML::Node& cellNode : config[UGCPlatformCellsKey]) {
                UGCPlatformCell existingCell;
                if (TryReadUGCPlatformCell(cellNode, existingCell) &&
                    existingCell.planetIndex == cell.planetIndex &&
                    std::abs(existingCell.gridSize - cell.gridSize) < 0.0001f &&
                    existingCell.gridPosition == cell.gridPosition) {
                    alreadyExists = true;
                    break;
                }
            }
            if (!alreadyExists) {
                config[UGCPlatformCellsKey].push_back(
                    CreateUGCPlatformCellNode(cell));
            }
        }
    }
    if (!RebuildUGCPlatformNodes(config) ||
        !StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mContext.game->ReloadCurrentStage(false);
    return true;
}

bool StageActorCreateService::RefreshUGCPlatformCells()
{
    if (!CanCreateActor()) {
        return false;
    }

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config) ||
        !RebuildUGCPlatformNodes(config) ||
        !StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mContext.game->ReloadCurrentStage(false);
    return true;
}

bool StageActorCreateService::TranslateUGCPlatformCells(
    const StageActorRef& generatedPlatformRef,
    const glm::vec3& worldDelta)
{
    return TranslateUGCPlatformCells(
        std::vector<StageActorRef>{generatedPlatformRef}, worldDelta);
}

bool StageActorCreateService::TranslateUGCPlatformCells(
    const std::vector<StageActorRef>& generatedPlatformRefs,
    const glm::vec3& worldDelta)
{
    if (!CanCreateActor() || generatedPlatformRefs.empty()) {
        return false;
    }

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }
    const YAML::Node platforms = config["platforms"];
    if (!platforms || !platforms.IsSequence()) {
        return false;
    }

    struct SelectedCellRegion {
        int planetIndex = -1;
        float gridSize = 1.0f;
        int gridLayer = 0;
        int minimumX = 0;
        int minimumZ = 0;
        int maximumX = 0;
        int maximumZ = 0;
        glm::ivec3 gridDelta{0};
    };
    std::vector<SelectedCellRegion> selectedRegions;
    selectedRegions.reserve(generatedPlatformRefs.size());

    for (const StageActorRef& generatedPlatformRef : generatedPlatformRefs) {
        if (generatedPlatformRef.sequenceName != "platforms" ||
            generatedPlatformRef.yamlIndex < 0 ||
            generatedPlatformRef.yamlIndex >=
                static_cast<int>(platforms.size())) {
            continue;
        }

        const YAML::Node platformNode =
            platforms[generatedPlatformRef.yamlIndex];
        const YAML::Node cellMin = platformNode["ugcCellMin"];
        const YAML::Node cellMax = platformNode["ugcCellMax"];
        if (!platformNode[UGCGeneratedPlatformKey] ||
            !platformNode[UGCGeneratedPlatformKey].as<bool>(false) ||
            !cellMin || !cellMin.IsSequence() || cellMin.size() < 2 ||
            !cellMax || !cellMax.IsSequence() || cellMax.size() < 2) {
            continue;
        }

        SelectedCellRegion region;
        region.planetIndex =
            platformNode["currentPlanetNum"].as<int>(-1);
        region.gridSize = platformNode["ugcGridSize"].as<float>(1.0f);
        region.gridLayer = platformNode["ugcGridLayer"].as<int>(0);
        region.minimumX = cellMin[0].as<int>();
        region.minimumZ = cellMin[1].as<int>();
        region.maximumX = cellMax[0].as<int>();
        region.maximumZ = cellMax[1].as<int>();
        region.gridDelta = glm::ivec3(
            static_cast<int>(std::round(worldDelta.x / region.gridSize)),
            static_cast<int>(std::round(worldDelta.y / region.gridSize)),
            static_cast<int>(std::round(worldDelta.z / region.gridSize)));
        selectedRegions.emplace_back(region);
    }

    if (selectedRegions.empty()) {
        return false;
    }

    YAML::Node cells = config[UGCPlatformCellsKey];
    if (!cells || !cells.IsSequence()) {
        return false;
    }
    for (YAML::Node cellNode : cells) {
        UGCPlatformCell cell;
        if (!TryReadUGCPlatformCell(cellNode, cell)) {
            continue;
        }

        for (const SelectedCellRegion& region : selectedRegions) {
            const bool belongsToRegion =
                cell.planetIndex == region.planetIndex &&
                std::abs(cell.gridSize - region.gridSize) < 0.0001f &&
                cell.gridPosition.y == region.gridLayer &&
                cell.gridPosition.x >= region.minimumX &&
                cell.gridPosition.x <= region.maximumX &&
                cell.gridPosition.z >= region.minimumZ &&
                cell.gridPosition.z <= region.maximumZ;
            if (!belongsToRegion) {
                continue;
            }

            cell.gridPosition += region.gridDelta;
            cellNode["gridPosition"][0] = cell.gridPosition.x;
            cellNode["gridPosition"][1] = cell.gridPosition.y;
            cellNode["gridPosition"][2] = cell.gridPosition.z;
            break;
        }
    }

    if (!RebuildUGCPlatformNodes(config) ||
        !StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }
    mContext.game->ReloadCurrentStage(false);
    return true;
}

bool StageActorCreateService::RemoveUGCPlatformCell(
    const StageActorRef& generatedPlatformRef,
    const glm::vec3& hitPosition)
{
    if (!CanCreateActor() ||
        generatedPlatformRef.sequenceName != "platforms" ||
        generatedPlatformRef.yamlIndex < 0) {
        return false;
    }

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    const YAML::Node platforms = config["platforms"];
    if (!platforms || !platforms.IsSequence() ||
        generatedPlatformRef.yamlIndex >= static_cast<int>(platforms.size())) {
        return false;
    }

    const YAML::Node generatedPlatform =
        platforms[generatedPlatformRef.yamlIndex];
    if (!generatedPlatform[UGCGeneratedPlatformKey] ||
        !generatedPlatform[UGCGeneratedPlatformKey].as<bool>(false)) {
        return false;
    }

    const int planetIndex =
        generatedPlatform["currentPlanetNum"].as<int>(-1);
    const float gridSize =
        generatedPlatform["ugcGridSize"].as<float>(1.0f);
    const int gridLayer =
        generatedPlatform["ugcGridLayer"].as<int>(0);

    const YAML::Node cells = config[UGCPlatformCellsKey];
    if (!cells || !cells.IsSequence()) {
        return false;
    }

    int closestCellIndex = -1;
    float closestDistanceSquared = std::numeric_limits<float>::max();
    for (std::size_t cellIndex = 0; cellIndex < cells.size(); ++cellIndex) {
        UGCPlatformCell cell;
        if (!TryReadUGCPlatformCell(cells[cellIndex], cell) ||
            cell.planetIndex != planetIndex ||
            std::abs(cell.gridSize - gridSize) >= 0.0001f ||
            cell.gridPosition.y != gridLayer) {
            continue;
        }

        const glm::vec3 difference =
            CalculateUGCCellWorldPosition(cell) - hitPosition;
        const float horizontalDistanceSquared =
            difference.x * difference.x + difference.z * difference.z;
        if (horizontalDistanceSquared < closestDistanceSquared) {
            closestDistanceSquared = horizontalDistanceSquared;
            closestCellIndex = static_cast<int>(cellIndex);
        }
    }

    if (closestCellIndex < 0) {
        return false;
    }

    YAML::Node remainingCells(YAML::NodeType::Sequence);
    for (std::size_t cellIndex = 0; cellIndex < cells.size(); ++cellIndex) {
        if (static_cast<int>(cellIndex) != closestCellIndex) {
            remainingCells.push_back(YAML::Clone(cells[cellIndex]));
        }
    }
    config[UGCPlatformCellsKey] = remainingCells;

    if (!RebuildUGCPlatformNodes(config) ||
        !StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mContext.game->ReloadCurrentStage(false);
    return true;
}

bool StageActorCreateService::RemoveUGCPlatformCellAtGridPosition(
    int planetIndex,
    const glm::vec3& worldPosition,
    float gridSize,
    int gridLayer)
{
    if (!CanCreateActor()) {
        return false;
    }

    const float safeGridSize = std::max(0.01f, gridSize);
    const glm::ivec3 targetGridPosition(
        static_cast<int>(std::floor(worldPosition.x / safeGridSize)),
        gridLayer,
        static_cast<int>(std::floor(worldPosition.z / safeGridSize)));

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    const YAML::Node cells = config[UGCPlatformCellsKey];
    if (!cells || !cells.IsSequence()) {
        return false;
    }

    int targetCellIndex = -1;
    for (std::size_t cellIndex = 0; cellIndex < cells.size(); ++cellIndex) {
        UGCPlatformCell cell;
        if (TryReadUGCPlatformCell(cells[cellIndex], cell) &&
            cell.planetIndex == planetIndex &&
            std::abs(cell.gridSize - safeGridSize) < 0.0001f &&
            cell.gridPosition == targetGridPosition) {
            targetCellIndex = static_cast<int>(cellIndex);
            break;
        }
    }
    if (targetCellIndex < 0) {
        return false;
    }

    YAML::Node remainingCells(YAML::NodeType::Sequence);
    for (std::size_t cellIndex = 0; cellIndex < cells.size(); ++cellIndex) {
        if (static_cast<int>(cellIndex) != targetCellIndex) {
            remainingCells.push_back(YAML::Clone(cells[cellIndex]));
        }
    }
    config[UGCPlatformCellsKey] = remainingCells;

    if (!RebuildUGCPlatformNodes(config) ||
        !StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mContext.game->ReloadCurrentStage(false);
    return true;
}

bool StageActorCreateService::ResolveUGCPlatformLayerAtGridPosition(
    int planetIndex,
    const glm::vec3& worldPosition,
    float gridSize,
    int preferredGridLayer,
    int& outGridLayer) const
{
    const float safeGridSize = std::max(0.01f, gridSize);
    const int targetX = static_cast<int>(
        std::floor(worldPosition.x / safeGridSize));
    const int targetZ = static_cast<int>(
        std::floor(worldPosition.z / safeGridSize));

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }
    const YAML::Node cells = config[UGCPlatformCellsKey];
    if (!cells || !cells.IsSequence()) {
        return false;
    }

    std::set<int> matchingLayers;
    for (const YAML::Node& cellNode : cells) {
        UGCPlatformCell cell;
        if (!TryReadUGCPlatformCell(cellNode, cell) ||
            cell.planetIndex != planetIndex ||
            std::abs(cell.gridSize - safeGridSize) >= 0.0001f ||
            cell.gridPosition.x != targetX ||
            cell.gridPosition.z != targetZ) {
            continue;
        }
        matchingLayers.insert(cell.gridPosition.y);
    }

    if (matchingLayers.contains(preferredGridLayer)) {
        outGridLayer = preferredGridLayer;
        return true;
    }
    if (matchingLayers.size() == 1) {
        outGridLayer = *matchingLayers.begin();
        return true;
    }
    return false;
}

int StageActorCreateService::ResolveUGCPlatformPlacementLayerAtGridPosition(
    int planetIndex,
    const glm::vec3& worldPosition,
    float gridSize,
    int emptyColumnGridLayer) const
{
    const float safeGridSize = std::max(0.01f, gridSize);
    const int targetX = static_cast<int>(
        std::floor(worldPosition.x / safeGridSize));
    const int targetZ = static_cast<int>(
        std::floor(worldPosition.z / safeGridSize));

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return emptyColumnGridLayer;
    }
    const YAML::Node cells = config[UGCPlatformCellsKey];
    if (!cells || !cells.IsSequence()) {
        return emptyColumnGridLayer;
    }

    std::optional<int> highestLayer;
    for (const YAML::Node& cellNode : cells) {
        UGCPlatformCell cell;
        if (!TryReadUGCPlatformCell(cellNode, cell) ||
            cell.planetIndex != planetIndex ||
            std::abs(cell.gridSize - safeGridSize) >= 0.0001f ||
            cell.gridPosition.x != targetX ||
            cell.gridPosition.z != targetZ) {
            continue;
        }
        highestLayer = highestLayer
            ? std::max(*highestLayer, cell.gridPosition.y)
            : cell.gridPosition.y;
    }
    return highestLayer ? *highestLayer + 1 : emptyColumnGridLayer;
}

bool StageActorCreateService::RebuildUGCPlatformNodes(
    YAML::Node& config) const
{
    EnsureSequence(config, "platforms");

    YAML::Node preservedPlatforms(YAML::NodeType::Sequence);
    for (const YAML::Node& platformNode : config["platforms"]) {
        if (!platformNode[UGCGeneratedPlatformKey] ||
            !platformNode[UGCGeneratedPlatformKey].as<bool>(false)) {
            preservedPlatforms.push_back(YAML::Clone(platformNode));
        }
    }
    config["platforms"] = preservedPlatforms;

    using GroupKey = std::tuple<int, int, int, std::string, int, int, int>;
    std::map<GroupKey, std::set<std::pair<int, int>>> groupedCells;
    const YAML::Node cells = config[UGCPlatformCellsKey];
    if (cells && cells.IsSequence()) {
        for (const YAML::Node& cellNode : cells) {
            UGCPlatformCell cell;
            if (!TryReadUGCPlatformCell(cellNode, cell)) {
                continue;
            }

            const int gridSizeMicrounits =
                static_cast<int>(std::round(cell.gridSize * 10000.0f));
            groupedCells[{cell.planetIndex, gridSizeMicrounits,
                          cell.gridPosition.y, cell.behavior,
                          cell.movementDeltaCells.x,
                          cell.movementDeltaCells.y,
                          cell.movementDeltaCells.z}]
                .insert({cell.gridPosition.x, cell.gridPosition.z});
        }
    }

    for (auto& [groupKey, remainingCells] : groupedCells) {
        const auto [planetIndex, gridSizeMicrounits, gridLayer, behavior,
                    movementDeltaX, movementDeltaY, movementDeltaZ] = groupKey;
        const float gridSize =
            static_cast<float>(gridSizeMicrounits) / 10000.0f;

        while (!remainingCells.empty()) {
            const auto [startX, startZ] = *remainingCells.begin();
            int endX = startX;
            while (remainingCells.contains({endX + 1, startZ})) {
                ++endX;
            }

            int endZ = startZ;
            for (;;) {
                const int candidateZ = endZ + 1;
                bool hasCompleteRow = true;
                for (int x = startX; x <= endX; ++x) {
                    if (!remainingCells.contains({x, candidateZ})) {
                        hasCompleteRow = false;
                        break;
                    }
                }
                if (!hasCompleteRow) {
                    break;
                }
                endZ = candidateZ;
            }

            for (int z = startZ; z <= endZ; ++z) {
                for (int x = startX; x <= endX; ++x) {
                    remainingCells.erase({x, z});
                }
            }

            const int widthInCells = endX - startX + 1;
            const int depthInCells = endZ - startZ + 1;
            const glm::vec3 worldPosition(
                (static_cast<float>(startX + endX + 1) * 0.5f) * gridSize,
                static_cast<float>(gridLayer) * gridSize,
                (static_cast<float>(startZ + endZ + 1) * 0.5f) * gridSize);



            // platform.objは各軸で2モデル単位、かつActorのローカルZ軸はUGCグリッドのworld X軸に対応するため幅と奥行きを入れ替える。
            const glm::vec3 scale(
                static_cast<float>(depthInCells) * gridSize * 0.5f,
                0.1f * gridSize,
                static_cast<float>(widthInCells) * gridSize * 0.5f);

            YAML::Node platformNode =
                CreatePlatformNode(planetIndex, "platform.obj", scale);
            StageActorPlacement placement;
            placement.worldPosition = worldPosition;
            placement.surfaceNormal = glm::vec3(0.0f, 1.0f, 0.0f);
            ApplyPlacementToNode(platformNode, planetIndex, &placement);
            platformNode["platformId"] = CreateUniquePlatformId(config);
            platformNode[UGCGeneratedPlatformKey] = true;
            platformNode["ugcGridSize"] = gridSize;
            platformNode["ugcGridLayer"] = gridLayer;
            platformNode["ugcCellMin"][0] = startX;
            platformNode["ugcCellMin"][1] = startZ;
            platformNode["ugcCellMax"][0] = endX;
            platformNode["ugcCellMax"][1] = endZ;
            platformNode["textureTiling"][0] = depthInCells;
            platformNode["textureTiling"][1] = widthInCells;
            platformNode["ugcPlatformBehavior"] = behavior;
            if (behavior == "fading") {
                platformNode["components"]["fadeOnStand"]["fadeOutDuration"] = 1.0f;
                platformNode["components"]["fadeOnStand"]["reappearDelay"] = 2.0f;
            } else if (behavior == "adhesive") {
                platformNode["components"]["adhesion"] =
                    YAML::Node(YAML::NodeType::Map);
            } else if (behavior == "moving") {
                const glm::vec3 startLocal =
                    worldPosition - mContext.game->GetCurrentStage()
                        ->GetPlanets()[planetIndex]->GetPos();
                const glm::vec3 movementDelta(
                    static_cast<float>(movementDeltaX) * gridSize,
                    static_cast<float>(movementDeltaY) * gridSize,
                    static_cast<float>(movementDeltaZ) * gridSize);
                YAML::Node movement = platformNode["components"]["movement"];
                for (int axis = 0; axis < 3; ++axis) {
                    movement["startLocalPos"][axis] = startLocal[axis];
                    movement["endLocalPos"][axis] = startLocal[axis] + movementDelta[axis];
                    movement["moveOffset"][axis] = movementDelta[axis];
                }
                movement["moveDuration"] = 3.0f;
                movement["moveOnPlayer"] = false;
                movement["returnDelay"] = 0.0f;
                movement["endpointWaitSeconds"] = 0.5f;
            }
            config["platforms"].push_back(platformNode);
        }
    }

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
        CreatePlatformNode(currentPlanetNum, modelPath, scale);
    ApplyPlacementToNode(platformNode, currentPlanetNum, placement);
    platformNode["platformId"] = CreateUniquePlatformId(config);

    YAML::Node pressureSwitch =
        platformNode["components"]["pressureSwitch"];
    pressureSwitch["remainsOnAfterPressed"] = false;
    pressureSwitch["inactiveOpacity"] = 0.25f;
    pressureSwitch["targets"] = YAML::Node(YAML::NodeType::Sequence);

    config["platforms"].push_back(platformNode);
    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mContext.game->GetActorLoadSystem()->CreatePlatformFromStageNode(
        platformNode,
        index);
    RefreshPhysicsWorld();
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
        CreateRideMovingPlatformNode(currentPlanetNum, modelPath, scale);
    ApplyPlacementToNode(platformNode, currentPlanetNum, placement);
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

    YAML::Node node = CreatePlatformNode(currentPlanetNum, "platform.obj", scale);
    ApplyPlacementToNode(node, currentPlanetNum, &startPlacement);
    node["platformId"] = CreateUniquePlatformId(config);
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
    mContext.game->GetActorLoadSystem()->CreatePlatformFromStageNode(node, index);
    RefreshPhysicsWorld();
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
    YAML::Node node = CreatePlatformNode(currentPlanetNum, "platform.obj", scale);
    ApplyPlacementToNode(node, currentPlanetNum, placement);
    node["platformId"] = CreateUniquePlatformId(config);
    node["components"]["fadeOnStand"]["fadeOutDuration"] = 1.0f;
    node["components"]["fadeOnStand"]["reappearDelay"] = 2.0f;
    const int index = static_cast<int>(config["platforms"].size());
    config["platforms"].push_back(node);
    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) return false;
    mContext.game->GetActorLoadSystem()->CreatePlatformFromStageNode(node, index);
    RefreshPhysicsWorld();
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
    YAML::Node node = CreatePlatformNode(currentPlanetNum, "platform.obj", scale);
    ApplyPlacementToNode(node, currentPlanetNum, placement);
    node["platformId"] = CreateUniquePlatformId(config);
    node["components"]["adhesion"] = YAML::Node(YAML::NodeType::Map);
    const int index = static_cast<int>(config["platforms"].size());
    config["platforms"].push_back(node);
    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) return false;
    mContext.game->GetActorLoadSystem()->CreatePlatformFromStageNode(node, index);
    RefreshPhysicsWorld();
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
        YAML::Node node = CreatePlatformNode(currentPlanetNum, "platform.obj", glm::vec3(0.75f, 0.2f, 0.75f));
        ApplyPlacementToNode(node, currentPlanetNum, placement);
        node["platformId"] = CreateUniquePlatformId(config);
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
    if (!mContext.game->GetActorLoadSystem()->CreatePlatformFromStageNode(
            firstNode, firstIndex) ||
        !mContext.game->GetActorLoadSystem()->CreatePlatformFromStageNode(
            secondNode, firstIndex + 1)) {
        return false;
    }
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

bool StageActorCreateService::AddEllipsePlanet(const std::string& modelPath)
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
    planetNode["scale"][0] = 4.0f;
    planetNode["scale"][1] = 1.0f;
    planetNode["scale"][2] = 4.0f;
    planetNode["shape"] = "Ellipse";

    config["planets"].push_back(planetNode);
    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mContext.game->GetActorLoadSystem()->CreatePlanetFromStageNode(planetNode);
    RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::AddEnemy(const std::string& type, int currentPlanetNum,
                                       const StageActorPlacement* placement)
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
    ApplyPlacementToNode(enemyNode, currentPlanetNum, placement);

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
    float scale,
    const StageActorPlacement* placement)
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
    ApplyPlacementToNode(npcNode, currentPlanetNum, placement);

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
    const glm::vec3& scale,
    const StageActorPlacement* placement)
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
    ApplyPlacementToNode(triggerNode, currentPlanetNum, placement);
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

bool StageActorCreateService::AddCrystal(const std::string& type, int currentPlanetNum,
                                         const StageActorPlacement* placement)
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
    ApplyPlacementToNode(crystalNode, currentPlanetNum, placement);

    config["crystals"].push_back(crystalNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mContext.game->GetActorLoadSystem()->CreateCrystalFromStageNode(crystalNode, index);
    RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::AddBoatParts(const std::string& type, int currentPlanetNum,
                                           const StageActorPlacement* placement)
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
    ApplyPlacementToNode(partNode, currentPlanetNum, placement);

    config["boatParts"].push_back(partNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mContext.game->GetActorLoadSystem()->CreateBoatPartsFromStageNode(partNode, index);
    RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::AddBoat(int startPlanetNum, int destPlanetNum, int destStage,
                                      const StageActorPlacement* placement)
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
    ApplyPlacementToNode(boatNode, startPlanetNum, placement);

    config["boats"].push_back(boatNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mContext.game->GetActorLoadSystem()->CreateBoatFromStageNode(boatNode, index);
    RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::AddBoatArrivalPoint(
    int currentPlanetNum,
    const std::string& modelPath,
    const glm::vec3& scale,
    const StageActorPlacement* placement)
{
    if (!CanCreateActor() || modelPath.empty() ||
        !IsValidPlanetIndex(currentPlanetNum, "boat arrival point")) {
        return false;
    }

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    EnsureSequence(config, "boatArrivalPoints");

    const int index =
        static_cast<int>(config["boatArrivalPoints"].size());
    YAML::Node arrivalPointNode =
        CreateBoatArrivalPointNode(
            currentPlanetNum,
            modelPath,
            scale);
    ApplyPlacementToNode(
        arrivalPointNode,
        currentPlanetNum,
        placement);
    config["boatArrivalPoints"].push_back(arrivalPointNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mContext.game->GetActorLoadSystem()
        ->CreateBoatArrivalPointFromStageNode(
            arrivalPointNode,
            index);
    RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::AddStar(int currentPlanetNum, const StageActorPlacement* placement)
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
    ApplyPlacementToNode(starNode, currentPlanetNum, placement);
    if (mContext.game->GetIsUGCMode()) {
        starNode["isActive"] = true;
    }

    config["star"].push_back(starNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mContext.game->GetActorLoadSystem()->CreateStarFromStageNode(starNode, index);
    RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::AddJewelItem(
    int currentPlanetNum,
    const std::string& modelPath,
    const std::string& texturePath,
    const glm::vec3& scale,
    const StageActorPlacement* placement)
{
    if (!CanCreateActor() || modelPath.empty() ||
        !IsValidPlanetIndex(currentPlanetNum, "jewel item")) {
        return false;
    }

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    EnsureSequence(config, "jewelItems");
    const int index = static_cast<int>(config["jewelItems"].size());
    YAML::Node jewelItemNode = CreateJewelItemNode(
        currentPlanetNum,
        modelPath,
        texturePath,
        scale);
    ApplyPlacementToNode(jewelItemNode, currentPlanetNum, placement);
    config["jewelItems"].push_back(jewelItemNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mContext.game->GetActorLoadSystem()->CreateJewelItemFromStageNode(
        jewelItemNode,
        index);
    RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::AddHazardActor(
    int currentPlanetNum,
    const std::string& modelPath,
    const std::string& texturePath,
    const glm::vec3& scale,
    float triggerRadius,
    float damage,
    float damageIntervalSeconds,
    const StageActorPlacement* placement)
{
    if (!CanCreateActor() || modelPath.empty() ||
        !IsValidPlanetIndex(currentPlanetNum, "hazard actor")) {
        return false;
    }

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    EnsureSequence(config, "hazardActors");
    const int index =
        static_cast<int>(config["hazardActors"].size());
    YAML::Node hazardActorNode = CreateHazardActorNode(
        currentPlanetNum,
        modelPath,
        texturePath,
        scale,
        triggerRadius,
        damage,
        damageIntervalSeconds);
    ApplyPlacementToNode(
        hazardActorNode,
        currentPlanetNum,
        placement);
    config["hazardActors"].push_back(hazardActorNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mContext.game->GetActorLoadSystem()->
        CreateHazardActorFromStageNode(hazardActorNode, index);
    RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::AddStageObject(
    int currentPlanetNum,
    const std::string& modelPath,
    bool collisionEnabled,
    const StageActorPlacement* placement)
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
    ApplyPlacementToNode(stageObjectNode, currentPlanetNum, placement);
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
    movement["endpointWaitSeconds"] = 0.0f;
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
    node["canAttractNearbyPlayer"] = true;
    node["reactsToOverheadGravityRay"] = false;
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

YAML::Node StageActorCreateService::CreateBoatArrivalPointNode(
    int currentPlanetNum,
    const std::string& modelPath,
    const glm::vec3& scale) const
{
    YAML::Node node;

    node["currentPlanetNum"] = currentPlanetNum;
    node["theta"] = 0.0f;
    node["phi"] = 0.0f;
    node["height"] = 1.0f;
    node["facingYaw"] = 0.0f;
    node["modelPath"] = modelPath;

    node["rotation"][0] = 0.0f;
    node["rotation"][1] = 0.0f;
    node["rotation"][2] = 0.0f;

    node["scale"][0] = std::max(0.01f, scale.x);
    node["scale"][1] = std::max(0.01f, scale.y);
    node["scale"][2] = std::max(0.01f, scale.z);

    return node;
}

YAML::Node StageActorCreateService::CreateStarNode(int currentPlanetNum) const
{
    YAML::Node node;

    node["currentPlanetNum"] = currentPlanetNum;
    node["theta"] = 0.0f;
    node["phi"] = 0.0f;
    node["height"] = 1.0f;
    node["isActive"] = false;

    return node;
}

YAML::Node StageActorCreateService::CreateJewelItemNode(
    int currentPlanetNum,
    const std::string& modelPath,
    const std::string& texturePath,
    const glm::vec3& scale) const
{
    YAML::Node node;
    node["currentPlanetNum"] = currentPlanetNum;
    node["theta"] = 0.0f;
    node["phi"] = 0.0f;
    node["height"] = 0.15f;
    node["facingYaw"] = 0.0f;
    node["modelPath"] = modelPath;
    if (!texturePath.empty()) {
        node["textureOverride"] = texturePath;
    }
    node["scale"][0] = std::max(0.01f, scale.x);
    node["scale"][1] = std::max(0.01f, scale.y);
    node["scale"][2] = std::max(0.01f, scale.z);
    return node;
}

YAML::Node StageActorCreateService::CreateHazardActorNode(
    int currentPlanetNum,
    const std::string& modelPath,
    const std::string& texturePath,
    const glm::vec3& scale,
    float triggerRadius,
    float damage,
    float damageIntervalSeconds) const
{
    YAML::Node node;
    node["currentPlanetNum"] = currentPlanetNum;
    node["theta"] = 0.0f;
    node["phi"] = 0.0f;
    node["height"] = 0.75f;
    node["facingYaw"] = 0.0f;
    node["modelPath"] = modelPath;
    if (!texturePath.empty()) {
        node["textureOverride"] = texturePath;
    }
    node["scale"][0] = std::max(0.01f, scale.x);
    node["scale"][1] = std::max(0.01f, scale.y);
    node["scale"][2] = std::max(0.01f, scale.z);
    node["triggerRadius"] = std::max(0.01f, triggerRadius);
    node["damage"] = std::max(0.0f, damage);
    node["damageIntervalSeconds"] =
        std::max(0.0f, damageIntervalSeconds);
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
