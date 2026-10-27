#include "system/camera/zones/CameraZoneDirectionController.h"

#include "actor/Player.h"
#include "system/camera/zones/CameraZoneSettings.h"
#include <algorithm>

namespace {
int ClassifyDirection(glm::vec3 movement, glm::vec3 reference, glm::vec3 up)
{
    if (glm::dot(up, up) < 0.000001f) {
        return 0;
    }
    up = glm::normalize(up);
    movement -= up * glm::dot(movement, up);
    reference -= up * glm::dot(reference, up);
    if (glm::dot(movement, movement) < 0.000001f || glm::dot(reference, reference) < 0.000001f) {
        return 0;
    }
    constexpr float directionThreshold = 0.35f;
    const float alignment = glm::dot(glm::normalize(movement), glm::normalize(reference));
    return alignment > directionThreshold ? 1 : alignment < -directionThreshold ? -1 : 0;
}
} // namespace

void CameraZoneDirectionController::ObserveMovement(const glm::vec3& direction)
{
    if (glm::dot(direction, direction) > 0.000001f) {
        mLastMovementDirection = direction;
    }
}

void CameraZoneDirectionController::Update(const CameraZoneSettings& settings, const Player& player,
                                           const glm::vec3& zoneCenter, bool entering, bool suspended,
                                           bool resetRequested, float deltaTime)
{
    if (!settings.bidirectional) {
        mIsReturnCamera = false;
        mPendingDirection = 0;
        mPendingSeconds = 0.0f;
        return;
    }
    const auto movement = player.GetInputMovementDirection();
    const auto up = player.GetUpVec();
    int direction = ClassifyDirection(movement, settings.travelDirection, up);
    if (entering || resetRequested) {
        if (direction == 0) {
            direction = ClassifyDirection(mLastMovementDirection, settings.travelDirection, up);
        }
        // 停止して進入した場合は、領域中心の手前側から奥へ進む向きを補助情報にする。
        if (direction == 0 && entering) {
            direction = ClassifyDirection(zoneCenter - player.GetPos(), settings.travelDirection, up);
        }
        if (direction != 0) {
            mIsReturnCamera = direction < 0;
        } else if (entering) {
            mIsReturnCamera = settings.defaultReturnCamera;
        }
        mPendingDirection = 0;
        mPendingSeconds = 0.0f;
        return;
    }
    if (suspended || (!player.GetOnGround() && !settings.allowAirborneDirectionSwitch) || direction == 0 ||
        (direction < 0) == mIsReturnCamera) {
        mPendingDirection = 0;
        mPendingSeconds = 0.0f;
        return;
    }
    if (mPendingDirection != direction) {
        mPendingDirection = direction;
        mPendingSeconds = 0.0f;
    }
    mPendingSeconds += std::max(0.0f, deltaTime);
    if (mPendingSeconds >= settings.directionSwitchDelay) {
        mIsReturnCamera = direction < 0;
        mPendingDirection = 0;
        mPendingSeconds = 0.0f;
    }
}

void CameraZoneDirectionController::CancelPendingSwitch()
{
    mPendingDirection = 0;
    mPendingSeconds = 0.0f;
}
