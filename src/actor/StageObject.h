#pragma once

#include "actor/Actor.h"

class Game;

// 静的なステージ配置物。上方向を毎フレーム再探索せず、衝突の有効状態を保持する。
class StageObject : public Actor {
public:
    explicit StageObject(Game* game);

    void SetCollisionEnabled(bool enabled) { mCollisionEnabled = enabled; }
    bool GetCollisionEnabled() const { return mCollisionEnabled; }

protected:
    bool ShouldUpdateUpVecEveryFrame() const override { return false; }

private:
    bool mCollisionEnabled = true;
};
