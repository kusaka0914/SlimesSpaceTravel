#include "TestSupport.h"

#include "gfx/debug/stage/UGCPlatformDocument.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

UGCPlatformCell CreateCell(
    int x,
    int layer,
    int z,
    const std::string& behavior = "normal")
{
    UGCPlatformCell cell;
    cell.planetIndex = 2;
    cell.gridSize = 2.0f;
    cell.gridPosition = glm::ivec3(x, layer, z);
    cell.behavior = behavior;
    return cell;
}

void AddFootprintCreatesRequestedCellsWithoutDuplicates()
{
    YAML::Node stageConfig;
    const UGCPlatformCell anchorCell = CreateCell(3, 1, 4);

    const int firstAddedCount =
        UGCPlatformDocument::AddFootprint(stageConfig, anchorCell, 2);
    const int secondAddedCount =
        UGCPlatformDocument::AddFootprint(stageConfig, anchorCell, 2);

    ExpectEqual(4, firstAddedCount, "first added cell count");
    ExpectEqual(0, secondAddedCount, "duplicate added cell count");
    ExpectEqual(
        std::size_t{4},
        stageConfig[UGCPlatformDocument::PlatformCellsKey].size(),
        "stored cell count");
}

void MovingCellRoundTripPreservesMovementDelta()
{
    UGCPlatformCell sourceCell = CreateCell(1, 2, 3, "moving");
    sourceCell.movementDeltaCells = glm::ivec3(4, -1, 2);

    UGCPlatformCell loadedCell;
    const bool wasRead = UGCPlatformDocument::TryReadCell(
        UGCPlatformDocument::CreateCellNode(sourceCell), loadedCell);

    ExpectTrue(wasRead, "moving cell was read");
    ExpectTrue(
        loadedCell.movementDeltaCells == sourceCell.movementDeltaCells,
        "movement delta");
}

void MalformedCellIsIgnored()
{
    YAML::Node malformedCell;
    malformedCell["planetIndex"] = 1;

    UGCPlatformCell loadedCell;
    ExpectFalse(
        UGCPlatformDocument::TryReadCell(malformedCell, loadedCell),
        "malformed cell read result");
}

void RemoveCellAtGridPositionRemovesOnlyExactCell()
{
    YAML::Node stageConfig;
    UGCPlatformDocument::AddFootprint(
        stageConfig, CreateCell(2, 0, 2), 2);

    const bool wasRemoved = UGCPlatformDocument::RemoveCellAtGridPosition(
        stageConfig, 2, 2.0f, glm::ivec3(2, 0, 2));

    ExpectTrue(wasRemoved, "remove result");
    ExpectEqual(
        std::size_t{3},
        stageConfig[UGCPlatformDocument::PlatformCellsKey].size(),
        "remaining cell count");
    ExpectFalse(
        UGCPlatformDocument::RemoveCellAtGridPosition(
            stageConfig, 2, 2.0f, glm::ivec3(2, 0, 2)),
        "second remove result");
}

void RemoveClosestCellUsesHorizontalDistanceWithinLayer()
{
    YAML::Node stageConfig;
    UGCPlatformDocument::AddFootprint(
        stageConfig, CreateCell(0, 0, 0), 1);
    UGCPlatformDocument::AddFootprint(
        stageConfig, CreateCell(3, 0, 0), 1);
    UGCPlatformDocument::AddFootprint(
        stageConfig, CreateCell(0, 1, 0), 1);

    const bool wasRemoved = UGCPlatformDocument::RemoveClosestCell(
        stageConfig, 2, 2.0f, 0, glm::vec3(6.8f, 100.0f, 1.0f));

    ExpectTrue(wasRemoved, "remove closest result");
    ExpectFalse(
        UGCPlatformDocument::RemoveCellAtGridPosition(
            stageConfig, 2, 2.0f, glm::ivec3(3, 0, 0)),
        "closest cell already removed");
    ExpectTrue(
        UGCPlatformDocument::RemoveCellAtGridPosition(
            stageConfig, 2, 2.0f, glm::ivec3(0, 1, 0)),
        "other layer remains");
}

