#include "gfx/debug/stage/UGCPlatformDocument.h"

#include "gfx/debug/stage/UGCPlatformGrid.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <tuple>

namespace {

constexpr float GridSizeComparisonTolerance = 0.0001f;

void EnsureSequence(YAML::Node& stageConfig, const char* sequenceName)
{
    if (!stageConfig[sequenceName] ||
        !stageConfig[sequenceName].IsSequence()) {
        stageConfig[sequenceName] = YAML::Node(YAML::NodeType::Sequence);
    }
}

bool MatchesCellLocation(
    const UGCPlatformCell& cell,
    int planetIndex,
    float gridSize,
    const glm::ivec3& gridPosition)
{
    return cell.planetIndex == planetIndex &&
        std::abs(cell.gridSize - gridSize) < GridSizeComparisonTolerance &&
        cell.gridPosition == gridPosition;
}

bool RemoveCellAtIndex(YAML::Node& stageConfig, std::size_t removedCellIndex)
{
    const YAML::Node cells =
        stageConfig[UGCPlatformDocument::PlatformCellsKey];
    if (!cells || !cells.IsSequence() || removedCellIndex >= cells.size()) {
        return false;
    }

    YAML::Node remainingCells(YAML::NodeType::Sequence);
    for (std::size_t cellIndex = 0; cellIndex < cells.size(); ++cellIndex) {
        if (cellIndex != removedCellIndex) {
            remainingCells.push_back(YAML::Clone(cells[cellIndex]));
        }
    }
    stageConfig[UGCPlatformDocument::PlatformCellsKey] = remainingCells;
    return true;
}

}

bool UGCPlatformDocument::TryReadCell(
    const YAML::Node& cellNode,
    UGCPlatformCell& outCell)
{
    if (!cellNode || !cellNode.IsMap() ||
        !cellNode["planetIndex"] ||
        !cellNode["gridSize"] ||
        !cellNode["gridPosition"] ||
        !cellNode["gridPosition"].IsSequence() ||
        cellNode["gridPosition"].size() < 3) {
        return false;
    }

    try {
        outCell.planetIndex = cellNode["planetIndex"].as<int>();
        outCell.gridSize = UGCPlatformGrid::SanitizeGridSize(
            cellNode["gridSize"].as<float>());
        outCell.gridPosition = glm::ivec3(
            cellNode["gridPosition"][0].as<int>(),
            cellNode["gridPosition"][1].as<int>(),
            cellNode["gridPosition"][2].as<int>());
        if (cellNode["behavior"] && cellNode["behavior"].IsScalar()) {
            outCell.behavior = cellNode["behavior"].as<std::string>();
        }
        if (cellNode["movementDeltaCells"] &&
            cellNode["movementDeltaCells"].IsSequence() &&
            cellNode["movementDeltaCells"].size() >= 3) {
            outCell.movementDeltaCells = glm::ivec3(
                cellNode["movementDeltaCells"][0].as<int>(),
                cellNode["movementDeltaCells"][1].as<int>(),
                cellNode["movementDeltaCells"][2].as<int>());
        }
        return true;
    } catch (const YAML::Exception&) {
        return false;
    }
}

YAML::Node UGCPlatformDocument::CreateCellNode(
    const UGCPlatformCell& cell)
{
    YAML::Node cellNode;
    cellNode["planetIndex"] = cell.planetIndex;
    cellNode["gridSize"] = cell.gridSize;
    cellNode["gridPosition"][0] = cell.gridPosition.x;
    cellNode["gridPosition"][1] = cell.gridPosition.y;
    cellNode["gridPosition"][2] = cell.gridPosition.z;
    cellNode["behavior"] = cell.behavior;
    if (cell.behavior == "moving") {
        cellNode["movementDeltaCells"][0] = cell.movementDeltaCells.x;
        cellNode["movementDeltaCells"][1] = cell.movementDeltaCells.y;
        cellNode["movementDeltaCells"][2] = cell.movementDeltaCells.z;
    }
    return cellNode;
}

