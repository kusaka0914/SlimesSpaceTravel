#include "gfx/debug/stage/UGCPlatformCellService.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Planet.h"
#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/stage/StagePlatformIdentifiers.h"
#include "gfx/debug/stage/StageYamlRepository.h"
#include "gfx/debug/stage/UGCPlatformDocument.h"
#include "gfx/debug/stage/UGCPlatformGrid.h"
#include "system/ActorLoadSystem.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <set>

namespace {

constexpr const char* PlatformCellsKey =
    UGCPlatformDocument::PlatformCellsKey;
constexpr const char* GeneratedPlatformKey =
    UGCPlatformDocument::GeneratedPlatformKey;
using PlatformCell = UGCPlatformCell;

bool TryReadPlatformCell(
    const YAML::Node& cellNode,
    PlatformCell& outCell)
{
    return UGCPlatformDocument::TryReadCell(cellNode, outCell);
}

}

UGCPlatformCellService::UGCPlatformCellService(DebugEditorContext& context)
    : mContext(context),
      mNodeFactory(context)
{
}

bool UGCPlatformCellService::CanEditCells() const
{
    return mContext.game &&
        mContext.game->GetCurrentStage() &&
        mContext.game->GetActorLoadSystem();
}

bool UGCPlatformCellService::IsValidPlanetIndex(int planetIndex) const
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return false;
    }

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();
    if (planetIndex < 0 || planetIndex >= static_cast<int>(planets.size())) {
        std::cerr << "Invalid UGC platform cell planet index: "
                  << planetIndex << std::endl;
        return false;
    }
    return true;
}

bool UGCPlatformCellService::AddCell(
    int planetIndex,
    const StageActorPlacement& placement,
    float gridSize,
    int footprintSideLength,
    const std::string& behavior,
    const glm::ivec3& movementDeltaCells)
{
    if (!CanEditCells() || !IsValidPlanetIndex(planetIndex)) {
        return false;
    }

    const float safeGridSize = UGCPlatformGrid::SanitizeGridSize(gridSize);
    PlatformCell newCell;
    newCell.planetIndex = planetIndex;
    newCell.gridSize = safeGridSize;
    newCell.gridPosition = UGCPlatformGrid::CalculateGridPosition(
        placement.worldPosition, safeGridSize);
    newCell.behavior = behavior;
    newCell.movementDeltaCells = movementDeltaCells;

    YAML::Node stageConfig;
    if (!StageYamlRepository::LoadCurrentStage(mContext, stageConfig)) {
        return false;
    }

    UGCPlatformDocument::AddFootprint(
        stageConfig, newCell, footprintSideLength);

    if (!RebuildGeneratedPlatforms(stageConfig) ||
        !StageYamlRepository::SaveCurrentStage(mContext, stageConfig)) {
        return false;
    }
    mContext.game->ReloadCurrentStage(StagePhysicsReloadMode::SkipRebuild);
    return true;
}

bool UGCPlatformCellService::RefreshGeneratedPlatforms()
{
    if (!CanEditCells()) {
        return false;
    }

    YAML::Node stageConfig;
    if (!StageYamlRepository::LoadCurrentStage(mContext, stageConfig) ||
        !RebuildGeneratedPlatforms(stageConfig) ||
        !StageYamlRepository::SaveCurrentStage(mContext, stageConfig)) {
        return false;
    }
    mContext.game->ReloadCurrentStage(StagePhysicsReloadMode::SkipRebuild);
    return true;
}

bool UGCPlatformCellService::TranslateCells(
    const StageActorRef& generatedPlatformRef,
    const glm::vec3& worldDelta)
{
    return TranslateCells(
        std::vector<StageActorRef>{generatedPlatformRef},
        worldDelta);
}

