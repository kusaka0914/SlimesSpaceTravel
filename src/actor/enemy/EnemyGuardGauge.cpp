#include "actor/enemy/EnemyGuardGauge.h"

#include <algorithm>
#include <cmath>

void EnemyGuardGauge::Configure(
    int segmentCount,
    float guardValuePerSegment)
{
    mSegmentCount = std::max(0, segmentCount);
    mGuardValuePerSegment = std::max(0.0f, guardValuePerSegment);
    Reset();
}

void EnemyGuardGauge::SetSegmentCount(int segmentCount)
{
    mSegmentCount = std::max(0, segmentCount);
    mCurrentGuard = std::clamp(
        mCurrentGuard,
        0.0f,
        GetMaxGuard());
}

EnemyGuardDamageResult EnemyGuardGauge::ApplyDamage(float guardDamage)
{
    EnemyGuardDamageResult damageResult;
    if (guardDamage <= 0.0f || IsEmpty()) {
        return damageResult;
    }

    const float previousGuard = mCurrentGuard;
    const int previousFilledSegmentCount =
        CalculateFilledSegmentCount(previousGuard);
    mCurrentGuard = std::clamp(
        previousGuard - guardDamage,
        0.0f,
        GetMaxGuard());
    const int currentFilledSegmentCount =
        CalculateFilledSegmentCount(mCurrentGuard);

    damageResult.brokenSegmentCount = std::max(
        0,
        previousFilledSegmentCount - currentFilledSegmentCount);
    damageResult.wasFullyBroken =
        previousGuard > 0.0f && IsEmpty();
    return damageResult;
}

EnemyGuardDamageResult
EnemyGuardGauge::ApplyDamageEnsuringCurrentSegmentBreak(
    float minimumGuardDamage)
{
    if (IsEmpty() || mGuardValuePerSegment <= 0.0f) {
        return {};
    }

    float guardRemainingInCurrentSegment =
        std::fmod(mCurrentGuard, mGuardValuePerSegment);
    constexpr float segmentBoundaryEpsilon = 0.0001f;
    if (guardRemainingInCurrentSegment <= segmentBoundaryEpsilon) {
        guardRemainingInCurrentSegment = mGuardValuePerSegment;
    }

    return ApplyDamage(std::max(
        minimumGuardDamage,
        guardRemainingInCurrentSegment));
}

EnemyGuardDamageResult EnemyGuardGauge::BreakAll()
{
    return ApplyDamage(mCurrentGuard);
}

void EnemyGuardGauge::Reset()
{
    mCurrentGuard = GetMaxGuard();
}

float EnemyGuardGauge::GetMaxGuard() const
{
    return static_cast<float>(mSegmentCount) *
        mGuardValuePerSegment;
}

int EnemyGuardGauge::CalculateFilledSegmentCount(float guard) const
{
    if (guard <= 0.0f || mGuardValuePerSegment <= 0.0f) {
        return 0;
    }
    return std::min(
        mSegmentCount,
        static_cast<int>(std::ceil(guard / mGuardValuePerSegment)));
}
