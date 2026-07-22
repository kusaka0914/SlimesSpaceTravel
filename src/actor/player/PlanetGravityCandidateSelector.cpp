#include "actor/player/PlanetGravityCandidateSelector.h"

#include "actor/Planet.h"

#include <algorithm>
#include <cmath>
#include <limits>

PlanetDistanceCandidate PlanetGravityCandidateSelector::FindNearestPlanet(const glm::vec3& playerPos,
                                                                          const std::vector<Planet*>& planets) const
{
    PlanetDistanceCandidate nearest;
    nearest.surfaceDistance = std::numeric_limits<float>::max();

    for (Planet* planet : planets) {
        if (!planet) {
            continue;
        }

        if (!planet->GetIsActive()) {
            continue;
        }

        const float surfaceDistance = CalculateSurfaceDistance(playerPos, *planet);

        if (surfaceDistance >= nearest.surfaceDistance) {
            continue;
        }

        nearest.planet = planet;
        nearest.surfaceDistance = surfaceDistance;
    }

    return nearest;
}

float PlanetGravityCandidateSelector::CalculateSurfaceDistance(const glm::vec3& playerPos, const Planet& planet) const
{
    const float centerDistance = glm::length(playerPos - planet.GetPos());

    if (centerDistance < 1e-6f) {
        return 0.0f;
    }

    const float surfaceRadius = CalculateApproximateSurfaceRadius(playerPos, planet);

    return std::max(0.0f, centerDistance - surfaceRadius);
}

float PlanetGravityCandidateSelector::CalculateApproximateSurfaceRadius(const glm::vec3& playerPos,
                                                                        const Planet& planet) const
{
    constexpr float minRadius = 0.001f;

    const glm::vec3 absoluteScale = glm::abs(planet.GetScale());

    if (planet.GetPlanetShape() == Planet::PlanetShape::Sphere) {
        return std::max(std::abs(planet.GetRadius()), minRadius);
    }

    if (planet.GetPlanetShape() == Planet::PlanetShape::Normal) {
        return std::max({absoluteScale.x, absoluteScale.y, absoluteScale.z, minRadius});
    }

    const glm::vec3 toPlayer = playerPos - planet.GetPos();

    const float centerDistance = glm::length(toPlayer);

    if (centerDistance < 1e-6f) {
        return std::max({absoluteScale.x, absoluteScale.y, absoluteScale.z, minRadius});
    }

    const glm::vec3 direction = glm::normalize(toPlayer);

    const glm::vec3 radii(std::max(absoluteScale.x, minRadius), std::max(absoluteScale.y, minRadius),
                          std::max(absoluteScale.z, minRadius));

    const float denominator = direction.x * direction.x / (radii.x * radii.x) +
                              direction.y * direction.y / (radii.y * radii.y) +
                              direction.z * direction.z / (radii.z * radii.z);

    if (denominator < 1e-6f) {
        return std::max({radii.x, radii.y, radii.z});
    }

    return 1.0f / std::sqrt(denominator);
}