void LayerResolutionUsesPreferredLayerWhenColumnHasMultipleCells()
{
    YAML::Node stageConfig;
    UGCPlatformDocument::AddFootprint(
        stageConfig, CreateCell(1, 2, 3), 1);
    UGCPlatformDocument::AddFootprint(
        stageConfig, CreateCell(1, 5, 3), 1);

    int resolvedLayer = -1;
    const bool wasResolved = UGCPlatformDocument::ResolveLayerAtGridPosition(
        stageConfig, 2, 2.0f, glm::ivec3(1, 0, 3), 5, resolvedLayer);

    ExpectTrue(wasResolved, "preferred layer resolve result");
    ExpectEqual(5, resolvedLayer, "preferred layer");
}

void LayerResolutionRejectsAmbiguousColumnWithoutPreferredLayer()
{
    YAML::Node stageConfig;
    UGCPlatformDocument::AddFootprint(
        stageConfig, CreateCell(1, 2, 3), 1);
    UGCPlatformDocument::AddFootprint(
        stageConfig, CreateCell(1, 5, 3), 1);

    int resolvedLayer = -1;
    ExpectFalse(
        UGCPlatformDocument::ResolveLayerAtGridPosition(
            stageConfig, 2, 2.0f, glm::ivec3(1, 0, 3), 9, resolvedLayer),
        "ambiguous layer resolve result");
}

void PlacementLayerUsesLayerAboveHighestCell()
{
    YAML::Node stageConfig;
    UGCPlatformDocument::AddFootprint(
        stageConfig, CreateCell(1, 2, 3), 1);
    UGCPlatformDocument::AddFootprint(
        stageConfig, CreateCell(1, 5, 3), 1);

    ExpectEqual(
        6,
        UGCPlatformDocument::ResolvePlacementLayerAtGridPosition(
            stageConfig, 2, 2.0f, glm::ivec3(1, 0, 3), 4),
        "next placement layer");
    ExpectEqual(
        4,
        UGCPlatformDocument::ResolvePlacementLayerAtGridPosition(
            stageConfig, 2, 2.0f, glm::ivec3(9, 0, 9), 4),
        "empty column placement layer");
}

void GeneratedRegionsMergeCompleteRectangle()
{
    YAML::Node stageConfig;
    UGCPlatformDocument::AddFootprint(
        stageConfig, CreateCell(2, 1, 3), 2);

    const std::vector<UGCGeneratedPlatformRegion> regions =
        UGCPlatformDocument::CalculateGeneratedPlatformRegions(stageConfig);

    ExpectEqual(std::size_t{1}, regions.size(), "region count");
    ExpectEqual(1, regions[0].minimumX, "minimum x");
    ExpectEqual(2, regions[0].maximumX, "maximum x");
    ExpectEqual(2, regions[0].minimumZ, "minimum z");
    ExpectEqual(3, regions[0].maximumZ, "maximum z");
}

void TranslateCellsMovesOnlyCellsInsideSelectedRegion()
{
    YAML::Node stageConfig;
    UGCPlatformDocument::AddFootprint(
        stageConfig, CreateCell(2, 1, 3), 2);
    UGCPlatformDocument::AddFootprint(
        stageConfig, CreateCell(8, 1, 8), 1);

    UGCPlatformCellTranslationRegion region;
    region.planetIndex = 2;
    region.gridSize = 2.0f;
    region.gridLayer = 1;
    region.minimumX = 1;
    region.minimumZ = 2;
    region.maximumX = 2;
    region.maximumZ = 3;
    region.gridDelta = glm::ivec3(3, 2, -1);

    const int translatedCellCount =
        UGCPlatformDocument::TranslateCells(stageConfig, {region});

    ExpectEqual(4, translatedCellCount, "translated cell count");
    ExpectTrue(
        UGCPlatformDocument::RemoveCellAtGridPosition(
            stageConfig, 2, 2.0f, glm::ivec3(5, 3, 2)),
        "translated anchor cell");
    ExpectTrue(
        UGCPlatformDocument::RemoveCellAtGridPosition(
            stageConfig, 2, 2.0f, glm::ivec3(8, 1, 8)),
        "unselected cell remains");
}

