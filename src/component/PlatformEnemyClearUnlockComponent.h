#pragma once

#include "component/Component.h"

class Platform;

class PlatformEnemyClearUnlockComponent : public Component {
public:
    explicit PlatformEnemyClearUnlockComponent(
        Platform* owner,
        int updateOrder = 84);
    ~PlatformEnemyClearUnlockComponent() override;

    void Update(float deltaTime) override;

    bool GetIsUnlocked() const { return mIsUnlocked; }

private:
    bool HasLivingEnemyOnCurrentPlanet() const;
    void ApplyLockedState();
    void ClearLockedState();

    Platform* mPlatform = nullptr;
    bool mIsUnlocked = false;
};
