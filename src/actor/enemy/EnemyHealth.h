#pragma once

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
