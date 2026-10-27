#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <optional>

namespace CameraLateralOrbit {
// 地上の入力移動量から、Playerを中心とする追加周回の回転を返す。upは地面法線。
// strengthは自然な旋回に加える割合（0～1）。追加周回が不要・不安定な場合はnullopt。
std::optional<glm::quat> CalculateRotation(const glm::vec3& cameraPosition, const glm::vec3& playerPosition,
                                           const glm::vec3& up, const glm::vec3& inputMovementDelta, float strength);
} // namespace CameraLateralOrbit
