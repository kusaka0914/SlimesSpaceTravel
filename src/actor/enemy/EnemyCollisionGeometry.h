#pragma once

#include <array>
#include <glm/glm.hpp>

class Enemy;

namespace EnemyCollisionGeometry {

// ワールド空間の有向境界ボックス。axesは正規直交する各軸、halfExtentsは各軸の半幅。
struct ModelBounds {
    glm::vec3 center{0.0f};
    std::array<glm::vec3, 3> axes{
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)};
    glm::vec3 halfExtents{0.0f};
};

// モデル境界・姿勢・スケールからboundsを作る。必要な情報や有効な体積がなければfalse。失敗時のboundsは使わない。
bool TryCreateModelBounds(const Enemy& enemy, ModelBounds& bounds);

// pointに最も近いボックス内のワールド位置。pointが内側ならその位置を返す。
glm::vec3 CalculateClosestPoint(
    const ModelBounds& bounds,
    const glm::vec3& point);

// 中心・頂点・pointへの最近傍点からなる9個のワールド位置を返す。厳密な交差判定ではなく候補点の抽出。
std::array<glm::vec3, 9> CreateCandidatePoints(
    const ModelBounds& bounds,
    const glm::vec3& point);

// 中心からdirection方向への投影半幅。距離として使う場合、directionは単位ベクトルを渡す。
float CalculateSupportDistance(
    const ModelBounds& bounds,
    const glm::vec3& direction);

// ワールド座標の線分と、ボックス各軸方向へexpansionだけ広げた境界との交差を判定する。
bool DoesSegmentIntersectExpandedBounds(
    const ModelBounds& bounds,
    const glm::vec3& segmentStart,
    const glm::vec3& segmentEnd,
    const glm::vec3& expansion);

}
