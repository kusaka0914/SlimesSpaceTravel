#include "actor/enemy/EnemyAttackAnimationController.h"

#include "system/mesh/LoadedModel.h"

void EnemyAttackAnimationController::Configure(const LoadedModel* model)
{
    Stop();
    mIsAvailable = false;
    mAnimation.SetLoadedModel(nullptr);
    if (!model || !model->IsLoaded() || !model->HasSkinningData()) {
        return;
    }
    mAnimation.Configure({{"idle", {"Idle", PlayerAnimationPlaybackMode::BaseLoop}},
                          {"attack", {"Attack", PlayerAnimationPlaybackMode::OneShot}}});
    mAnimation.SetLoadedModel(model);
    mIsAvailable = mAnimation.HasAnimation("attack");
}

void EnemyAttackAnimationController::Start(float playbackRate)
{
    mIsActive = mIsAvailable && mAnimation.RequestAnimation("attack", true, playbackRate);
}

void EnemyAttackAnimationController::Update(float deltaTime)
{
    if (mIsActive) {
        mAnimation.Update(deltaTime);
    }
}

void EnemyAttackAnimationController::Stop()
{
    if (!mIsActive) {
        return;
    }
    mIsActive = false;
    mAnimation.ResetToAnimation();
}

const std::vector<glm::mat4>* EnemyAttackAnimationController::GetSkinningMatrices() const
{
    return mIsActive ? mAnimation.GetSkinningMatrices() : nullptr;
}
