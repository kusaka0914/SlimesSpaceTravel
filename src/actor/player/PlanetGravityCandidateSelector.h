#pragma once

#include <glm/glm.hpp>

#include <vector>

class Planet;

// planetは非所有参照。surfaceDistanceは内外を区別しない非負の表面距離（ワールド単位）。
struct PlanetDistanceCandidate {
    Planet* planet = nullptr;
    float surfaceDistance = 0.0f;
};

// 重力で引き寄せる候補惑星を、プレイヤーから表面までの距離で選ぶ。
class PlanetGravityCandidateSelector {
public:
    // 有効かつ近接吸引可能な惑星から選ぶ。候補なしではplanetがnullptr、距離がfloatの最大値。
    PlanetDistanceCandidate FindNearestPlanet(const glm::vec3& playerPos, const std::vector<Planet*>& planets) const;

    // 楕円体は表面投影の距離、球は半径、Normalは最大スケールを球半径として距離を求める。
    float CalculateSurfaceDistance(const glm::vec3& playerPos, const Planet& planet) const;

private:
    float CalculateApproximateSurfaceRadius(const glm::vec3& playerPos, const Planet& planet) const;
};