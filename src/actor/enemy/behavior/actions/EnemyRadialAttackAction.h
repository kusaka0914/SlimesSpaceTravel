#pragma once

#include "actor/enemy/behavior/EnemyBehaviorAction.h"

// ボスの全周攻撃。準備中に予兆を進め、攻撃中は接地中のPlayerだけに1回の命中判定を試みる。
// 設定のrangeはワールド距離、windUpDuration・attackDurationは秒。
class EnemyRadialAttackAction final : public EnemyBehaviorAction {
public:
    explicit EnemyRadialAttackAction(const EnemyBehaviorActionConfig& config);

    const char* GetType() const override { return "radialAttack"; }
    bool CanStart(const EnemyBehaviorContext& context) const override;
    bool CanContinue(const EnemyBehaviorContext& context) const override;
    float Evaluate(const EnemyBehaviorContext& context) const override;

    // 判定履歴をリセットし、準備時間を設定する。状態機械から準備時間の保持を要求されている場合は維持する。
    void Enter(EnemyBehaviorContext& context) override;
    EnemyBehaviorActionResult Update(EnemyBehaviorContext& context, float deltaTime) override;
    bool GetAttackPreview(EnemyAttackPreview& preview) const override;

private:
    float mRange = 5.0f;
    float mWindUpDuration = 2.0f;
    float mAttackDuration = 0.5f;
    bool mHasAppliedDamage = false;
};
