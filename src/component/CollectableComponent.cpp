#include "CollectableComponent.h"
#include "Game.h"
#include "actor/Player.h"

CollectableComponent::CollectableComponent(Actor* owner, int updateOrder)
    : Component(owner, updateOrder),
      mIsObtained(false)
{
}

void CollectableComponent::Update(float deltaTime)
{
    if (mIsObtained || !mOwner || !mOwner->GetIsActive() ||
        (mOwner->GetGame() &&
         mOwner->GetGame()->GetIsDebugEditorShowing())) {
        return;
    }

    mObtainingPlayer = FindCollectablePlayerInPickUpRadius();
    if (mObtainingPlayer) {
        mIsObtained = true;
    }
}

Player* CollectableComponent::FindCollectablePlayerInPickUpRadius() const
{
    Player* nearestPlayer =
        mOwner->GetGame()->FindNearestPlayer(mOwner);

    if (!nearestPlayer) {
        return nullptr;
    }

    if (nearestPlayer->IsAttacking()) {
        return nullptr;
    }

    const float distTo = glm::length(nearestPlayer->GetPos() - mOwner->GetPos());
    constexpr float pickupRadius = 0.8f;
    if (distTo <= pickupRadius) {
        return nearestPlayer;
    }

    return nullptr;
}
