#pragma once

#include "actor/Actor.h"

#include <cstdint>
#include <unordered_map>

class Player;

// 同じ惑星面のプレイヤーに接触ダメージを与える障害物。攻撃で触れた場合も反応する。
class HazardActor final : public Actor {
public:
    explicit HazardActor(Game* game);

    void UpdateActor(float deltaTime) override;

    // 同じ攻撃シーケンスには1回だけ反応する。trueは反応成立であり、ダメージ発生の保証ではない。
    bool TryReactToAttack(Player& player);

    void SetDamage(float damage);

    // モデルの各軸スケールに掛ける基準半径。実際の判定は姿勢を反映した楕円体で行う。
    void SetTriggerRadius(float triggerRadius);

    // 同じプレイヤーへの接触ダメージ間隔（秒）。負値は0に制限する。
    void SetDamageIntervalSeconds(float damageIntervalSeconds);

    float GetDamage() const { return mDamage; }
    float GetTriggerRadius() const { return mTriggerRadius; }

    // Actorのローカル軸に沿う、スケール適用後の半径を返す。回転はまだ含まない。
    glm::vec3 CalculateScaledTriggerHalfExtents() const;
    float GetDamageIntervalSeconds() const { return mDamageIntervalSeconds; }

private:
    bool IsPlayerOnSamePlanetSurface(const Player& player) const;
    bool IsPlayerTouching(const Player& player) const;
    bool IsWithinPlayerAttack(const Player& player) const;
    float CalculateTriggerRadiusAlongWorldDirection(const glm::vec3& worldDirection) const;

    float mDamage = 1.0f;
    float mTriggerRadius = 1.0f;
    float mDamageIntervalSeconds = 1.0f;

    // キーは非所有のPlayer参照。攻撃の重複反応と接触ダメージの間隔を別々に管理する。
    std::unordered_map<const Player*, std::uint64_t> mHandledAttackSequences;
    std::unordered_map<const Player*, float> mDamageCooldownSecondsByPlayer;
};
