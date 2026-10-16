#include "component/PlatformEnemyClearUnlockComponent.h"

#include "Game.h"
#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "actor/Platform.h"

namespace {

bool IsEditorPreview(const Platform* platform)
{
    return platform && platform->GetGame() && platform->GetGame()->GetIsDebugEditorShowing();
}

} // namespace

PlatformEnemyClearUnlockComponent::PlatformEnemyClearUnlockComponent(Platform* owner, int updateOrder)
    : Component(owner, updateOrder),
      mPlatform(owner)
{
}

PlatformEnemyClearUnlockComponent::~PlatformEnemyClearUnlockComponent() = default;

void PlatformEnemyClearUnlockComponent::Update(float deltaTime)
{
    (void)deltaTime;
    if (!mPlatform) {
        return;
    }

    if (IsEditorPreview(mPlatform)) {
        ClearLockedState();
        return;
    }

    if (mIsUnlocked) {
        return;
    }

    if (HasLivingEnemyOnCurrentPlanet()) {
        ApplyLockedState();
        return;
    }

    mIsUnlocked = true;
    ClearLockedState();
}

bool PlatformEnemyClearUnlockComponent::HasLivingEnemyOnCurrentPlanet() const
{
    const Planet* currentPlanet = mPlatform ? mPlatform->GetCurrentPlanet() : nullptr;
    if (!currentPlanet) {
        return false;
    }

    for (const Enemy* enemy : currentPlanet->GetEnemies()) {
        if (enemy && enemy->GetIsActive() && !enemy->GetIsDead()) {
            return true;
        }
    }
    return false;
}

void PlatformEnemyClearUnlockComponent::ApplyLockedState()
{
    if (!mPlatform) {
        return;
    }
    constexpr float lockedSwitchOpacity = 0.2f;
    mPlatform->SetComponentOpacity(this, lockedSwitchOpacity);
    mPlatform->SetComponentCollisionEnabled(this, false);
}

void PlatformEnemyClearUnlockComponent::ClearLockedState()
{
    if (mPlatform) {
        mPlatform->ClearComponentRuntimeState(this);
    }
}
