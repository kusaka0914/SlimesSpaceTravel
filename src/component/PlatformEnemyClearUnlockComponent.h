#pragma once

#include "component/Component.h"

class Platform;

// 同じ惑星の有効な敵が死亡完了するまで、床を半透明・衝突無効にする。一度解除すると再ロックしない。
class PlatformEnemyClearUnlockComponent : public Component {
public:
    explicit PlatformEnemyClearUnlockComponent(Platform* owner, int updateOrder = 84);
    ~PlatformEnemyClearUnlockComponent() override;

    // 編集中はこのComponentの制御要求だけを解除する。惑星未設定・対象の敵なしの場合もロック解除へ進む。
    void Update(float deltaTime) override;

    bool GetIsUnlocked() const { return mIsUnlocked; }

private:
    bool HasLivingEnemyOnCurrentPlanet() const;
    void ApplyLockedState();
    void ClearLockedState();

    Platform* mPlatform = nullptr;
    bool mIsUnlocked = false;
};
