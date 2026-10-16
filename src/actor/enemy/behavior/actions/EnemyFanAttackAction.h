#pragma once

#include "actor/enemy/behavior/EnemyBehaviorAction.h"

// ボスの扇形攻撃。準備中に予兆を進め、攻撃中は1回だけ命中判定を試みる。
// 設定のrangeはワールド距離、angleDegreesは度、windUpDuration・attackDurationは秒。
class EnemyFanAttackAction final : public EnemyBehaviorAction {
public:
    explicit EnemyFanAttackAction(const EnemyBehaviorActionConfig& config);

    const char* GetType() const override { return "fanAttack"; }
    bool CanStart(const EnemyBehaviorContext& context) const override;
    bool CanContinue(const EnemyBehaviorContext& context) const override;
    float Evaluate(const EnemyBehaviorContext& context) const override;

    // 判定履歴をリセットし、準備時間を設定する。状態機械から準備時間の保持を要求されている場合は維持する。
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
