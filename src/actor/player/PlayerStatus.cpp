#include "actor/player/PlayerStatus.h"

#include "Game.h"

#include <algorithm>

void PlayerStatus::ConfigureHp(float hp)
{
    mHp = hp;
    mMaxHp = hp;
}

void PlayerStatus::SetMaxHp(float maxHp)
{
    mMaxHp = maxHp;
    if (mHp > mMaxHp) {
        mHp = mMaxHp;
    }
}

void PlayerStatus::TakeDamage(float damage)
{
    mHp = std::max(0.0f, mHp - damage);
    StartDamageCooldown();
    StartInvincible();
}

void PlayerStatus::TakeFallDamage(float damage)
{
    TakeDamage(damage);
}

void PlayerStatus::Heal(float amount)
{
    mHp = std::min(mMaxHp, mHp + amount);
}

void PlayerStatus::RestoreFullHp()
{
    mHp = mMaxHp;
}

void PlayerStatus::StartDamageCooldown()
{
    mDamageTimer = mDefaultDamageTimer;
}

void PlayerStatus::StartDamageCooldown(float seconds)
{
    mDamageTimer = seconds;
}

void PlayerStatus::ReduceDamageCooldown(float deltaTime)
{
    if (mDamageTimer > 0.0f) {
        mDamageTimer -= deltaTime;
    }
}

void PlayerStatus::StartInvincible()
{
    mInvincibleTimer = mDefaultInvincibleTimer;
}

void PlayerStatus::StartInvincible(float seconds)
{
    mInvincibleTimer = seconds;
}

void PlayerStatus::ClearInvincible()
{
    mInvincibleTimer = -1.0f;
}

void PlayerStatus::StartTired()
{
    mIsTired = true;
}

void PlayerStatus::EndTired()
{
    mIsTired = false;
}

void PlayerStatus::UpdateDamageTimer(float deltaTime)
{
    ReduceDamageCooldown(deltaTime);
}

void PlayerStatus::UpdateInvincibleTimer(float deltaTime)
{
    if (mInvincibleTimer > 0.0f) {
        mInvincibleTimer -= deltaTime;
    }
}

void PlayerStatus::UpdateTimers(float deltaTime)
{
    UpdateDamageTimer(deltaTime);
    UpdateInvincibleTimer(deltaTime);
}

void PlayerStatus::Die(Game& game) const
{
    game.OnPlayerDied();
}
