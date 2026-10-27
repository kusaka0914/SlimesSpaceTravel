#pragma once

#include <functional>
#include <glm/glm.hpp>

class Game;
class Planet;

// 接地レイから上方向を求める処理。Actorの状態更新は呼び出し側とコールバックが担当する。
class ActorGroundResolver {
public:
    // trueを返した法線は採用しない。各レイの正規化済み法線と上方向を受け取る。
    using NormalRejector = std::function<bool(const glm::vec3& hitNormal, const glm::vec3& up)>;

    // 各レイが面に当たった直後に通知する。1回の探索中に複数回呼ばれる場合がある。
    using SurfaceDetectedCallback = std::function<void()>;

    // 各レイの法線が長さ・傾斜の検証を通過した時点で通知する。平均計算への採用前に呼ばれる。
    using CastSucceededCallback = std::function<void()>;

    // 中央と周囲4点から、中央と同じ物体の法線を重み付きで平均する。位置・方向はワールド空間。
    // footprintScaleは体の衝突倍率。周囲4点の間隔に適用し、惑星探索用のレイ長は維持する。
    // 中央のレイが失敗、または有効な法線を得られなければゼロベクトルを返す。
    static glm::vec3 CalculateAverageNormal(Game* game, const glm::vec3& pos, const glm::vec3& upVec,
                                            const glm::vec3& forwardVec, const glm::vec3& leftVec,
                                            const NormalRejector& shouldRejectNormal,
                                            const SurfaceDetectedCallback& onSurfaceDetected,
                                            const CastSucceededCallback& onCastSucceeded, float footprintScale = 1.0f);

    // 通常形状・惑星なしはY上向き、球は中心からの方向、楕円体は最短軸の表裏を使う。
    static glm::vec3 CalculateFallbackUpVec(const Planet* currentPlanet, const glm::vec3& actorPos);
};
