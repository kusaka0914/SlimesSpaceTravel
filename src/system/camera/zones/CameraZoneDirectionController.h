#pragma once

#include <glm/glm.hpp>

class Player;
struct CameraZoneSettings;

// 接平面上の入力移動から、Playerごとの行き・帰りと反転待ち時間を管理する。
class CameraZoneDirectionController {
public:
    void ObserveMovement(const glm::vec3& direction);
    void Update(const CameraZoneSettings& settings, const Player& player, const glm::vec3& zoneCenter, bool entering,
                bool suspended, bool resetRequested, float deltaTime);
    bool IsReturnCamera() const { return mIsReturnCamera; }

private:
    glm::vec3 mLastMovementDirection{0.0f};
    bool mIsReturnCamera = false;
    int mPendingDirection = 0;
    float mPendingSeconds = 0.0f;
};
