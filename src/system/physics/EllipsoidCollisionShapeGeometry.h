#pragma once

#include <glm/glm.hpp>

class btConvexHullShape;

namespace EllipsoidCollisionShapeGeometry {
inline constexpr int LatitudeSegmentCount = 8;
inline constexpr int LongitudeSegmentCount = 16;

// 幅・高さ・奥行き（直径）から、緯度・経度の分割インデックスに対応するローカル表面点を求める。
glm::vec3 CalculateLocalSurfacePoint(float collisionWidth, float collisionHeight, float collisionDepth,
                                     int latitudeIndex, int longitudeIndex);

// dimensionsを各軸の直径として表面点を追加し、形状のローカルAABBを更新する。
void AddSurfacePoints(btConvexHullShape& shape, const glm::vec3& dimensions);

} // namespace EllipsoidCollisionShapeGeometry
