#pragma once

#include "component/Component.h"

class Actor;
class Player;

// 近くのPlayerによる回収成立と取得者を記録する。回収後の非アクティブ化や進行通知はActor側が担当する。
class CollectableComponent : public Component {
public:
    CollectableComponent(Actor* owner, int updateOrder = 100);

    // 取得済み・所有Actorが無効・編集中なら判定しない。成立後の取得状態は維持する。
    void Update(float deltaTime) override;
    bool GetIsObtained() const { return mIsObtained; }

    // 未取得ならnullptr。取得者への非所有参照を返す。
    Player* GetObtainingPlayer() const { return mObtainingPlayer; }
    void SetPickupRadius(float pickupRadius) { mPickupRadius = pickupRadius; }
    void SetCanPickupWhileAttacking(bool canPickupWhileAttacking)
    {
        mCanPickupWhileAttacking = canPickupWhileAttacking;
    }

private:
    Player* FindCollectablePlayerInPickUpRadius() const;

private:
    bool mIsObtained;
    Player* mObtainingPlayer = nullptr;
    float mPickupRadius = 0.8f;
    bool mCanPickupWhileAttacking = false;
};
