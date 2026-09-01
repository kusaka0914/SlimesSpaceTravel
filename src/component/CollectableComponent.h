#pragma once

#include "Component.h"

class Actor;
class Player;

class CollectableComponent : public Component {
public:
    CollectableComponent(Actor* owner, int updateOrder = 100);

    void Update(float deltaTime) override;
    bool GetIsObtained() const { return mIsObtained; }
    Player* GetObtainingPlayer() const { return mObtainingPlayer; }
    void SetPickupRadius(float pickupRadius)
    {
        mPickupRadius = pickupRadius;
    }
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
