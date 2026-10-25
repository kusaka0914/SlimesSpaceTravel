#pragma once

#include <memory>
#include <random>
#include <string>
#include <vector>

class Enemy;
class EnemyBehaviorAction;
class EnemyCombat;
class EnemyMovement;
class EnemyStateMachine;
class EnemyStatus;
struct EnemyBehaviorConfig;
struct EnemyBehaviorContext;
struct EnemyAttackPreview;

// 行動を所有し、開始条件と重みに基づく選択、継続判定、Enter・Exitによる切替を管理する。
class EnemyBehaviorController {
public:
    EnemyBehaviorController();
    ~EnemyBehaviorController();

    // 行動一覧と実行状態を置き換える。旧行動のExitは呼ばず、未知のtypeはログを出して除外する。
    // 基本行動も生成し、設定した行動に開始可能な候補がない場合の代替に使う。
    void Configure(const EnemyBehaviorConfig& config);

    // deltaTimeは秒。行動を最大1回更新し、継続不可・終了・失敗なら再選択する。
    // 攻撃終了時にボスの退避が始まった場合、次の行動はここでは選択しない。
    void Update(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, EnemyCombat& combat,
                EnemyStateMachine& stateMachine, float deltaTime);

    const std::string& GetProfileName() const { return mProfileName; }

    // 実行中の行動がなければ"none"を返す。
    const char* GetCurrentActionType() const;

    // 現在行動が予告形状を提供する場合だけtrueを返す。形状には位置・向きや表示タイミングを含まない。
    bool GetCurrentAttackPreview(EnemyAttackPreview& preview) const;

private:
    EnemyBehaviorAction* SelectAction(const EnemyBehaviorContext& context);
    void SwitchAction(EnemyBehaviorAction* nextAction, EnemyBehaviorContext& context);

private:
    std::string mProfileName = "legacyMelee";
    std::vector<std::unique_ptr<EnemyBehaviorAction>> mActions;
    std::vector<std::unique_ptr<EnemyBehaviorAction>> mFallbackActions;
    EnemyBehaviorAction* mCurrentAction = nullptr;
    std::mt19937 mRandomEngine;
};
