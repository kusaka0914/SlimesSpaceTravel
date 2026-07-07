#pragma once

class EnemyBreakGauge {
public:
    void SetCount(int count) { mCount = count; }
    void SetMax(int max) { mMax = max; }
    void Reset() { mCount = mMax; }
    void Decrease() { --mCount; }
    void BreakAll() { mCount = 0; }

    int GetCount() const { return mCount; }
    int GetMax() const { return mMax; }
    bool IsEmpty() const { return mCount <= 0; }

private:
    int mCount = 0;
    int mMax = 0;
};
