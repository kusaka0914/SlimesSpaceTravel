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
    if (mIsObtained || !mOwner || !mOwner->GetIsActive()) {
        return;
    }

    if (IsCollectablePlayerInPickUpRadius()) {
        mIsObtained = true;
    }
}

bool CollectableComponent::IsCollectablePlayerInPickUpRadius() const
{
    const Player* nearestPlayer = mOwner->GetGame()->FindNearestPlayer(mOwner);

    if (!nearestPlayer) {
        return false;
    }

    if (nearestPlayer->IsAttacking()) {
        return false;
    }

    const float distTo = glm::length(nearestPlayer->GetPos() - mOwner->GetPos());
    constexpr float pickupRadius = 0.8f;
    if (distTo <= pickupRadius) {
        return true;
    }

    return false;
}
