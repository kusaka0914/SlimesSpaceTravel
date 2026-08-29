#pragma once

#include "component/Component.h"

#include <cstdint>

class Platform;

class PlatformFadeOnStandComponent : public Component {
public:
    explicit PlatformFadeOnStandComponent(Platform* owner, int updateOrder = 70);
    void Update(float deltaTime) override;

    void SetFadeOutDuration(float duration);
    void SetReappearDelay(float delay);
    float GetFadeOutDuration() const { return mFadeOutDuration; }
    float GetReappearDelay() const { return mReappearDelay; }

private:
    enum class FadePhase {
        Visible,
        FadingOut,
        WaitingToReappear,
    };

    Platform* mPlatform = nullptr;
    float mOpacity = 1.0f;
    float mFadeOutDuration = 1.0f;
    float mReappearDelay = 2.0f;
    float mHiddenTimer = 0.0f;
    bool mCollisionEnabled = true;
    FadePhase mFadePhase = FadePhase::Visible;
};

class PlatformJumpToggleComponent : public Component {
public:
    explicit PlatformJumpToggleComponent(Platform* owner, int updateOrder = 71);
    void Update(float deltaTime) override;

    void SetInitiallyVisible(bool visible);
    bool GetInitiallyVisible() const { return mInitiallyVisible; }
    bool GetVisible() const { return mVisible; }

private:
    void ApplyState();

    Platform* mPlatform = nullptr;
    bool mInitiallyVisible = true;
    bool mVisible = true;
    std::uint64_t mLastObservedJumpCount = 0;
};

class PlatformIntervalToggleComponent : public Component {
public:
    explicit PlatformIntervalToggleComponent(Platform* owner, int updateOrder = 72);
    void Update(float deltaTime) override;

    void SetInitiallyVisible(bool visible);
    void SetInterval(float interval);
    void SetWarningDuration(float duration);
    void SetBlinkInterval(float interval);

    bool GetInitiallyVisible() const { return mInitiallyVisible; }
    float GetInterval() const { return mInterval; }
    float GetWarningDuration() const { return mWarningDuration; }
    float GetBlinkInterval() const { return mBlinkInterval; }

private:
    void ApplyState(float opacity);

    Platform* mPlatform = nullptr;
    bool mInitiallyVisible = true;
    bool mVisible = true;
    float mTimer = 0.0f;
    float mInterval = 3.0f;
    float mWarningDuration = 1.0f;
    float mBlinkInterval = 0.15f;
};
