#include "system/physics/PlayerCollisionShapeGeometry.h"

#include <cmath>
#include <glm/gtc/constants.hpp>

glm::vec3 PlayerCollisionShapeGeometry::CalculateLocalSurfacePoint(
    float collisionWidth,
    float collisionHeight,
    float collisionDepth,
    int latitudeIndex,
    int longitudeIndex)
{
    const float latitudeRadians =
        -glm::half_pi<float>() +
        glm::pi<float>() * static_cast<float>(latitudeIndex) /
            static_cast<float>(LatitudeSegmentCount);
    const float longitudeRadians =
        glm::two_pi<float>() * static_cast<float>(longitudeIndex) /
        static_cast<float>(LongitudeSegmentCount);

    const float ringRadiusRatio = std::cos(latitudeRadians);
    return glm::vec3(
        collisionWidth * 0.5f * ringRadiusRatio * std::cos(longitudeRadians),
        collisionHeight * 0.5f * std::sin(latitudeRadians),
        collisionDepth * 0.5f * ringRadiusRatio * std::sin(longitudeRadians));
}