int UGCPlatformDocument::AddFootprint(
    YAML::Node& stageConfig,
    const UGCPlatformCell& anchorCell,
    int footprintSideLength)
{
    EnsureSequence(stageConfig, PlatformCellsKey);

    int addedCellCount = 0;
    for (const glm::ivec3& gridPosition :
         UGCPlatformGrid::CalculateFootprintCells(
             anchorCell.gridPosition, footprintSideLength)) {
        bool alreadyExists = false;
        for (const YAML::Node& existingCellNode :
             stageConfig[PlatformCellsKey]) {
            UGCPlatformCell existingCell;
            if (TryReadCell(existingCellNode, existingCell) &&
                MatchesCellLocation(
                    existingCell,
                    anchorCell.planetIndex,
                    anchorCell.gridSize,
                    gridPosition)) {
                alreadyExists = true;
                break;
            }
        }
        if (alreadyExists) {
            continue;
        }

        UGCPlatformCell footprintCell = anchorCell;
        footprintCell.gridPosition = gridPosition;
        stageConfig[PlatformCellsKey].push_back(
            CreateCellNode(footprintCell));
        ++addedCellCount;
    }
    return addedCellCount;
}

bool UGCPlatformDocument::RemoveCellAtGridPosition(
    YAML::Node& stageConfig,
    int planetIndex,
    float gridSize,
    const glm::ivec3& gridPosition)
{
    const YAML::Node cells = stageConfig[PlatformCellsKey];
    if (!cells || !cells.IsSequence()) {
        return false;
    }

    for (std::size_t cellIndex = 0; cellIndex < cells.size(); ++cellIndex) {
        UGCPlatformCell cell;
        if (TryReadCell(cells[cellIndex], cell) &&
            MatchesCellLocation(cell, planetIndex, gridSize, gridPosition)) {
            return RemoveCellAtIndex(stageConfig, cellIndex);
        }
    }
    return false;
}

bool UGCPlatformDocument::RemoveClosestCell(
    YAML::Node& stageConfig,
    int planetIndex,
    float gridSize,
    int gridLayer,
    const glm::vec3& hitPosition)
{
    const YAML::Node cells = stageConfig[PlatformCellsKey];
    if (!cells || !cells.IsSequence()) {
        return false;
    }

    std::optional<std::size_t> closestCellIndex;
    float closestDistanceSquared = std::numeric_limits<float>::max();
    for (std::size_t cellIndex = 0; cellIndex < cells.size(); ++cellIndex) {
        UGCPlatformCell cell;
        if (!TryReadCell(cells[cellIndex], cell) ||
            cell.planetIndex != planetIndex ||
            std::abs(cell.gridSize - gridSize) >= GridSizeComparisonTolerance ||
            cell.gridPosition.y != gridLayer) {
            continue;
        }

        const glm::vec3 difference =
            UGCPlatformGrid::CalculateCellWorldPosition(
                cell.gridPosition, cell.gridSize) - hitPosition;
        const float horizontalDistanceSquared =
            difference.x * difference.x + difference.z * difference.z;
        if (horizontalDistanceSquared < closestDistanceSquared) {
            closestDistanceSquared = horizontalDistanceSquared;
            closestCellIndex = cellIndex;
        }
    }
    return closestCellIndex &&
        RemoveCellAtIndex(stageConfig, *closestCellIndex);
}

bool UGCPlatformDocument::RemoveMovingDestinationCellAtGridPosition(
    YAML::Node& stageConfig,
    const UGCGeneratedPlatformRegion& sourceRegion,
    const glm::ivec3& destinationGridPosition)
{
    const YAML::Node cells = stageConfig[PlatformCellsKey];
    if (!cells || !cells.IsSequence()) {
        return false;
    }

    for (std::size_t cellIndex = 0; cellIndex < cells.size(); ++cellIndex) {
        UGCPlatformCell cell;
        if (!TryReadCell(cells[cellIndex], cell)) {
            continue;
        }

        const bool belongsToSourceRegion =
            cell.behavior == "moving" &&
            cell.planetIndex == sourceRegion.planetIndex &&
            std::abs(cell.gridSize - sourceRegion.gridSize) <
                GridSizeComparisonTolerance &&
            cell.gridPosition.y == sourceRegion.gridLayer &&
            cell.gridPosition.x >= sourceRegion.minimumX &&
            cell.gridPosition.x <= sourceRegion.maximumX &&
            cell.gridPosition.z >= sourceRegion.minimumZ &&
            cell.gridPosition.z <= sourceRegion.maximumZ;
        if (!belongsToSourceRegion) {
            continue;
        }

        const glm::ivec3 cellDestination =
            cell.gridPosition + cell.movementDeltaCells;
        if (cellDestination == destinationGridPosition) {
            return RemoveCellAtIndex(stageConfig, cellIndex);
        }
    }
    return false;
}

