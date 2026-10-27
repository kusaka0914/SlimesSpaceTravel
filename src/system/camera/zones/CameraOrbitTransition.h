#pragma once

#include "system/camera/CinematicCameraTypes.h"
#include <optional>
#include <vector>

class Actor;
class CameraCollisionResolver;

// 注視点の周囲を回る補間と、その左右経路の選択。角度はラジアン。
namespace CameraOrbitTransition {
// referenceYawを指定すると、その角度に連続する回転経路を使い、補間途中の回転方向反転を防ぐ。
CameraPose Blend(const CameraPose& from, const CameraPose& to, float amount,
                 std::optional<float> referenceYaw = std::nullopt);

// 両経路の接触区間を比較する。同等なら短い方を選ぶ。結果は遷移開始時に保存してBlendへ渡す。
float SelectYaw(const CameraPose& from, const CameraPose& to, const CameraCollisionResolver& collision,
                float aspectRatio, const std::vector<Actor*>& ignoredActors);
} // namespace CameraOrbitTransition
