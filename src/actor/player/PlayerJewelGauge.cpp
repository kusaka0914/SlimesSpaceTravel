#include "actor/player/PlayerJewelGauge.h"

#include <algorithm>

bool PlayerJewelGauge::CanConsume(int amount) const
{
    return amount > 0 && mCount >= amount;
}

bool PlayerJewelGauge::Consume(int amount)
{
    if (!CanConsume(amount)) {
        return false;
    }

    mCount -= amount;
    return true;
}

void PlayerJewelGauge::Add(int value)
{
    mCount = std::clamp(mCount + value, 0, mMaxCount);
}

void PlayerJewelGauge::RestoreFull()
{
    mCount = mMaxCount;
    mRecoverTimer = -1.0f;
}

bool PlayerJewelGauge::ShouldStartRecoverTimer() const
{
    return mCount < mMaxCount && mRecoverTimer <= 0.0f;
}

void PlayerJewelGauge::StartRecoverTimer(float seconds)
{
    mRecoverTimer = seconds;
}

void PlayerJewelGauge::UpdateRecoverTimer(float deltaTime)
{
    mRecoverTimer -= deltaTime;
    if (mRecoverTimer >= 0.0f) {
        return;
    }

    if (mCount < mMaxCount) {
        ++mCount;
    }
}

void PlayerJewelGauge::SetCount(int count)
{
    mCount = std::clamp(count, 0, mMaxCount);
}
