#include "system/camera/CameraLateralOrbit.h"

#include <algorithm>
#include <cmath>

namespace CameraLateralOrbit {
std::optional<glm::quat> CalculateRotation(const glm::vec3& cameraPosition, const glm::vec3& playerPosition,
                                           const glm::vec3& up, const glm::vec3& inputMovementDelta, float strength)
{
    constexpr float directionEpsilonSquared = 0.000001f;
    if (strength <= 0.0f || glm::dot(up, up) <= directionEpsilonSquared) {
        return std::nullopt;
    }
    const auto axis = glm::normalize(up);
    const auto offset = cameraPosition - playerPosition;
    const auto horizontal = offset - axis * glm::dot(offset, axis);
    const float radius = glm::length(horizontal);
    // 真上付近では小さな移動が大きな角度になるため、追加周回を抑える。
    if (radius < 0.5f) {
        return std::nullopt;
    }
    const auto lateral = glm::cross(axis, horizontal / radius);
    const float distance = glm::dot(inputMovementDelta, lateral);
    const float angle = -std::clamp(strength, 0.0f, 1.0f) * distance / radius;
    if (std::abs(angle) < 0.000001f) {
        return std::nullopt;
    }
    // 大きなフレーム時間でも一度の追加回転を抑え、視点の急変を防ぐ。
    return glm::angleAxis(std::clamp(angle, -glm::radians(10.0f), glm::radians(10.0f)), axis);
}
} // namespace CameraLateralOrbit
