#include "gfx/debug/stage/UGCPlatformGrid.h"

#include <algorithm>
#include <cmath>

namespace UGCPlatformGrid {

float SanitizeGridSize(float gridSize)
{
    return std::max(0.01f, gridSize);
}

glm::ivec3 CalculateGridPosition(
    const glm::vec3& worldPosition,
    float gridSize)
{
    const float safeGridSize = SanitizeGridSize(gridSize);
    return glm::ivec3(
        static_cast<int>(std::floor(worldPosition.x / safeGridSize)),
        static_cast<int>(std::round(worldPosition.y / safeGridSize)),
        static_cast<int>(std::floor(worldPosition.z / safeGridSize)));
}

glm::vec3 CalculateCellWorldPosition(
    const glm::ivec3& gridPosition,
    float gridSize)
{
    const float safeGridSize = SanitizeGridSize(gridSize);
    return glm::vec3(
        (static_cast<float>(gridPosition.x) + 0.5f) * safeGridSize,
        static_cast<float>(gridPosition.y) * safeGridSize,
        (static_cast<float>(gridPosition.z) + 0.5f) * safeGridSize);
}

glm::ivec3 CalculateGridDelta(
    const glm::vec3& worldDelta,
    float gridSize)
{
    const float safeGridSize = SanitizeGridSize(gridSize);
    return glm::ivec3(
        static_cast<int>(std::round(worldDelta.x / safeGridSize)),
        static_cast<int>(std::round(worldDelta.y / safeGridSize)),
        static_cast<int>(std::round(worldDelta.z / safeGridSize)));
}

std::vector<glm::ivec3> CalculateFootprintCells(
    const glm::ivec3& anchorGridPosition,
    int footprintSideLength)
{
    const int safeSideLength = std::clamp(footprintSideLength, 1, 3);
    std::vector<glm::ivec3> gridPositions;
    gridPositions.reserve(safeSideLength * safeSideLength);
    for (int offsetX = 0; offsetX < safeSideLength; ++offsetX) {
        for (int offsetZ = 0; offsetZ < safeSideLength; ++offsetZ) {
            gridPositions.emplace_back(
                anchorGridPosition.x - offsetX,
                anchorGridPosition.y,
                anchorGridPosition.z - offsetZ);
        }
    }
    return gridPositions;
}

glm::vec3 SnapToGridIntersections(
    const glm::vec3& worldPosition,
    float gridSize,
    int gridLayer)
{
    const float safeGridSize = SanitizeGridSize(gridSize);
    return glm::vec3(
        std::round(worldPosition.x / safeGridSize) * safeGridSize,
        static_cast<float>(gridLayer) * safeGridSize,
        std::round(worldPosition.z / safeGridSize) * safeGridSize);
}

glm::vec3 CalculateCellPreviewPosition(
    const glm::vec3& worldPosition,
    float gridSize)
{
    const float safeGridSize = SanitizeGridSize(gridSize);
    return glm::vec3(
        (std::floor(worldPosition.x / safeGridSize) + 0.5f) *
            safeGridSize,
        worldPosition.y,
        (std::floor(worldPosition.z / safeGridSize) + 0.5f) *
            safeGridSize);
}

glm::vec3 CalculateFootprintPreviewPosition(
    const glm::vec3& worldPosition,
    float gridSize,
    int footprintSideLength)
{
    const float safeGridSize = SanitizeGridSize(gridSize);
    const float safeSideLength = static_cast<float>(
        std::clamp(footprintSideLength, 1, 3));
    return glm::vec3(
        (std::floor(worldPosition.x / safeGridSize) +
         1.0f - safeSideLength * 0.5f) * safeGridSize,
        worldPosition.y,
        (std::floor(worldPosition.z / safeGridSize) +
         1.0f - safeSideLength * 0.5f) * safeGridSize);
}

glm::vec3 CalculateFootprintPreviewScale(
    float gridSize,
    int footprintSideLength)
{
    const float safeGridSize = SanitizeGridSize(gridSize);
    const float safeSideLength = static_cast<float>(
        std::clamp(footprintSideLength, 1, 3));
    return glm::vec3(
        safeSideLength * safeGridSize * 0.5f,
        0.1f * safeGridSize,
        safeSideLength * safeGridSize * 0.5f);
}

}
