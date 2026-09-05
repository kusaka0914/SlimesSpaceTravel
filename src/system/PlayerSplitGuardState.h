#pragma once

class PlayerSplitGuardState {
public:
    static constexpr int MaximumGuardCount = 3;
    static constexpr float RecoveryIntervalSeconds = 10.0f;

    void Reset();
    void Update(float deltaTimeSeconds);
    bool ConsumeOne();

    int GetCount() const { return mGuardCount; }
    float GetRecoveryRemainingSeconds() const
    {
        return mRecoveryRemainingSeconds;
    }

private:
    int mGuardCount = MaximumGuardCount;
    float mRecoveryRemainingSeconds = RecoveryIntervalSeconds;
};
