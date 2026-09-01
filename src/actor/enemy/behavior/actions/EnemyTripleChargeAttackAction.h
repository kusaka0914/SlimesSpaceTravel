#pragma once

#include "actor/enemy/behavior/EnemyBehaviorAction.h"

class EnemyTripleChargeAttackAction final : public EnemyBehaviorAction {
public:
    explicit EnemyTripleChargeAttackAction(const EnemyBehaviorActionConfig& config);

    const char* GetType() const override { return "tripleChargeAttack"; }
    bool CanStart(const EnemyBehaviorContext& context) const override;
    bool CanContinue(const EnemyBehaviorContext& context) const override;
    float Evaluate(const EnemyBehaviorContext& context) const override;

    void Enter(EnemyBehaviorContext& context) override;
    EnemyBehaviorActionResult Update(EnemyBehaviorContext& context, float deltaTime) override;

private:
    int mChargeCount = 3;
    int mCompletedCharges = 0;
    float mRepeatDelay = 1.0f;
};
