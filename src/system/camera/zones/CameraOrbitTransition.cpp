#include "system/camera/zones/CameraOrbitTransition.h"

#include "system/camera/CameraCollisionResolver.h"
#include "system/camera/CameraProjection.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>

namespace {
glm::vec3 SafeDirection(const glm::vec3& value, const glm::vec3& fallback)
{
    return glm::dot(value, value) > 0.000001f ? glm::normalize(value) : fallback;
}

glm::vec3 BlendUp(const CameraPose& from, const CameraPose& to, float amount)
{
    const auto up = SafeDirection(from.up, {0, 1, 0});
    const auto rotation = glm::quat(up, SafeDirection(to.up, up));
    return glm::slerp(glm::quat(1, 0, 0, 0), rotation, amount) * up;
}

struct OrbitFrame {
    glm::vec3 fromOffset;
    glm::vec3 toOffset;
    glm::vec3 fromTangent;
    glm::vec3 toTangent;
    glm::vec3 fromDirection;
    float yaw;
};

OrbitFrame CalculateOrbitFrame(const CameraPose& from, const CameraPose& to, const glm::vec3& up)
{
    OrbitFrame frame;
    frame.fromOffset = from.position - from.target;
    frame.toOffset = to.position - to.target;
    frame.fromTangent = frame.fromOffset - up * glm::dot(frame.fromOffset, up);
    frame.toTangent = frame.toOffset - up * glm::dot(frame.toOffset, up);
    const auto perpendicular =
        SafeDirection(glm::cross(up, std::abs(up.x) < 0.9f ? glm::vec3(1, 0, 0) : glm::vec3(0, 0, 1)), {0, 0, 1});
    frame.fromDirection = SafeDirection(frame.fromTangent, perpendicular);
    const auto toDirection = SafeDirection(frame.toTangent, frame.fromDirection);
    const float alignment = std::clamp(glm::dot(frame.fromDirection, toDirection), -1.0f, 1.0f);
    const float cross = glm::dot(glm::cross(frame.fromDirection, toDirection), up);
    frame.yaw = std::atan2(cross, alignment);
    // 正反対の構図で符号が丸め誤差に依存しないよう、初期経路だけ正方向へ揃える。
    if (alignment < -0.999999f && std::abs(cross) < 0.000001f) {
        frame.yaw = glm::pi<float>();
    }
    return frame;
}

float MeasurePath(const CameraPose& from, const CameraPose& to, float yaw, const CameraCollisionResolver& collision,
                  float aspectRatio, const std::vector<Actor*>& ignoredActors)
{
    const float orbitRadius = std::max(glm::length(from.position - from.target), glm::length(to.position - to.target));
    const float upAngle = std::acos(
        std::clamp(glm::dot(SafeDirection(from.up, {0, 1, 0}), SafeDirection(to.up, {0, 1, 0})), -1.0f, 1.0f));
    const int segments =
        std::clamp(static_cast<int>(std::ceil((std::abs(yaw) + upAngle) / glm::radians(7.5f))), 12, 96);
    const float angularStep = (std::abs(yaw) + upAngle) / segments;
    // 球スイープは点間の弦を調べるため、円弧との差も半径へ加えて薄い壁の取りこぼしを抑える。
    const float arcMargin = orbitRadius * (1.0f - std::cos(angularStep * 0.5f));
    const float radius =
        CalculateCameraClearanceRadius(std::max(from.fieldOfViewDegrees, to.fieldOfViewDegrees), aspectRatio) +
        arcMargin;
    std::vector<glm::vec3> points;
    points.reserve(segments + 1);
    for (int index = 0; index <= segments; ++index) {
        points.push_back(CameraOrbitTransition::Blend(from, to, static_cast<float>(index) / segments, yaw).position);
    }
    return collision.MeasureCameraPathObstruction(points, radius, ignoredActors);
}
} // namespace

CameraPose CameraOrbitTransition::Blend(const CameraPose& from, const CameraPose& to, float amount,
                                        std::optional<float> referenceYaw)
{
    if (amount <= 0.0f) {
        return from;
    }
    if (amount >= 1.0f) {
        return to;
    }
    CameraPose result;
    result.target = glm::mix(from.target, to.target, amount);
    result.up = BlendUp(from, to, amount);
    auto frame = CalculateOrbitFrame(from, to, result.up);
    if (referenceYaw) {
        // 到達構図が動いて±πや0を跨いでも、選択済みの経路から一周分飛ばないようにする。
        frame.yaw += glm::two_pi<float>() * std::round((*referenceYaw - frame.yaw) / glm::two_pi<float>());
    }
    const auto direction = glm::angleAxis(frame.yaw * amount, result.up) * frame.fromDirection;
    const float distance = glm::mix(glm::length(frame.fromTangent), glm::length(frame.toTangent), amount);
    const float height = glm::mix(glm::dot(frame.fromOffset, result.up), glm::dot(frame.toOffset, result.up), amount);
    result.position = result.target + direction * distance + result.up * height;
    result.fieldOfViewDegrees = glm::mix(from.fieldOfViewDegrees, to.fieldOfViewDegrees, amount);
    return result;
}

float CameraOrbitTransition::SelectYaw(const CameraPose& from, const CameraPose& to,
                                       const CameraCollisionResolver& collision, float aspectRatio,
                                       const std::vector<Actor*>& ignoredActors)
{
    const float shortest = CalculateOrbitFrame(from, to, BlendUp(from, to, 0.5f)).yaw;
    // 方位差のない移動で、壁を避けるためだけにほぼ一周する回転は追加しない。
    if (std::abs(shortest) < glm::radians(1.0f)) {
        return shortest;
    }
    const float shortObstruction = MeasurePath(from, to, shortest, collision, aspectRatio, ignoredActors);
    if (shortObstruction <= 0.0f) {
        return shortest;
    }
    const float opposite = shortest > 0.0f ? shortest - glm::two_pi<float>() : shortest + glm::two_pi<float>();
    const float oppositeObstruction = MeasurePath(from, to, opposite, collision, aspectRatio, ignoredActors);
    return oppositeObstruction + 0.01f < shortObstruction ? opposite : shortest;
}
