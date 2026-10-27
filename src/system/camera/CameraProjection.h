#pragma once

#include <glm/mat4x4.hpp>

struct CameraProjectionSettings {
    bool isOrthographic = false;
    float orthographicHalfHeight = 1.0f;
};

// 平行投影と透視投影で共通のクリップ面を使う。aspectRatioは最小0.01へ補正し、画角は度単位。
glm::mat4 CalculateCameraProjection(const CameraProjectionSettings& settings, float aspectRatio,
                                    float fieldOfViewDegrees);

// ニアクリップ面の四隅を含む球の半径。FOVと分割後の縦横比を使う。
float CalculateCameraClearanceRadius(float fieldOfViewDegrees, float aspectRatio);
