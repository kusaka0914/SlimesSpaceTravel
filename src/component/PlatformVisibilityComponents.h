#pragma once

#include "component/Component.h"

#include <cstdint>

class Platform;

// Playerが乗るとフェードを開始し、消えた後は衝突を止め、待ち時間後に再出現する。途中で降りてもフェードは続く。
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

// Player全員のジャンプ通算回数の変化で、床の表示と衝突を反転する。1回の更新では反転は最大1回。
class PlatformJumpToggleComponent : public Component {
public:
    explicit PlatformJumpToggleComponent(Platform* owner, int updateOrder = 71);
    void Update(float deltaTime) override;

    // 初期値と現在の表示・衝突を同時に切り替える。
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

// 一定間隔で床の表示と衝突を反転する。表示中は切替前に点滅で予告し、予告中も衝突は維持する。
class PlatformIntervalToggleComponent : public Component {
public:
    explicit PlatformIntervalToggleComponent(Platform* owner, int updateOrder = 72);
    void Update(float deltaTime) override;

    // 初期値と現在の表示・衝突を切り替え、周期タイマーも0へ戻す。
    void SetInitiallyVisible(bool visible);

    // 周期を0.1秒以上に制限し、現在の経過時間を新しい周期内へ折り返す。
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
