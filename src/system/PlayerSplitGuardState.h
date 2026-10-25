#pragma once

// 分裂中のガード回数と、消費後の時間経過による回復を管理する。
class PlayerSplitGuardState {
public:
    static constexpr int MaximumGuardCount = 3;
    static constexpr float RecoveryIntervalSeconds = 10.0f;

    void Reset();

    // 不足分を一定間隔で1回ずつ回復する。負の経過時間は0として扱う。
    void Update(float deltaTimeSeconds);

    // 回復待ち時間をリセットし、残数があれば1回分を消費してtrueを返す。
    bool ConsumeOne();

    int GetCount() const { return mGuardCount; }
    float GetRecoveryRemainingSeconds() const { return mRecoveryRemainingSeconds; }

private:
    int mGuardCount = MaximumGuardCount;
    float mRecoveryRemainingSeconds = RecoveryIntervalSeconds;
};
