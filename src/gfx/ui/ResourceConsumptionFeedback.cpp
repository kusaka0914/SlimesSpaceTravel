#include "gfx/ui/ResourceConsumptionFeedback.h"

#include <algorithm>

void ResourceConsumptionFeedback::Update(float value, int maximumCount, float deltaTime, bool allowsFeedback)
{
    maximumCount = std::max(0, maximumCount);
    value = std::clamp(value, 0.0f, static_cast<float>(maximumCount));
    mRemainingSeconds = std::max(0.0f, mRemainingSeconds - std::max(0.0f, deltaTime));
    if (!allowsFeedback || !mPreviousValue || mPreviousMaximumCount != maximumCount) {
        mRemainingSeconds = 0.0f;
    } else if (value < *mPreviousValue) {
        const int firstSlot = static_cast<int>(value);
        const int endSlot = static_cast<int>(*mPreviousValue);
        // 短時間に連続消費しても、先に減ったアイコンの強調を落とさない。
        mFirstSlot = mRemainingSeconds > 0.0f ? std::min(mFirstSlot, firstSlot) : firstSlot;
        mEndSlot = mRemainingSeconds > 0.0f ? std::max(mEndSlot, endSlot) : endSlot;
        mRemainingSeconds = durationSeconds;
    }
    mPreviousValue = value;
    mPreviousMaximumCount = maximumCount;
}

void ResourceConsumptionFeedback::Reset()
{
    mPreviousValue.reset();
    mPreviousMaximumCount = 0;
    mRemainingSeconds = 0.0f;
    mFirstSlot = mEndSlot = 0;
}

float ResourceConsumptionFeedback::GetStrength() const
{
    return std::clamp(mRemainingSeconds / durationSeconds, 0.0f, 1.0f);
}
