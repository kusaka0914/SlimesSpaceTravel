#pragma once

#include "component/Component.h"

class Actor;
class Player;

// Playerの攻撃状態・距離・向きから耐久値を減らし、破壊成立を記録する。Actorの非アクティブ化は行わない。
class DestructibleComponent : public Component {
public:
    DestructibleComponent(Actor* owner, int updateOrder = 100);
    void Update(float deltaTime) override;

    // 残り耐久値だけを設定する。破壊済み状態と直前の命中履歴はリセットしない。
    void SetDestroyHp(int destroyHp) { mDestroyHp = destroyHp; }

    bool GetIsDestroyed() const { return mIsDestroyed; }

private:
    void TryApplyDamage();
    void ApplyDamage(const float attack);

    bool IsPlayerInHitRange(Player* player) const;

private:
    bool mIsDestroyed;
    bool mIsHitCurrentAttack;

    float mDestroyHp;
};
