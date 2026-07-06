#pragma once

class Enemy;
struct PlayerModuleContext;

class PlayerStatus {
public:
    float hp = 100.0f;
    float maxHp = 100.0f;
    float damageTimer = 0.0f;
    float defaultDamageTimer = 1.0f;
    float invincibleTimer = -1.0f;
    float defaultInvincibleTimer = 2.0f;
    bool isTired = false;

    bool IsAlive() const { return hp > 0.0f; }
    bool IsInvincible() const { return invincibleTimer > 0.0f; }

    void ApplyDamage(PlayerModuleContext& context, Enemy* enemy, float deltaTime);
    void Recover(PlayerModuleContext& context);
    void Die(PlayerModuleContext& context);
};