bool UGCPlatformCellService::TranslateCells(
    const std::vector<StageActorRef>& generatedPlatformRefs,
    const glm::vec3& worldDelta)
{
    if (!CanEditCells() || generatedPlatformRefs.empty()) {
        return false;
    }

    YAML::Node stageConfig;
    if (!StageYamlRepository::LoadCurrentStage(mContext, stageConfig)) {
        return false;
    }
    const YAML::Node platforms = stageConfig["platforms"];
    if (!platforms || !platforms.IsSequence()) {
        return false;
    }

    std::vector<UGCPlatformCellTranslationRegion> selectedRegions;
    selectedRegions.reserve(generatedPlatformRefs.size());
    for (const StageActorRef& generatedPlatformRef : generatedPlatformRefs) {
        if (generatedPlatformRef.sequenceName != "platforms" ||
            generatedPlatformRef.yamlIndex < 0 ||
            generatedPlatformRef.yamlIndex >= static_cast<int>(platforms.size())) {
            continue;
        }

        const YAML::Node platformNode =
            platforms[generatedPlatformRef.yamlIndex];
        const YAML::Node cellMin = platformNode["ugcCellMin"];
        const YAML::Node cellMax = platformNode["ugcCellMax"];
        if (!platformNode[GeneratedPlatformKey] ||
            !platformNode[GeneratedPlatformKey].as<bool>(false) ||
            !cellMin || !cellMin.IsSequence() || cellMin.size() < 2 ||
            !cellMax || !cellMax.IsSequence() || cellMax.size() < 2) {
            continue;
        }

        UGCPlatformCellTranslationRegion region;
        region.planetIndex = platformNode["currentPlanetNum"].as<int>(-1);
        region.gridSize = platformNode["ugcGridSize"].as<float>(1.0f);
        region.gridLayer = platformNode["ugcGridLayer"].as<int>(0);
        region.minimumX = cellMin[0].as<int>();
        region.minimumZ = cellMin[1].as<int>();
        region.maximumX = cellMax[0].as<int>();
        region.maximumZ = cellMax[1].as<int>();
        region.gridDelta = UGCPlatformGrid::CalculateGridDelta(
            worldDelta, region.gridSize);
        selectedRegions.emplace_back(region);
    }

    if (selectedRegions.empty()) {
        return false;
    }

    if (UGCPlatformDocument::TranslateCells(
            stageConfig, selectedRegions) == 0) {
        return false;
    }

    if (!RebuildGeneratedPlatforms(stageConfig) ||
        !StageYamlRepository::SaveCurrentStage(mContext, stageConfig)) {
        return false;
    }
    mContext.game->ReloadCurrentStage(StagePhysicsReloadMode::SkipRebuild);
    return true;
}

bool UGCPlatformCellService::RemoveCell(
    const StageActorRef& generatedPlatformRef,
    const glm::vec3& hitPosition)
{
    if (!CanEditCells() ||
        generatedPlatformRef.sequenceName != "platforms" ||
        generatedPlatformRef.yamlIndex < 0) {
        return false;
    }

    YAML::Node stageConfig;
    if (!StageYamlRepository::LoadCurrentStage(mContext, stageConfig)) {
        return false;
    }
    const YAML::Node platforms = stageConfig["platforms"];
    if (!platforms || !platforms.IsSequence() ||
        generatedPlatformRef.yamlIndex >= static_cast<int>(platforms.size())) {
        return false;
    }

    const YAML::Node generatedPlatform =
        platforms[generatedPlatformRef.yamlIndex];
    if (!generatedPlatform[GeneratedPlatformKey] ||
        !generatedPlatform[GeneratedPlatformKey].as<bool>(false)) {
        return false;
    }

    const int planetIndex =
        generatedPlatform["currentPlanetNum"].as<int>(-1);
    const float gridSize = generatedPlatform["ugcGridSize"].as<float>(1.0f);
    const int gridLayer = generatedPlatform["ugcGridLayer"].as<int>(0);
    if (!UGCPlatformDocument::RemoveClosestCell(
            stageConfig,
            planetIndex,
            gridSize,
            gridLayer,
            hitPosition)) {
        return false;
    }

    if (!RebuildGeneratedPlatforms(stageConfig) ||
        !StageYamlRepository::SaveCurrentStage(mContext, stageConfig)) {
        return false;
    }
    mContext.game->ReloadCurrentStage(StagePhysicsReloadMode::SkipRebuild);
    return true;
}

