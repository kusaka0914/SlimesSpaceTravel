#pragma once

class Game;

class PlayerStatus {
public:
    bool IsAlive() const { return mHp > 0.0f; }
    bool IsDead() const { return !IsAlive(); }
    bool IsInvincible() const { return mInvincibleTimer > 0.0f; }
    bool ShouldBlinkWhileInvincible() const
    {
        return IsInvincible() && mShouldBlinkWhileInvincible;
    }
    bool IsTired() const { return mIsTired; }

    void ConfigureHp(float hp);
    void SetHp(float hp) { mHp = hp; }
    void SetMaxHp(float maxHp);
    void SetDefaultDamageTimer(float defaultDamageTimer) { mDefaultDamageTimer = defaultDamageTimer; }
    void SetDefaultInvincibleTimer(float defaultInvincibleTimer) { mDefaultInvincibleTimer = defaultInvincibleTimer; }

    float GetHp() const { return mHp; }
    float GetMaxHp() const { return mMaxHp; }
    float GetDamageTimer() const { return mDamageTimer; }
    float GetDefaultDamageTimer() const { return mDefaultDamageTimer; }
    float GetInvincibleTimer() const { return mInvincibleTimer; }
    float GetDefaultInvincibleTimer() const { return mDefaultInvincibleTimer; }
    bool GetIsTired() const { return mIsTired; }

    void TakeDamage(float damage);
    void TakeFallDamage(float damage);
    void Heal(float amount);
    void RestoreFullHp();

    void StartDamageCooldown();
    void StartDamageCooldown(float seconds);
    void ReduceDamageCooldown(float deltaTime);

    void StartDamageInvincibility();
    void StartDodgeInvincibility(float seconds);
    void ClearInvincible();

    void StartTired();
    void EndTired();

    void UpdateDamageTimer(float deltaTime);
    void UpdateInvincibleTimer(float deltaTime);
    void UpdateTimers(float deltaTime);
    void Die(Game& game) const;

private:
    float mHp = 100.0f;
    float mMaxHp = 100.0f;
    float mDamageTimer = 0.0f;
    float mDefaultDamageTimer = 1.0f;
    float mInvincibleTimer = -1.0f;
    float mDefaultInvincibleTimer = 2.0f;
    bool mShouldBlinkWhileInvincible = false;
    bool mIsTired = false;
};
