#pragma once

#include "actor/player/PlayerAnimationController.h"

struct LoadedModel;

// 敵自身のモデルに含まれる攻撃クリップを再生する。モデルやGPU資源は所有しない。
class EnemyAttackAnimationController {
public:
    // モデルは非所有参照。スキニングやAttackクリップがない場合はアニメーションを再生しない。
    void Configure(const LoadedModel* model);
    void Start(float playbackRate);
    void Update(float deltaTime);
    void Stop();

    const std::vector<glm::mat4>* GetSkinningMatrices() const;

private:
    PlayerAnimationController mAnimation;
    bool mIsAvailable = false;
    bool mIsActive = false;
};
