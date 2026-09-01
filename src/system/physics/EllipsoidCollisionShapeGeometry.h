#pragma once

#include <glm/glm.hpp>

class btConvexHullShape;

namespace EllipsoidCollisionShapeGeometry {

inline constexpr int LatitudeSegmentCount = 8;
inline constexpr int LongitudeSegmentCount = 16;

glm::vec3 CalculateLocalSurfacePoint(
    float collisionWidth,
    float collisionHeight,
    float collisionDepth,
    int latitudeIndex,
    int longitudeIndex);

void AddSurfacePoints(
    btConvexHullShape& shape,
    const glm::vec3& dimensions);

}