bool UGCPlatformDocument::ResolveLayerAtGridPosition(
    const YAML::Node& stageConfig,
    int planetIndex,
    float gridSize,
    const glm::ivec3& gridPosition,
    int preferredGridLayer,
    int& outGridLayer)
{
    std::set<int> matchingLayers;
    const YAML::Node cells = stageConfig[PlatformCellsKey];
    if (cells && cells.IsSequence()) {
        for (const YAML::Node& cellNode : cells) {
            UGCPlatformCell cell;
            if (!TryReadCell(cellNode, cell) ||
                cell.planetIndex != planetIndex ||
                std::abs(cell.gridSize - gridSize) >= GridSizeComparisonTolerance ||
                cell.gridPosition.x != gridPosition.x ||
                cell.gridPosition.z != gridPosition.z) {
                continue;
            }
            matchingLayers.insert(cell.gridPosition.y);
        }
    }

    if (matchingLayers.contains(preferredGridLayer)) {
        outGridLayer = preferredGridLayer;
        return true;
    }
    if (!matchingLayers.empty()) {
        outGridLayer = *matchingLayers.rbegin();
        return true;
    }
    return false;
}

int UGCPlatformDocument::ResolvePlacementLayerAtGridPosition(
    const YAML::Node& stageConfig,
    int planetIndex,
    float gridSize,
    const glm::ivec3& gridPosition,
    int emptyColumnGridLayer)
{
    std::optional<int> highestLayer;
    const YAML::Node cells = stageConfig[PlatformCellsKey];
    if (cells && cells.IsSequence()) {
        for (const YAML::Node& cellNode : cells) {
            UGCPlatformCell cell;
            if (!TryReadCell(cellNode, cell) ||
                cell.planetIndex != planetIndex ||
                std::abs(cell.gridSize - gridSize) >= GridSizeComparisonTolerance ||
                cell.gridPosition.x != gridPosition.x ||
                cell.gridPosition.z != gridPosition.z) {
                continue;
            }
            highestLayer = highestLayer
                ? std::max(*highestLayer, cell.gridPosition.y)
                : cell.gridPosition.y;
        }
    }
    return highestLayer ? *highestLayer + 1 : emptyColumnGridLayer;
}

int UGCPlatformDocument::TranslateCells(
    YAML::Node& stageConfig,
    const std::vector<UGCPlatformCellTranslationRegion>& regions)
{
    YAML::Node cells = stageConfig[PlatformCellsKey];
    if (!cells || !cells.IsSequence()) {
        return 0;
    }

    int translatedCellCount = 0;
    for (YAML::Node cellNode : cells) {
        UGCPlatformCell cell;
        if (!TryReadCell(cellNode, cell)) {
            continue;
        }

        for (const UGCPlatformCellTranslationRegion& region : regions) {
            const bool belongsToRegion =
                cell.planetIndex == region.planetIndex &&
                std::abs(cell.gridSize - region.gridSize) <
                    GridSizeComparisonTolerance &&
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
            ++translatedCellCount;
            break;
        }
    }
    return translatedCellCount;
}

bool UGCPlatformDocument::TranslateGeneratedPlatformRegionMetadata(
    YAML::Node& platformNode,
    const glm::ivec3& gridDelta)
{
    const YAML::Node cellMin = platformNode["ugcCellMin"];
    const YAML::Node cellMax = platformNode["ugcCellMax"];
    if (!platformNode[GeneratedPlatformKey] ||
        !platformNode[GeneratedPlatformKey].as<bool>(false) ||
        !cellMin || !cellMin.IsSequence() || cellMin.size() < 2 ||
        !cellMax || !cellMax.IsSequence() || cellMax.size() < 2) {
        return false;
    }

    platformNode["ugcGridLayer"] =
        platformNode["ugcGridLayer"].as<int>(0) + gridDelta.y;
    platformNode["ugcCellMin"][0] = cellMin[0].as<int>() + gridDelta.x;
    platformNode["ugcCellMin"][1] = cellMin[1].as<int>() + gridDelta.z;
    platformNode["ugcCellMax"][0] = cellMax[0].as<int>() + gridDelta.x;
    platformNode["ugcCellMax"][1] = cellMax[1].as<int>() + gridDelta.z;
    return true;
}

