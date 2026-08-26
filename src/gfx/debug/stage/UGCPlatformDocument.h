#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

struct UGCPlatformCell {
    int planetIndex = 0;
    float gridSize = 1.0f;
    glm::ivec3 gridPosition{0};
    std::string behavior = "normal";
    glm::ivec3 movementDeltaCells{0};
};

struct UGCGeneratedPlatformRegion {
    int planetIndex = 0;
    float gridSize = 1.0f;
    int gridLayer = 0;
    int minimumX = 0;
    int minimumZ = 0;
    int maximumX = 0;
    int maximumZ = 0;
    std::string behavior = "normal";
    glm::ivec3 movementDeltaCells{0};
};

struct UGCPlatformCellTranslationRegion {
    int planetIndex = 0;
    float gridSize = 1.0f;
    int gridLayer = 0;
    int minimumX = 0;
    int minimumZ = 0;
    int maximumX = 0;
    int maximumZ = 0;
    glm::ivec3 gridDelta{0};
};

namespace UGCPlatformDocument {

constexpr const char* PlatformCellsKey = "ugcPlatformCells";
constexpr const char* GeneratedPlatformKey = "ugcGeneratedPlatform";

bool TryReadCell(const YAML::Node& cellNode, UGCPlatformCell& outCell);
YAML::Node CreateCellNode(const UGCPlatformCell& cell);

int AddFootprint(
    YAML::Node& stageConfig,
    const UGCPlatformCell& anchorCell,
    int footprintSideLength);
bool RemoveCellAtGridPosition(
    YAML::Node& stageConfig,
    int planetIndex,
    float gridSize,
    const glm::ivec3& gridPosition);
bool RemoveClosestCell(
    YAML::Node& stageConfig,
    int planetIndex,
    float gridSize,
    int gridLayer,
    const glm::vec3& hitPosition);

bool ResolveLayerAtGridPosition(
    const YAML::Node& stageConfig,
    int planetIndex,
    float gridSize,
    const glm::ivec3& gridPosition,
    int preferredGridLayer,
    int& outGridLayer);
int ResolvePlacementLayerAtGridPosition(
    const YAML::Node& stageConfig,
    int planetIndex,
    float gridSize,
    const glm::ivec3& gridPosition,
    int emptyColumnGridLayer);
int TranslateCells(
    YAML::Node& stageConfig,
    const std::vector<UGCPlatformCellTranslationRegion>& regions);

void RemoveGeneratedPlatforms(YAML::Node& stageConfig);
std::vector<UGCGeneratedPlatformRegion> CalculateGeneratedPlatformRegions(
    const YAML::Node& stageConfig);

}
