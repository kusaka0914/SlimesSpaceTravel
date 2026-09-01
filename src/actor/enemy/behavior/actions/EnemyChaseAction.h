#pragma once

#include "actor/enemy/behavior/EnemyBehaviorAction.h"

class EnemyChaseAction final : public EnemyBehaviorAction {
public:
    explicit EnemyChaseAction(const EnemyBehaviorActionConfig& config)
        : EnemyBehaviorAction(config)
    {
    }

    const char* GetType() const override { return "chase"; }
    bool CanStart(const EnemyBehaviorContext& context) const override;
    bool CanContinue(const EnemyBehaviorContext& context) const override;
    float Evaluate(const EnemyBehaviorContext& context) const override;
    EnemyBehaviorActionResult Update(EnemyBehaviorContext& context, float deltaTime) override;
};
