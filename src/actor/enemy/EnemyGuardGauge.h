#pragma once

// 今回の処理で破壊した区画数と、正のガードが0になったかを表す。既に空なら全破壊扱いにしない。
struct EnemyGuardDamageResult {
    int brokenSegmentCount = 0;
    bool wasFullyBroken = false;

    bool DidBreakSegment() const { return brokenSegmentCount > 0; }
};

// 区画ごとの耐久値からガード総量を管理し、1回のダメージで破壊した区画数を求める。
class EnemyGuardGauge {
public:
    // 区画数と区画耐久値を0以上に制限して設定し、現在ガードも最大値へ回復する。
    void Configure(int segmentCount, float guardValuePerSegment);

    // 区画数を0以上に制限し、現在ガードが新しい最大値を超えれば切り詰める。増設分の回復は行わない。
    void SetSegmentCount(int segmentCount);

    // 正のダメージだけを適用し、区画破壊の結果を返す。非正の入力・ガードなしでは変更しない。
    EnemyGuardDamageResult ApplyDamage(float guardDamage);

    // minimumGuardDamage以上、かつ現在区画を破壊できるダメージを適用する。ガードなしでは空の結果。
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
