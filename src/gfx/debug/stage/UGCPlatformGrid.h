#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace UGCPlatformGrid {

float SanitizeGridSize(float gridSize);
glm::ivec3 CalculateGridPosition(
    const glm::vec3& worldPosition,
    float gridSize);
glm::vec3 CalculateCellWorldPosition(
    const glm::ivec3& gridPosition,
    float gridSize);
glm::ivec3 CalculateGridDelta(
    const glm::vec3& worldDelta,
    float gridSize);
std::vector<glm::ivec3> CalculateFootprintCells(
    const glm::ivec3& anchorGridPosition,
    int footprintSideLength);
glm::vec3 SnapToGridIntersections(
    const glm::vec3& worldPosition,
    float gridSize,
    int gridLayer);
glm::vec3 CalculateCellPreviewPosition(
    const glm::vec3& worldPosition,
    float gridSize);
glm::vec3 CalculateFootprintPreviewPosition(
    const glm::vec3& worldPosition,
    float gridSize,
    int footprintSideLength);
glm::vec3 CalculateFootprintPreviewScale(
    float gridSize,
    int footprintSideLength);

}
