#pragma once

#include <glm/glm.hpp>

namespace PlayerCollisionShapeGeometry {

inline constexpr int LatitudeSegmentCount = 8;
inline constexpr int LongitudeSegmentCount = 16;

glm::vec3 CalculateLocalSurfacePoint(
    float collisionWidth,
    float collisionHeight,
    float collisionDepth,
    int latitudeIndex,
    int longitudeIndex);

} // namespace PlayerCollisionShapeGeometry
