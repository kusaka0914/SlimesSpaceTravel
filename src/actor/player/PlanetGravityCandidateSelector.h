#pragma once

#include <glm/glm.hpp>

#include <vector>

class Planet;

struct PlanetDistanceCandidate {
    Planet* planet = nullptr;
    float surfaceDistance = 0.0f;
};

class PlanetGravityCandidateSelector {
public:
    PlanetDistanceCandidate FindNearestPlanet(const glm::vec3& playerPos, const std::vector<Planet*>& planets) const;

    float CalculateSurfaceDistance(const glm::vec3& playerPos, const Planet& planet) const;

private:
    float CalculateApproximateSurfaceRadius(const glm::vec3& playerPos, const Planet& planet) const;
};