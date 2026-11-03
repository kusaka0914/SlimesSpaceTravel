#pragma once

#include <optional>

// HUDの残量減少を検出し、消費されたスロットだけを短時間強調する。描画資源・Actorは保持しない。
class ResourceConsumptionFeedback {
public:
    static constexpr float durationSeconds = 0.2f;

    // 初回・最大数変更時は演出しない。非表示中も残量履歴を更新し、復帰後の誤発光を防ぐ。
    void Update(float value, int maximumCount, float deltaTime, bool allowsFeedback);
    void Reset();
    float GetStrength() const;
    int GetFirstSlot() const { return mFirstSlot; }
    int GetEndSlot() const { return mEndSlot; }

private:
    std::optional<float> mPreviousValue;
    int mPreviousMaximumCount = 0;
    float mRemainingSeconds = 0.0f;
    int mFirstSlot = 0;
    int mEndSlot = 0;
};