bool UGCPlatformCellService::RemoveCellAtGridPosition(
    int planetIndex,
    const glm::vec3& worldPosition,
    float gridSize,
    int gridLayer)
{
    if (!CanEditCells()) {
        return false;
    }

    const float safeGridSize = UGCPlatformGrid::SanitizeGridSize(gridSize);
    glm::ivec3 targetGridPosition =
        UGCPlatformGrid::CalculateGridPosition(
            worldPosition, safeGridSize);
    targetGridPosition.y = gridLayer;

    YAML::Node stageConfig;
    if (!StageYamlRepository::LoadCurrentStage(mContext, stageConfig)) {
        return false;
    }
    if (!UGCPlatformDocument::RemoveCellAtGridPosition(
            stageConfig,
            planetIndex,
            safeGridSize,
            targetGridPosition)) {
        return false;
    }

    if (!RebuildGeneratedPlatforms(stageConfig) ||
        !StageYamlRepository::SaveCurrentStage(mContext, stageConfig)) {
        return false;
    }
    mContext.game->ReloadCurrentStage(StagePhysicsReloadMode::SkipRebuild);
    return true;
}

bool UGCPlatformCellService::ResolveLayerAtGridPosition(
    int planetIndex,
    const glm::vec3& worldPosition,
    float gridSize,
    int preferredGridLayer,
    int& outGridLayer) const
{
    const float safeGridSize = UGCPlatformGrid::SanitizeGridSize(gridSize);
    const glm::ivec3 targetGridPosition =
        UGCPlatformGrid::CalculateGridPosition(
            worldPosition, safeGridSize);

    YAML::Node stageConfig;
    if (!StageYamlRepository::LoadCurrentStage(mContext, stageConfig)) {
        return false;
    }
    return UGCPlatformDocument::ResolveLayerAtGridPosition(
        stageConfig,
        planetIndex,
        safeGridSize,
        targetGridPosition,
        preferredGridLayer,
        outGridLayer);
}

int UGCPlatformCellService::ResolvePlacementLayerAtGridPosition(
    int planetIndex,
    const glm::vec3& worldPosition,
    float gridSize,
    int emptyColumnGridLayer) const
{
    const float safeGridSize = UGCPlatformGrid::SanitizeGridSize(gridSize);
    const glm::ivec3 targetGridPosition =
        UGCPlatformGrid::CalculateGridPosition(
            worldPosition, safeGridSize);

    YAML::Node stageConfig;
    if (!StageYamlRepository::LoadCurrentStage(mContext, stageConfig)) {
        return emptyColumnGridLayer;
    }
    return UGCPlatformDocument::ResolvePlacementLayerAtGridPosition(
        stageConfig,
        planetIndex,
        safeGridSize,
        targetGridPosition,
        emptyColumnGridLayer);
}

void UGCPlatformCellService::ApplyPlacement(
    YAML::Node& actorNode,
    int planetIndex,
    const StageActorPlacement& placement) const
{
    if (!IsValidPlanetIndex(planetIndex)) {
        return;
    }

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();
    const Planet* planet = planets[planetIndex];
    if (!planet) {
        return;
    }

    const glm::vec3 localPosition = placement.worldPosition - planet->GetPos();
    actorNode["pos"][0] = localPosition.x;
    actorNode["pos"][1] = localPosition.y;
    actorNode["pos"][2] = localPosition.z;

    const float localDistance = glm::length(localPosition);
    if (localDistance > 1e-6f) {
        const glm::vec3 radialDirection = localPosition / localDistance;
        actorNode["theta"] =
            std::atan2(radialDirection.z, radialDirection.x);
        actorNode["phi"] =
            std::asin(std::clamp(radialDirection.y, -1.0f, 1.0f));
        actorNode["height"] = localDistance - planet->GetRadius();
    }

    glm::vec3 surfaceNormal = placement.surfaceNormal;
    if (glm::length(surfaceNormal) < 1e-6f) {
        surfaceNormal = localDistance > 1e-6f
            ? localPosition / localDistance
            : glm::vec3(0.0f, 1.0f, 0.0f);
    }
    surfaceNormal = glm::normalize(surfaceNormal);
    actorNode["upVec"][0] = surfaceNormal.x;
    actorNode["upVec"][1] = surfaceNormal.y;
    actorNode["upVec"][2] = surfaceNormal.z;
}

