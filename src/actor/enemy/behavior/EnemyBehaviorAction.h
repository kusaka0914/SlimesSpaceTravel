#pragma once

#include "actor/enemy/EnemyConfig.h"

class Enemy;
class EnemyCombat;
class EnemyMovement;
class EnemyStateMachine;
class EnemyStatus;

enum class EnemyAttackPreviewShape {
    Fan,
    Radial,
};

// 攻撃予告の形状と大きさ。rangeはワールド距離、angleRadiansは扇形全体の角度。位置・向きは含まない。
struct EnemyAttackPreview {
    EnemyAttackPreviewShape shape = EnemyAttackPreviewShape::Fan;
    float range = 0.0f;
    float angleRadians = 0.0f;
};

// 行動が操作するEnemyと各担当への参照。所有権は持たず、各呼び出し中は参照先が有効であること。
struct EnemyBehaviorContext {
    Enemy& enemy;
    EnemyStatus& status;
    EnemyMovement& movement;
    EnemyCombat& combat;
    EnemyStateMachine& stateMachine;
};

// Runningは継続を要求し、Finished・FailedはいずれもControllerによる行動の終了・再選択へ進む。
enum class EnemyBehaviorActionResult {
    Running,
    Finished,
    Failed,
};

// 敵の1つの行動を表す共通インターフェース。開始・継続条件と選択重みを提供し、状態変更は各行動が行う。
// Controllerが選択・切替を管理し、実行中の行動にはEnter→Update→Exitの順で処理を渡す。
class EnemyBehaviorAction {
public:
    explicit EnemyBehaviorAction(const EnemyBehaviorActionConfig& config)
        : mConfig(config)
    {
    }

    virtual ~EnemyBehaviorAction() = default;

    virtual const char* GetType() const = 0;

    // 行動選択時の候補にできるかを返す。実際の開始処理はEnterで行う。
    virtual bool CanStart(const EnemyBehaviorContext& context) const = 0;

    // Updateの前後で確認され、falseなら終了する。更新中の状態遷移もここで継続可否へ反映する。
    virtual bool CanContinue(const EnemyBehaviorContext& context) const = 0;

    // 候補間の相対的な抽選重み。負値はControllerが0に制限するが、0でも候補自体からは除外しない。
    virtual float Evaluate(const EnemyBehaviorContext& context) const = 0;

    // 行動が新たに選択された時に呼ばれる。行動固有の回数・判定履歴などを初期化する。
    virtual void Enter(EnemyBehaviorContext& context) { (void)context; }

    // deltaTimeは秒。行動を進めて継続・終了・失敗を返す。RunningでもCanContinueがfalseなら終了する。
    virtual EnemyBehaviorActionResult Update(EnemyBehaviorContext& context, float deltaTime) = 0;

    // 終了・中断時、次の行動のEnterより先に呼ばれる。Controllerの再設定・破棄では呼ばれない。
    virtual void Exit(EnemyBehaviorContext& context) { (void)context; }

    // 予告形状を提供できる場合だけpreviewへ書き込んでtrue。既定実装は変更せずfalseを返す。
    virtual bool GetAttackPreview(EnemyAttackPreview& preview) const
    {
        (void)preview;
        return false;
    }

protected:
    float GetWeight() const { return mConfig.weight; }
    float GetParameter(const std::string& name, float fallback) const { return mConfig.GetParameter(name, fallback); }

private:
    EnemyBehaviorActionConfig mConfig;
};
