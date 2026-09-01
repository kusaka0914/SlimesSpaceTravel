#pragma once

class PlayerJewelGauge {
public:
    static constexpr int ContinuousAttackCost = 2;

    bool CanConsume(int amount) const;
    bool Consume(int amount);
    void Add(int value);
    void AddFromItem(int value);
    void RestoreFull();

    bool ShouldStartRecoverTimer() const;
    void StartRecoverTimer(float seconds = 30.0f);
    void UpdateRecoverTimer(float deltaTime);

    void SetCount(int count);
    int GetCount() const { return mCount; }
    int GetMaxCount() const { return itemPickupMaxCount; }
    float GetRecoverTimer() const { return mRecoverTimer; }

private:
    int mCount = 2;
    static constexpr int automaticRecoveryMaxCount = 2;
    static constexpr int itemPickupMaxCount = 6;
    float mRecoverTimer = -1.0f;
};
