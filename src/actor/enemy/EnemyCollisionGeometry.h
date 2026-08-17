#pragma once

#include <array>
#include <glm/glm.hpp>

class Enemy;

namespace EnemyCollisionGeometry {

struct ModelBounds {
    glm::vec3 center{0.0f};
    std::array<glm::vec3, 3> axes{
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)};
    glm::vec3 halfExtents{0.0f};
};

bool TryCreateModelBounds(const Enemy& enemy, ModelBounds& bounds);

glm::vec3 CalculateClosestPoint(
    const ModelBounds& bounds,
    const glm::vec3& point);

std::array<glm::vec3, 9> CreateCandidatePoints(
    const ModelBounds& bounds,
    const glm::vec3& point);

float CalculateSupportDistance(
    const ModelBounds& bounds,
    const glm::vec3& direction);

bool DoesSegmentIntersectExpandedBounds(
    const ModelBounds& bounds,
    const glm::vec3& segmentStart,
    const glm::vec3& segmentEnd,
    const glm::vec3& expansion);

} // namespace EnemyCollisionGeometry
