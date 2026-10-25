#pragma once

#include "actor/enemy/behavior/EnemyBehaviorAction.h"

// ボスの近接攻撃をchargeCount回繰り返す。各回の間に対象へ向き直し、repeatDelay秒の準備時間を置く。
class EnemyTripleChargeAttackAction final : public EnemyBehaviorAction {
public:
    explicit EnemyTripleChargeAttackAction(const EnemyBehaviorActionConfig& config);

    const char* GetType() const override { return "tripleChargeAttack"; }
    bool CanStart(const EnemyBehaviorContext& context) const override;
    bool CanContinue(const EnemyBehaviorContext& context) const override;
    float Evaluate(const EnemyBehaviorContext& context) const override;

    // 完了回数を0へ戻す。最初の攻撃の準備タイマーは変更しない。
    void Enter(EnemyBehaviorContext& context) override;
    EnemyBehaviorActionResult Update(EnemyBehaviorContext& context, float deltaTime) override;

private:
    int mChargeCount = 3;
    int mCompletedCharges = 0;
    float mRepeatDelay = 1.0f;
};
