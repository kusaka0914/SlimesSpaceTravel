#pragma once

#include <glm/glm.hpp>
#include <vector>

// UGCのX/ZセルとYレイヤーをワールド座標へ相互変換する。gridSize は最小0.01に補正する。
namespace UGCPlatformGrid {

float SanitizeGridSize(float gridSize);

// X/Zはセル境界で切り下げ、Yは最寄りのレイヤーへ丸める。
glm::ivec3 CalculateGridPosition(const glm::vec3& worldPosition, float gridSize);

// X/Zはセル中央、Yは指定レイヤーの平面上のワールド座標を返す。
glm::vec3 CalculateCellWorldPosition(const glm::ivec3& gridPosition, float gridSize);
glm::ivec3 CalculateGridDelta(const glm::vec3& worldDelta, float gridSize);

// 一辺を1〜3セルに制限し、アンカーからX/Zの負方向へ広がるセル群を返す。
std::vector<glm::ivec3> CalculateFootprintCells(const glm::ivec3& anchorGridPosition, int footprintSideLength);
glm::vec3 SnapToGridIntersections(const glm::vec3& worldPosition, float gridSize, int gridLayer);
glm::vec3 CalculateCellPreviewPosition(const glm::vec3& worldPosition, float gridSize);
glm::vec3 CalculateFootprintPreviewPosition(const glm::vec3& worldPosition, float gridSize, int footprintSideLength);

// 一辺のセル数とグリッド幅から、足場モデルの半サイズに対応するプレビュー倍率を返す。
glm::vec3 CalculateFootprintPreviewScale(float gridSize, int footprintSideLength);

} // namespace UGCPlatformGrid