bool UGCPlatformCellService::RebuildGeneratedPlatforms(
    YAML::Node& stageConfig) const
{
    UGCPlatformDocument::RemoveGeneratedPlatforms(stageConfig);
    const std::vector<UGCGeneratedPlatformRegion> generatedRegions =
        UGCPlatformDocument::CalculateGeneratedPlatformRegions(stageConfig);

    for (const UGCGeneratedPlatformRegion& region : generatedRegions) {
        const int widthInCells = region.maximumX - region.minimumX + 1;
        const int depthInCells = region.maximumZ - region.minimumZ + 1;
        const glm::vec3 worldPosition(
            (static_cast<float>(region.minimumX + region.maximumX + 1) * 0.5f) *
                region.gridSize,
            static_cast<float>(region.gridLayer) * region.gridSize,
            (static_cast<float>(region.minimumZ + region.maximumZ + 1) * 0.5f) *
                region.gridSize);

        // platform.objのローカルZ軸はUGCグリッドのworld X軸に対応する。
        const glm::vec3 scale(
            static_cast<float>(depthInCells) * region.gridSize * 0.5f,
            0.1f * region.gridSize,
            static_cast<float>(widthInCells) * region.gridSize * 0.5f);

        YAML::Node platformNode = mNodeFactory.CreatePlatform(
            region.planetIndex, "platform.obj", scale);
        StageActorPlacement placement;
        placement.worldPosition = worldPosition;
        placement.surfaceNormal = glm::vec3(0.0f, 1.0f, 0.0f);
        ApplyPlacement(platformNode, region.planetIndex, placement);
        platformNode["platformId"] =
            StagePlatformIdentifiers::CreateUniqueId(stageConfig);
        platformNode[GeneratedPlatformKey] = true;
        platformNode["ugcGridSize"] = region.gridSize;
        platformNode["ugcGridLayer"] = region.gridLayer;
        platformNode["ugcCellMin"][0] = region.minimumX;
        platformNode["ugcCellMin"][1] = region.minimumZ;
        platformNode["ugcCellMax"][0] = region.maximumX;
        platformNode["ugcCellMax"][1] = region.maximumZ;
        platformNode["textureTiling"][0] = depthInCells;
        platformNode["textureTiling"][1] = widthInCells;
        platformNode["ugcPlatformBehavior"] = region.behavior;
        if (region.behavior == "fading") {
                platformNode["components"]["fadeOnStand"]["fadeOutDuration"] = 1.0f;
                platformNode["components"]["fadeOnStand"]["reappearDelay"] = 2.0f;
        } else if (region.behavior == "adhesive") {
            platformNode["components"]["adhesion"] =
                YAML::Node(YAML::NodeType::Map);
        } else if (region.behavior == "moving") {
            const glm::vec3 startLocal =
                worldPosition - mContext.game->GetCurrentStage()
                    ->GetPlanets()[region.planetIndex]->GetPos();
            const glm::vec3 movementDelta =
                glm::vec3(region.movementDeltaCells) * region.gridSize;
            YAML::Node movement = platformNode["components"]["movement"];
            for (int axis = 0; axis < 3; ++axis) {
                movement["startLocalPos"][axis] = startLocal[axis];
                movement["endLocalPos"][axis] =
                    startLocal[axis] + movementDelta[axis];
                movement["moveOffset"][axis] = movementDelta[axis];
            }
            movement["moveDuration"] = 3.0f;
            movement["moveOnPlayer"] = false;
            movement["returnDelay"] = 0.0f;
            movement["endpointWaitSeconds"] = 0.5f;
        }
        stageConfig["platforms"].push_back(platformNode);
    }
    return true;
}
