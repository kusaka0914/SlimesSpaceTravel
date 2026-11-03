#include "system/camera/CameraProjection.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace {
constexpr float cameraNearPlane = 0.1f;
}

glm::mat4 CalculateCameraProjection(const CameraProjectionSettings& settings, float aspectRatio,
                                    float fieldOfViewDegrees)
{
    const float safeAspectRatio = std::max(0.01f, aspectRatio);
    constexpr float farPlane = 1000.0f;

    if (settings.isOrthographic) {
        const float halfHeight = settings.orthographicHalfHeight;
        const float halfWidth = halfHeight * safeAspectRatio;
        return glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, cameraNearPlane, farPlane);
    }

    return glm::perspective(glm::radians(fieldOfViewDegrees), safeAspectRatio, cameraNearPlane, farPlane);
}

float CalculateCameraClearanceRadius(float fieldOfViewDegrees, float aspectRatio)
{
    const float halfHeight =
        cameraNearPlane * std::tan(glm::radians(std::clamp(fieldOfViewDegrees, 10.0f, 120.0f)) * 0.5f);
    const float halfWidth = halfHeight * std::max(0.01f, aspectRatio);
    return std::max(
        0.3f, std::sqrt(cameraNearPlane * cameraNearPlane + halfHeight * halfHeight + halfWidth * halfWidth) + 0.05f);
}
