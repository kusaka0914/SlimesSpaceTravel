#pragma once

#include "actor/Actor.h"

#include <cstdint>
#include <unordered_map>

class Player;

class HazardActor final : public Actor {
public:
    explicit HazardActor(Game* game);

    void UpdateActor(float deltaTime) override;
    bool TryReactToAttack(Player& player);

    void SetDamage(float damage);
    void SetTriggerRadius(float triggerRadius);
    void SetDamageIntervalSeconds(float damageIntervalSeconds);

    float GetDamage() const { return mDamage; }
    float GetTriggerRadius() const { return mTriggerRadius; }
    float GetDamageIntervalSeconds() const
    {
        return mDamageIntervalSeconds;
    }

private:
    bool IsPlayerOnSamePlanetSurface(const Player& player) const;
    bool IsPlayerTouching(const Player& player) const;
    bool IsWithinPlayerAttack(const Player& player) const;

    float mDamage = 1.0f;
    float mTriggerRadius = 0.75f;
    float mDamageIntervalSeconds = 1.0f;
    std::unordered_map<const Player*, std::uint64_t>
        mHandledAttackSequences;
    std::unordered_map<const Player*, float>
        mDamageCooldownSecondsByPlayer;
};