void GeneratedRegionsSeparateBehaviorsAndMovementDeltas()
{
    YAML::Node stageConfig;
    UGCPlatformCell movingRight = CreateCell(0, 0, 0, "moving");
    movingRight.movementDeltaCells = glm::ivec3(1, 0, 0);
    UGCPlatformCell movingUp = CreateCell(1, 0, 0, "moving");
    movingUp.movementDeltaCells = glm::ivec3(0, 1, 0);
    UGCPlatformDocument::AddFootprint(stageConfig, movingRight, 1);
    UGCPlatformDocument::AddFootprint(stageConfig, movingUp, 1);
    UGCPlatformDocument::AddFootprint(
        stageConfig, CreateCell(2, 0, 0, "fading"), 1);

    const std::vector<UGCGeneratedPlatformRegion> regions =
        UGCPlatformDocument::CalculateGeneratedPlatformRegions(stageConfig);

    ExpectEqual(std::size_t{3}, regions.size(), "separate region count");
}

void GeneratedPlatformRemovalPreservesAuthoredPlatforms()
{
    YAML::Node stageConfig;
    YAML::Node authoredPlatform;
    authoredPlatform["name"] = "authored";
    YAML::Node generatedPlatform;
    generatedPlatform[UGCPlatformDocument::GeneratedPlatformKey] = true;
    stageConfig["platforms"].push_back(authoredPlatform);
    stageConfig["platforms"].push_back(generatedPlatform);

    UGCPlatformDocument::RemoveGeneratedPlatforms(stageConfig);

    ExpectEqual(std::size_t{1}, stageConfig["platforms"].size(), "platform count");
    ExpectEqual(
        std::string("authored"),
        stageConfig["platforms"][0]["name"].as<std::string>(),
        "preserved platform name");
}

}

void RegisterUGCPlatformDocumentTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.emplace_back("UGCPlatformDocument.AddFootprintCreatesRequestedCellsWithoutDuplicates", AddFootprintCreatesRequestedCellsWithoutDuplicates);
    tests.emplace_back("UGCPlatformDocument.MovingCellRoundTripPreservesMovementDelta", MovingCellRoundTripPreservesMovementDelta);
    tests.emplace_back("UGCPlatformDocument.MalformedCellIsIgnored", MalformedCellIsIgnored);
    tests.emplace_back("UGCPlatformDocument.RemoveCellAtGridPositionRemovesOnlyExactCell", RemoveCellAtGridPositionRemovesOnlyExactCell);
    tests.emplace_back("UGCPlatformDocument.RemoveClosestCellUsesHorizontalDistanceWithinLayer", RemoveClosestCellUsesHorizontalDistanceWithinLayer);
    tests.emplace_back("UGCPlatformDocument.LayerResolutionUsesPreferredLayerWhenColumnHasMultipleCells", LayerResolutionUsesPreferredLayerWhenColumnHasMultipleCells);
    tests.emplace_back("UGCPlatformDocument.LayerResolutionRejectsAmbiguousColumnWithoutPreferredLayer", LayerResolutionRejectsAmbiguousColumnWithoutPreferredLayer);
    tests.emplace_back("UGCPlatformDocument.PlacementLayerUsesLayerAboveHighestCell", PlacementLayerUsesLayerAboveHighestCell);
    tests.emplace_back("UGCPlatformDocument.GeneratedRegionsMergeCompleteRectangle", GeneratedRegionsMergeCompleteRectangle);
    tests.emplace_back("UGCPlatformDocument.TranslateCellsMovesOnlyCellsInsideSelectedRegion", TranslateCellsMovesOnlyCellsInsideSelectedRegion);
    tests.emplace_back("UGCPlatformDocument.GeneratedRegionsSeparateBehaviorsAndMovementDeltas", GeneratedRegionsSeparateBehaviorsAndMovementDeltas);
    tests.emplace_back("UGCPlatformDocument.GeneratedPlatformRemovalPreservesAuthoredPlatforms", GeneratedPlatformRemovalPreservesAuthoredPlatforms);
}
