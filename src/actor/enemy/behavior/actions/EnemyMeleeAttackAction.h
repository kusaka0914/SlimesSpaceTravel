#pragma once

#include "actor/enemy/behavior/EnemyBehaviorAction.h"

class EnemyMeleeAttackAction final : public EnemyBehaviorAction {
public:
    explicit EnemyMeleeAttackAction(const EnemyBehaviorActionConfig& config)
        : EnemyBehaviorAction(config)
    {
    }

    const char* GetType() const override { return "meleeAttack"; }
    bool CanStart(const EnemyBehaviorContext& context) const override;
    bool CanContinue(const EnemyBehaviorContext& context) const override;
    float Evaluate(const EnemyBehaviorContext& context) const override;
    EnemyBehaviorActionResult Update(EnemyBehaviorContext& context, float deltaTime) override;
};
