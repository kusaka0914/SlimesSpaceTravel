#pragma once

struct EnemyGuardDamageResult {
    int brokenSegmentCount = 0;
    bool wasFullyBroken = false;

    bool DidBreakSegment() const { return brokenSegmentCount > 0; }
};

class EnemyGuardGauge {
public:
    void Configure(int segmentCount, float guardValuePerSegment);
    void SetSegmentCount(int segmentCount);
    EnemyGuardDamageResult ApplyDamage(float guardDamage);
    EnemyGuardDamageResult ApplyDamageEnsuringCurrentSegmentBreak(
        float minimumGuardDamage);
    EnemyGuardDamageResult BreakAll();
    void Reset();

    float GetCurrentGuard() const { return mCurrentGuard; }
    float GetMaxGuard() const;
    float GetGuardValuePerSegment() const
    {
        return mGuardValuePerSegment;
    }
    int GetSegmentCount() const { return mSegmentCount; }
    bool IsEmpty() const { return mCurrentGuard <= 0.0f; }

private:
    int CalculateFilledSegmentCount(float guard) const;

    float mCurrentGuard = 0.0f;
    float mGuardValuePerSegment = 20.0f;
    int mSegmentCount = 0;
};
