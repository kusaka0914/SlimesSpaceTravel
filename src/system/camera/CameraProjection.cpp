#include "system/camera/CameraProjection.h"

#include "Game.h"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

glm::mat4 CalculateCameraProjection(
    const Game& game,
    float aspectRatio,
    float fieldOfViewDegrees)
{
    const float safeAspectRatio = std::max(0.01f, aspectRatio);
    constexpr float nearPlane = 0.1f;
    constexpr float farPlane = 1000.0f;

    if (game.GetIsUGCMode() && game.GetIsUGCOrthographicView()) {
        const float halfHeight = game.GetUGCOrthographicHalfHeight();
        const float halfWidth = halfHeight * safeAspectRatio;
        return glm::ortho(
            -halfWidth,
            halfWidth,
            -halfHeight,
            halfHeight,
            nearPlane,
            farPlane);
    }

    return glm::perspective(
        glm::radians(fieldOfViewDegrees),
        safeAspectRatio,
        nearPlane,
        farPlane);
}
