#pragma once

#include "actor/enemy/behavior/EnemyBehaviorAction.h"

class EnemyIdleAction final : public EnemyBehaviorAction {
public:
    explicit EnemyIdleAction(const EnemyBehaviorActionConfig& config)
        : EnemyBehaviorAction(config)
    {
    }

    const char* GetType() const override { return "idle"; }
    bool CanStart(const EnemyBehaviorContext& context) const override;
    bool CanContinue(const EnemyBehaviorContext& context) const override;
    float Evaluate(const EnemyBehaviorContext& context) const override;
    EnemyBehaviorActionResult Update(EnemyBehaviorContext& context, float deltaTime) override;
};
