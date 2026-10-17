#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

// 保存単位となるセル。gridPosition のX/Zはセル添字、Yはレイヤーを表す。
struct UGCPlatformCell {
    int planetIndex = 0;
    float gridSize = 1.0f;
    glm::ivec3 gridPosition{0};
    std::string behavior = "normal";
    glm::ivec3 movementDeltaCells{0};
};

// 同じ惑星・レイヤー・挙動からまとめた生成足場の矩形範囲。X/Zの最小・最大添字を両端とも含む。
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

// 移動元の矩形範囲とグリッド単位の移動量。X/Zの最小・最大添字を両端とも含む。
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

// セルと生成足場のYAMLを操作する。ファイル保存や実行中Actorの更新は呼び出し元が行う。
namespace UGCPlatformDocument {

constexpr const char* PlatformCellsKey = "ugcPlatformCells";
constexpr const char* GeneratedPlatformKey = "ugcGeneratedPlatform";

// 必須キーの不足・型不正時は false。失敗時のoutCellは部分更新されることがある。
bool TryReadCell(const YAML::Node& cellNode, UGCPlatformCell& outCell);
YAML::Node CreateCellNode(const UGCPlatformCell& cell);

// 既存セルと同じ位置への追加を避け、実際に追加したセル数を返す。
int AddFootprint(YAML::Node& stageConfig, const UGCPlatformCell& anchorCell, int footprintSideLength);
bool RemoveCellAtGridPosition(YAML::Node& stageConfig, int planetIndex, float gridSize, const glm::ivec3& gridPosition);

// 指定惑星・レイヤー内でhitPositionに最も近いセルを削除する。対象がなければ false。
bool RemoveClosestCell(YAML::Node& stageConfig, int planetIndex, float gridSize, int gridLayer,
                       const glm::vec3& hitPosition);
bool RemoveMovingDestinationCellAtGridPosition(YAML::Node& stageConfig, const UGCGeneratedPlatformRegion& sourceRegion,
                                               const glm::ivec3& destinationGridPosition);

// 同じX/Z列のpreferredGridLayerを優先し、なければ最上段を返す。空の列では false。
bool ResolveLayerAtGridPosition(const YAML::Node& stageConfig, int planetIndex, float gridSize,
                                const glm::ivec3& gridPosition, int preferredGridLayer, int& outGridLayer);

// 同じX/Z列の最上段の1つ上、空の列ではemptyColumnGridLayerを返す。
int ResolvePlacementLayerAtGridPosition(const YAML::Node& stageConfig, int planetIndex, float gridSize,
                                        const glm::ivec3& gridPosition, int emptyColumnGridLayer);

// 各セルに最初に一致した範囲の差分だけを適用し、移動したセル数を返す。
int TranslateCells(YAML::Node& stageConfig, const std::vector<UGCPlatformCellTranslationRegion>& regions);
bool TranslateGeneratedPlatformRegionMetadata(YAML::Node& platformNode, const glm::ivec3& gridDelta);

void RemoveGeneratedPlatforms(YAML::Node& stageConfig);

// 同じ惑星・グリッド幅・レイヤー・挙動・移動量を持つセルを矩形へまとめる。
std::vector<UGCGeneratedPlatformRegion> CalculateGeneratedPlatformRegions(const YAML::Node& stageConfig);
YAML::Node FindMatchingGeneratedPlatformNode(const YAML::Node& stageConfig, const UGCGeneratedPlatformRegion& region);

} // namespace UGCPlatformDocument
