#include "TestSupport.h"

#include "gfx/debug/stage/UGCPlatformGrid.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

void InvalidGridSizeUsesMinimumPositiveSize()
{
    ExpectNear(
        0.01f,
        UGCPlatformGrid::SanitizeGridSize(0.0f),
        0.0001f,
        "zero grid size");
    ExpectNear(
        0.01f,
        UGCPlatformGrid::SanitizeGridSize(-3.0f),
        0.0001f,
        "negative grid size");
}

void WorldPositionConvertsToGridUsingFloorForHorizontalAxes()
{
    const glm::ivec3 gridPosition =
        UGCPlatformGrid::CalculateGridPosition(
            glm::vec3(2.9f, 3.1f, 4.0f), 2.0f);

    ExpectEqual(1, gridPosition.x, "grid x");
    ExpectEqual(2, gridPosition.y, "grid layer");
    ExpectEqual(2, gridPosition.z, "grid z");
}

void NegativeWorldPositionUsesLowerGridCell()
{
    const glm::ivec3 gridPosition =
        UGCPlatformGrid::CalculateGridPosition(
            glm::vec3(-0.1f, -0.6f, -2.1f), 1.0f);

    ExpectEqual(-1, gridPosition.x, "negative grid x");
    ExpectEqual(-1, gridPosition.y, "negative grid layer");
    ExpectEqual(-3, gridPosition.z, "negative grid z");
}

void GridPositionConvertsToCellCenter()
{
    const glm::vec3 worldPosition =
        UGCPlatformGrid::CalculateCellWorldPosition(
            glm::ivec3(2, 3, -1), 2.0f);

    ExpectNear(5.0f, worldPosition.x, 0.0001f, "world x");
    ExpectNear(6.0f, worldPosition.y, 0.0001f, "world y");
    ExpectNear(-1.0f, worldPosition.z, 0.0001f, "world z");
}

void WorldDeltaRoundsToNearestGridDelta()
{
    const glm::ivec3 gridDelta = UGCPlatformGrid::CalculateGridDelta(
        glm::vec3(2.9f, -2.9f, 1.0f), 2.0f);

    ExpectEqual(1, gridDelta.x, "grid delta x");
    ExpectEqual(-1, gridDelta.y, "grid delta y");
    ExpectEqual(1, gridDelta.z, "grid delta z");
}

void FootprintCellsExpandFromAnchorTowardNegativeAxes()
{
    const std::vector<glm::ivec3> cells =
        UGCPlatformGrid::CalculateFootprintCells(
            glm::ivec3(5, 2, 7), 2);

    ExpectEqual(std::size_t{4}, cells.size(), "footprint cell count");
    ExpectTrue(cells[0] == glm::ivec3(5, 2, 7), "first cell");
    ExpectTrue(cells[1] == glm::ivec3(5, 2, 6), "second cell");
    ExpectTrue(cells[2] == glm::ivec3(4, 2, 7), "third cell");
    ExpectTrue(cells[3] == glm::ivec3(4, 2, 6), "fourth cell");
}

void FootprintSideLengthIsClampedToSupportedRange()
{
    const std::vector<glm::ivec3> minimumCells =
        UGCPlatformGrid::CalculateFootprintCells(glm::ivec3(0), 0);
    const std::vector<glm::ivec3> maximumCells =
        UGCPlatformGrid::CalculateFootprintCells(glm::ivec3(0), 8);

    ExpectEqual(std::size_t{1}, minimumCells.size(), "minimum footprint");
    ExpectEqual(std::size_t{9}, maximumCells.size(), "maximum footprint");
}

void PlacementSnapsToIntersectionsAndSelectedLayer()
{
    const glm::vec3 snappedPosition =
        UGCPlatformGrid::SnapToGridIntersections(
            glm::vec3(2.9f, 100.0f, -2.9f), 2.0f, 3);

    ExpectNear(2.0f, snappedPosition.x, 0.0001f, "snapped x");
    ExpectNear(6.0f, snappedPosition.y, 0.0001f, "snapped y");
    ExpectNear(-2.0f, snappedPosition.z, 0.0001f, "snapped z");
}

void CellPreviewUsesCellCenterAndPreservesHeight()
{
    const glm::vec3 previewPosition =
        UGCPlatformGrid::CalculateCellPreviewPosition(
            glm::vec3(-0.1f, 4.0f, 2.1f), 2.0f);

    ExpectNear(-1.0f, previewPosition.x, 0.0001f, "preview x");
    ExpectNear(4.0f, previewPosition.y, 0.0001f, "preview y");
    ExpectNear(3.0f, previewPosition.z, 0.0001f, "preview z");
}

void FootprintPreviewMatchesGeneratedArea()
{
    const glm::vec3 previewPosition =
        UGCPlatformGrid::CalculateFootprintPreviewPosition(
            glm::vec3(4.0f, 2.0f, 6.0f), 2.0f, 3);
    const glm::vec3 previewScale =
        UGCPlatformGrid::CalculateFootprintPreviewScale(2.0f, 3);

    ExpectNear(3.0f, previewPosition.x, 0.0001f, "preview x");
    ExpectNear(2.0f, previewPosition.y, 0.0001f, "preview y");
    ExpectNear(5.0f, previewPosition.z, 0.0001f, "preview z");
    ExpectNear(3.0f, previewScale.x, 0.0001f, "preview scale x");
    ExpectNear(0.2f, previewScale.y, 0.0001f, "preview scale y");
    ExpectNear(3.0f, previewScale.z, 0.0001f, "preview scale z");
}

}

void RegisterUGCPlatformGridTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.emplace_back(
        "UGCPlatformGrid.InvalidGridSizeUsesMinimumPositiveSize",
        InvalidGridSizeUsesMinimumPositiveSize);
    tests.emplace_back(
        "UGCPlatformGrid.WorldPositionConvertsToGridUsingFloorForHorizontalAxes",
        WorldPositionConvertsToGridUsingFloorForHorizontalAxes);
    tests.emplace_back(
        "UGCPlatformGrid.NegativeWorldPositionUsesLowerGridCell",
        NegativeWorldPositionUsesLowerGridCell);
    tests.emplace_back(
        "UGCPlatformGrid.GridPositionConvertsToCellCenter",
        GridPositionConvertsToCellCenter);
    tests.emplace_back(
        "UGCPlatformGrid.WorldDeltaRoundsToNearestGridDelta",
        WorldDeltaRoundsToNearestGridDelta);
    tests.emplace_back(
        "UGCPlatformGrid.FootprintCellsExpandFromAnchorTowardNegativeAxes",
        FootprintCellsExpandFromAnchorTowardNegativeAxes);
    tests.emplace_back(
        "UGCPlatformGrid.FootprintSideLengthIsClampedToSupportedRange",
        FootprintSideLengthIsClampedToSupportedRange);
    tests.emplace_back(
        "UGCPlatformGrid.PlacementSnapsToIntersectionsAndSelectedLayer",
        PlacementSnapsToIntersectionsAndSelectedLayer);
    tests.emplace_back(
        "UGCPlatformGrid.CellPreviewUsesCellCenterAndPreservesHeight",
        CellPreviewUsesCellCenterAndPreservesHeight);
    tests.emplace_back(
        "UGCPlatformGrid.FootprintPreviewMatchesGeneratedArea",
        FootprintPreviewMatchesGeneratedArea);
}
