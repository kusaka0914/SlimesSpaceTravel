#pragma once

#include "component/Component.h"

#include <glm/glm.hpp>

class Platform;

// Playerが乗った瞬間の床上の位置から進行方向を決め、乗っている間だけワールドXZ平面上を動く。
// 速度はワールド距離/秒。往復移動のComponentがあれば、その経路も同じ変位で移す。
class PlatformDirectionalMovementComponent : public Component {
public:
    explicit PlatformDirectionalMovementComponent(Platform* owner, int updateOrder = 55);
    void Update(float deltaTime) override;

    void SetSpeed(float speed);
    float GetSpeed() const { return mSpeed; }

private:
    Platform* mPlatform = nullptr;
    glm::vec3 mTravelDirection{0.0f};
    float mSpeed = 2.0f;
    bool mWasOccupied = false;
};

// 床を指定したローカル軸まわりに回転させる。角速度は度/秒で、編集画面では更新しない。
class PlatformRotationComponent : public Component {
public:
    explicit PlatformRotationComponent(Platform* owner, int updateOrder = 60);
    void Update(float deltaTime) override;

    // 軸を正規化する。長さがほぼ0ならローカルY軸へ戻す。
    void SetLocalAxis(const glm::vec3& axis);
    void SetDegreesPerSecond(float speed) { mDegreesPerSecond = speed; }
    const glm::vec3& GetLocalAxis() const { return mLocalAxis; }
    float GetDegreesPerSecond() const { return mDegreesPerSecond; }

private:
    Platform* mPlatform = nullptr;
    glm::vec3 mLocalAxis{0.0f, 1.0f, 0.0f};
    float mDegreesPerSecond = 45.0f;
};

// 床のローカル方向からコンベヤーのフレーム変位を求める。床自身は動かさず、乗員側が変位を適用する。
// 速度はワールド距離/秒。編集画面では更新しない。
class PlatformConveyorComponent : public Component {
public:
    explicit PlatformConveyorComponent(Platform* owner, int updateOrder = 80);
    void Update(float deltaTime) override;

    // 方向を正規化する。長さがほぼ0ならローカルZ正方向へ戻す。
    void SetLocalDirection(const glm::vec3& direction);
    void SetSpeed(float speed);
    const glm::vec3& GetLocalDirection() const { return mLocalDirection; }
    float GetSpeed() const { return mSpeed; }

private:
    Platform* mPlatform = nullptr;
    glm::vec3 mLocalDirection{0.0f, 0.0f, 1.0f};
    float mSpeed = 2.0f;
};
