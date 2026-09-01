#pragma once

#include "actor/enemy/behavior/EnemyBehaviorAction.h"

class EnemyRadialAttackAction final : public EnemyBehaviorAction {
public:
    explicit EnemyRadialAttackAction(const EnemyBehaviorActionConfig& config);

    const char* GetType() const override { return "radialAttack"; }
    bool CanStart(const EnemyBehaviorContext& context) const override;
    bool CanContinue(const EnemyBehaviorContext& context) const override;
    float Evaluate(const EnemyBehaviorContext& context) const override;

    void Enter(EnemyBehaviorContext& context) override;
    EnemyBehaviorActionResult Update(EnemyBehaviorContext& context, float deltaTime) override;
    bool GetAttackPreview(EnemyAttackPreview& preview) const override;

private:
    float mRange = 5.0f;
    float mWindUpDuration = 2.0f;
    float mAttackDuration = 0.5f;
    bool mHasAppliedDamage = false;
};
