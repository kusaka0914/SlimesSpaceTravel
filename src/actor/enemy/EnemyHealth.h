#pragma once

// 現在HPと最大HPを保持する。値の範囲制限や死亡演出への遷移は担当しない。
class EnemyHealth {
public:
    void SetHp(float hp) { mHp = hp; }
    void SetMaxHp(float maxHp) { mMaxHp = maxHp; }
    void AddDamage(float damage) { mHp -= damage; }
    void SetHpZero() { mHp = 0.0f; }

    float GetHp() const { return mHp; }
    float GetMaxHp() const { return mMaxHp; }
    bool IsDead() const { return mHp <= 0.0f; }

private:
    float mHp = 10.0f;
    float mMaxHp = 10.0f;
};
