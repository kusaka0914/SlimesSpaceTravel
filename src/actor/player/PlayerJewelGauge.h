#pragma once

class PlayerJewelGauge {
public:
    bool CanConsume(int amount) const;
    bool Consume(int amount);
    void Add(int value);
    void RestoreFull();

    bool ShouldStartRecoverTimer() const;
    void StartRecoverTimer(float seconds = 30.0f);
    void UpdateRecoverTimer(float deltaTime);

    void SetCount(int count);
    int GetCount() const { return mCount; }
    int GetMaxCount() const { return mMaxCount; }
    float GetRecoverTimer() const { return mRecoverTimer; }

private:
    int mCount = 2;
    int mMaxCount = 2;
    float mRecoverTimer = -1.0f;
};
