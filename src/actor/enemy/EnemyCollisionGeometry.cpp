#include "actor/enemy/EnemyCollisionGeometry.h"

#include "actor/Enemy.h"
#include "system/mesh/LoadedModel.h"

#include <algorithm>
#include <cmath>

namespace EnemyCollisionGeometry {
namespace {
constexpr float geometryEpsilon = 0.000001f;
}

bool TryCreateModelBounds(const Enemy& enemy, ModelBounds& bounds)
{
    const LoadedModel* loadedModel = enemy.GetLoadedModel();
    if (!loadedModel || !loadedModel->hasBounds) {
        return false;
    }

    glm::vec3 modelForwardAxis = -enemy.GetForwardVec();
    glm::vec3 modelUpAxis = enemy.GetUpVec();
    glm::vec3 modelLeftAxis = -enemy.GetLeftVec();
    if (glm::dot(modelForwardAxis, modelForwardAxis) <= geometryEpsilon ||
        glm::dot(modelUpAxis, modelUpAxis) <= geometryEpsilon ||
        glm::dot(modelLeftAxis, modelLeftAxis) <= geometryEpsilon) {
        return false;
    }

    modelForwardAxis = glm::normalize(modelForwardAxis);
    modelUpAxis = glm::normalize(modelUpAxis);
    modelLeftAxis = glm::normalize(modelLeftAxis);

    const glm::vec3 modelScale = glm::abs(enemy.GetScale());
    const glm::vec3 localBoundsCenter =
        (loadedModel->boundsMinimum + loadedModel->boundsMaximum) *
        0.5f *
        enemy.GetScale();
    bounds.center =
        enemy.GetPos() +
        modelForwardAxis * localBoundsCenter.x +
        modelUpAxis * localBoundsCenter.y +
        modelLeftAxis * localBoundsCenter.z;
    bounds.axes = {
        modelForwardAxis,
        modelUpAxis,
        modelLeftAxis};
    bounds.halfExtents =
        (loadedModel->boundsMaximum - loadedModel->boundsMinimum) *
        0.5f *
        modelScale;
    return glm::all(
        glm::greaterThan(
            bounds.halfExtents,
            glm::vec3(geometryEpsilon)));
}

glm::vec3 CalculateClosestPoint(
    const ModelBounds& bounds,
    const glm::vec3& point)
{
    glm::vec3 closestPoint = bounds.center;
    const glm::vec3 offset = point - bounds.center;
    for (glm::length_t axisIndex = 0;
         axisIndex < bounds.axes.size();
         ++axisIndex) {
        const float coordinate = glm::dot(
            offset,
            bounds.axes[axisIndex]);
        const float clampedCoordinate = glm::clamp(
            coordinate,
            -bounds.halfExtents[axisIndex],
            bounds.halfExtents[axisIndex]);
        closestPoint +=
            bounds.axes[axisIndex] *
            clampedCoordinate;
    }
    return closestPoint;
}

std::array<glm::vec3, 9> CreateCandidatePoints(
    const ModelBounds& bounds,
    const glm::vec3& point)
{
    std::array<glm::vec3, 9> candidatePoints{};
    candidatePoints[0] = bounds.center;

    std::size_t candidateIndex = 1;
    for (int forwardSign : {-1, 1}) {
        for (int upSign : {-1, 1}) {
            for (int leftSign : {-1, 1}) {
                candidatePoints[candidateIndex++] =
                    bounds.center +
                    bounds.axes[0] *
                        (bounds.halfExtents.x *
                         static_cast<float>(forwardSign)) +
                    bounds.axes[1] *
                        (bounds.halfExtents.y *
                         static_cast<float>(upSign)) +
                    bounds.axes[2] *
                        (bounds.halfExtents.z *
                         static_cast<float>(leftSign));
            }
        }
    }

    candidatePoints[8] = CalculateClosestPoint(bounds, point);
    return candidatePoints;
}

float CalculateSupportDistance(
    const ModelBounds& bounds,
    const glm::vec3& direction)
{
    float supportDistance = 0.0f;
    for (glm::length_t axisIndex = 0;
         axisIndex < bounds.axes.size();
         ++axisIndex) {
        supportDistance +=
            std::abs(glm::dot(direction, bounds.axes[axisIndex])) *
            bounds.halfExtents[axisIndex];
    }
    return supportDistance;
}

bool DoesSegmentIntersectExpandedBounds(
    const ModelBounds& bounds,
    const glm::vec3& segmentStart,
    const glm::vec3& segmentEnd,
    const glm::vec3& expansion)
{
    float minimumSegmentTime = 0.0f;
    float maximumSegmentTime = 1.0f;
    const glm::vec3 localStartOffset = segmentStart - bounds.center;
    const glm::vec3 localEndOffset = segmentEnd - bounds.center;

    for (glm::length_t axisIndex = 0;
         axisIndex < bounds.axes.size();
         ++axisIndex) {
        const float localStart = glm::dot(
            localStartOffset,
            bounds.axes[axisIndex]);
        const float localDelta = glm::dot(
            localEndOffset - localStartOffset,
            bounds.axes[axisIndex]);
        const float expandedExtent =
            bounds.halfExtents[axisIndex] +
            expansion[axisIndex];

        if (std::abs(localDelta) <= geometryEpsilon) {
            if (localStart < -expandedExtent ||
                localStart > expandedExtent) {
                return false;
            }
            continue;
        }

        float entryTime =
            (-expandedExtent - localStart) / localDelta;
        float exitTime =
            (expandedExtent - localStart) / localDelta;
        if (entryTime > exitTime) {
            std::swap(entryTime, exitTime);
        }

        minimumSegmentTime = std::max(minimumSegmentTime, entryTime);
        maximumSegmentTime = std::min(maximumSegmentTime, exitTime);
        if (minimumSegmentTime > maximumSegmentTime) {
            return false;
        }
    }

    return maximumSegmentTime >= 0.0f && minimumSegmentTime <= 1.0f;
}
}
