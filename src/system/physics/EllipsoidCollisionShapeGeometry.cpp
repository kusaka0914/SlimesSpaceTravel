#include "system/physics/EllipsoidCollisionShapeGeometry.h"

#include <btBulletDynamicsCommon.h>
#include <cmath>
#include <glm/gtc/constants.hpp>

glm::vec3 EllipsoidCollisionShapeGeometry::CalculateLocalSurfacePoint(
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
        collisionWidth * 0.5f * ringRadiusRatio *
            std::cos(longitudeRadians),
        collisionHeight * 0.5f * std::sin(latitudeRadians),
        collisionDepth * 0.5f * ringRadiusRatio *
            std::sin(longitudeRadians));
}

void EllipsoidCollisionShapeGeometry::AddSurfacePoints(
    btConvexHullShape& shape,
    const glm::vec3& dimensions)
{
    for (int latitudeIndex = 0;
         latitudeIndex <= LatitudeSegmentCount;
         ++latitudeIndex) {
        for (int longitudeIndex = 0;
             longitudeIndex < LongitudeSegmentCount;
             ++longitudeIndex) {
            const glm::vec3 localSurfacePoint =
                CalculateLocalSurfacePoint(
                    dimensions.x,
                    dimensions.y,
                    dimensions.z,
                    latitudeIndex,
                    longitudeIndex);
            shape.addPoint(
                btVector3(
                    localSurfacePoint.x,
                    localSurfacePoint.y,
                    localSurfacePoint.z),
                false);
        }
    }
    shape.recalcLocalAabb();
}
