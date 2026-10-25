#pragma once

#include "component/Component.h"

#include <cstdint>
#include <unordered_map>

class Actor;
class Player;

// Playerの攻撃状態・距離・向きから耐久値を減らし、破壊成立を記録する。Actorの非アクティブ化は行わない。
// 解決済みの攻撃番号ごとに、各Playerからのダメージを1回だけ受け付ける。
class DestructibleComponent : public Component {
public:
    DestructibleComponent(Actor* owner, int updateOrder = 100);

    // 予約中は判定せず、攻撃終了と同時に解決した攻撃も受け付ける。所有Actor・Gameが未設定なら何もしない。
    void Update(float deltaTime) override;

    // 残り耐久値だけを設定する。破壊済み状態と直前の命中履歴はリセットしない。
    void SetDestroyHp(int destroyHp) { mDestroyHp = destroyHp; }

    bool GetIsDestroyed() const { return mIsDestroyed; }

private:
    struct PlayerAttackHitState {
        std::uint64_t attackSequence = 0;

        // 命中済み、または攻撃終了で判定を閉じた状態。
        bool isConsumed = false;
    };

    void TryApplyDamage();
    void ApplyDamage(const float attack);

    bool IsPlayerInHitRange(Player* player) const;

private:
    bool mIsDestroyed;

    // Playerは所有せず、キーのポインタを履歴から逆参照しない。
    std::unordered_map<const Player*, PlayerAttackHitState> mPlayerAttackHitStates;

    float mDestroyHp;
};
