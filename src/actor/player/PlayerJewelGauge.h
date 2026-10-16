#pragma once

// ジュエルの消費と回復を管理する。自動回復の上限は2、アイテム取得時の上限は6。
class PlayerJewelGauge {
public:
    static constexpr int ContinuousAttackCost = 2;

    bool CanConsume(int amount) const;

    // 正のamountを支払える場合だけ消費してtrue。失敗時は個数を変えない。
    bool Consume(int amount);

    // 自動回復用。正の値だけ加え、2個を超えて増やさない。
    void Add(int value);

    // アイテム用。加算後の個数を0〜6に制限する。
    void AddFromItem(int value);

    // 少なくとも2個へ回復し、既に2個を超える場合は維持する。回復タイマーも停止する。
    void RestoreFull();

    bool ShouldStartRecoverTimer() const;
    void StartRecoverTimer(float seconds = 30.0f);

    // 開始済みのタイマーに対して呼ぶ。満了時に2個未満なら1個回復し、満了後は呼び出し側で停止・再開始する。
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
