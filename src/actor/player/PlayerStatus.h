#pragma once

class Game;

// HP・被ダメージ間隔・無敵・疲労を保持する。被弾の可否判定と行動遷移は呼び出し側が担当する。
class PlayerStatus {
public:
    bool IsAlive() const { return mHp > 0.0f; }
    bool IsDead() const { return !IsAlive(); }
    bool IsInvincible() const { return mInvincibleTimer > 0.0f; }
    bool ShouldBlinkWhileInvincible() const { return IsInvincible() && mShouldBlinkWhileInvincible; }
    bool IsTired() const { return mIsTired; }

    // 現在HPと最大HPを同じ値で初期化する。
    void ConfigureHp(float hp);
    void SetHp(float hp) { mHp = hp; }

    // 最大HPを下げた場合、現在HPもその値以下に制限する。
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

    // HPを減らして0以上に制限し、被ダメージ間隔と点滅付き無敵を開始する。無敵の可否判定は呼出元で行う。
    void TakeDamage(float damage);
    void TakeFallDamage(float damage);
    void Heal(float amount);
    void RestoreFullHp();

    void StartDamageCooldown();
    void StartDamageCooldown(float seconds);
    void ReduceDamageCooldown(float deltaTime);

    void StartDamageInvincibility();

    // seconds秒の無敵へ置き換える。被ダメージ由来の無敵と異なり、点滅は行わない。
    void StartDodgeInvincibility(float seconds);
    void ClearInvincible();

    void StartTired();
    void EndTired();

    void UpdateDamageTimer(float deltaTime);
    void UpdateInvincibleTimer(float deltaTime);
    void UpdateTimers(float deltaTime);

    // Gameへ死亡を通知する。HPや行動状態は変更しない。
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