void UGCPlatformDocument::RemoveGeneratedPlatforms(YAML::Node& stageConfig)
{
    EnsureSequence(stageConfig, "platforms");

    YAML::Node preservedPlatforms(YAML::NodeType::Sequence);
    for (const YAML::Node& platformNode : stageConfig["platforms"]) {
        if (!platformNode[GeneratedPlatformKey] ||
            !platformNode[GeneratedPlatformKey].as<bool>(false)) {
            preservedPlatforms.push_back(YAML::Clone(platformNode));
        }
    }
    stageConfig["platforms"] = preservedPlatforms;
}

std::vector<UGCGeneratedPlatformRegion>
UGCPlatformDocument::CalculateGeneratedPlatformRegions(
    const YAML::Node& stageConfig)
{
    using GroupKey = std::tuple<int, int, int, std::string, int, int, int>;
    std::map<GroupKey, std::set<std::pair<int, int>>> groupedCells;

    const YAML::Node cells = stageConfig[PlatformCellsKey];
    if (cells && cells.IsSequence()) {
        for (const YAML::Node& cellNode : cells) {
            UGCPlatformCell cell;
            if (!TryReadCell(cellNode, cell)) {
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

    std::vector<UGCGeneratedPlatformRegion> regions;
    for (auto& [groupKey, remainingCells] : groupedCells) {
        const auto [planetIndex, gridSizeMicrounits, gridLayer, behavior,
                    movementDeltaX, movementDeltaY, movementDeltaZ] = groupKey;

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

            UGCGeneratedPlatformRegion region;
            region.planetIndex = planetIndex;
            region.gridSize =
                static_cast<float>(gridSizeMicrounits) / 10000.0f;
            region.gridLayer = gridLayer;
            region.minimumX = startX;
            region.minimumZ = startZ;
            region.maximumX = endX;
            region.maximumZ = endZ;
            region.behavior = behavior;
            region.movementDeltaCells = glm::ivec3(
                movementDeltaX, movementDeltaY, movementDeltaZ);
            regions.emplace_back(std::move(region));
        }
    }
    return regions;
}

YAML::Node UGCPlatformDocument::FindMatchingGeneratedPlatformNode(
    const YAML::Node& stageConfig,
    const UGCGeneratedPlatformRegion& region)
{
    const YAML::Node platforms = stageConfig["platforms"];
    if (!platforms || !platforms.IsSequence()) {
        return YAML::Node(YAML::NodeType::Undefined);
    }

    for (const YAML::Node& platformNode : platforms) {
        const YAML::Node cellMin = platformNode["ugcCellMin"];
        const YAML::Node cellMax = platformNode["ugcCellMax"];
        if (!platformNode[GeneratedPlatformKey] ||
            !platformNode[GeneratedPlatformKey].as<bool>(false) ||
            !cellMin || !cellMin.IsSequence() || cellMin.size() < 2 ||
            !cellMax || !cellMax.IsSequence() || cellMax.size() < 2) {
            continue;
        }

        const bool matchesRegion =
            platformNode["currentPlanetNum"].as<int>(-1) ==
                region.planetIndex &&
            std::abs(
                platformNode["ugcGridSize"].as<float>(1.0f) -
                region.gridSize) < GridSizeComparisonTolerance &&
            platformNode["ugcGridLayer"].as<int>(0) == region.gridLayer &&
            cellMin[0].as<int>() == region.minimumX &&
            cellMin[1].as<int>() == region.minimumZ &&
            cellMax[0].as<int>() == region.maximumX &&
            cellMax[1].as<int>() == region.maximumZ &&
            platformNode["ugcPlatformBehavior"].as<std::string>("normal") ==
                region.behavior;
        if (matchesRegion) {
            return platformNode;
        }
    }

    return YAML::Node(YAML::NodeType::Undefined);
}
