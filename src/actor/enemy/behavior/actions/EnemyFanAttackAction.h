#pragma once

#include "actor/enemy/behavior/EnemyBehaviorAction.h"

class EnemyFanAttackAction final : public EnemyBehaviorAction {
public:
    explicit EnemyFanAttackAction(const EnemyBehaviorActionConfig& config);

    const char* GetType() const override { return "fanAttack"; }
    bool CanStart(const EnemyBehaviorContext& context) const override;
    bool CanContinue(const EnemyBehaviorContext& context) const override;
    float Evaluate(const EnemyBehaviorContext& context) const override;

    void Enter(EnemyBehaviorContext& context) override;
    EnemyBehaviorActionResult Update(EnemyBehaviorContext& context, float deltaTime) override;
    bool GetAttackPreview(EnemyAttackPreview& preview) const override;

private:
    float mRange = 6.0f;
    float mAngleRadians = 2.0943951f;
    float mWindUpDuration = 2.0f;
    float mAttackDuration = 0.5f;
    bool mHasAppliedDamage = false;
};
