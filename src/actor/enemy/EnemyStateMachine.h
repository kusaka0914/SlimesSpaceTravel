#pragma once

#include "actor/enemy/EnemyAttackGeometry.h"

#include <random>

class Enemy;
class EnemyBehaviorController;
class EnemyCombat;
class EnemyMovement;
class EnemyStatus;
struct EnemyBossManeuverConfig;

// 生存状態と行動状態を管理し、通常行動をEnemyBehaviorControllerへ、空中・被弾・死亡処理を各担当へ渡す。
// deltaTimeと各タイマーは秒。
class EnemyStateMachine {
public:
    enum class LifeState { Alive, Dying, Dead };

    enum class ActionState {
        Idle,
        Tracking,
        PreparingAttack,
        PreAttackApproach,
        Attacking,
        PostAttackRetreatDelay,
        PostAttackRetreat,
        PostRetreatRecovery,
        Launched,
        KnockedBack,
    };

    EnemyStateMachine();

    // 確率を0〜100%、距離・速度・時間を0以上に制限してボスの接近・退避設定を反映する。
    void ConfigureBossManeuver(const EnemyBossManeuverConfig& config);

    // 生存中の更新入口。空中・被弾・ボスの接近退避を優先し、それ以外の接地行動はbehaviorControllerへ渡す。
    void UpdateAlive(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, EnemyCombat& combat,
                     EnemyBehaviorController& behaviorController, float deltaTime);

    // 死亡演出の移動と残り時間を更新し、満了時にFinishDyingを呼ぶ。
    void UpdateDying(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, float deltaTime);

    void UpdateIdle(Enemy& enemy, EnemyStatus& status, EnemyCombat& combat);
    void UpdateTracking(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, EnemyCombat& combat, float deltaTime);
    void UpdatePreparingAttack(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, float deltaTime);
    void UpdatePreAttackApproach(
        Enemy& enemy,
        EnemyStatus& status,
        EnemyMovement& movement,
        float deltaTime);
    void UpdateAttacking(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, EnemyCombat& combat, float deltaTime);
    void UpdateKnockedBack(Enemy& enemy, EnemyStatus& status, EnemyMovement& movement, float deltaTime);
    void UpdatePostAttackRetreatDelay(float deltaTime);
    void UpdatePostAttackRetreat(
        Enemy& enemy,
        EnemyStatus& status,
        EnemyMovement& movement,
        float deltaTime);
    void UpdatePostRetreatRecovery(
        Enemy& enemy,
        EnemyStatus& status,
        float deltaTime);

    void StartIdle(Enemy& enemy);
    void StartTracking(Enemy& enemy);
    void TryStartPreparingAttack(Enemy& enemy, EnemyStatus& status, EnemyCombat& combat);

    // PreparingAttackへ移り、攻撃準備タイマーと接近行動の判定状態を初期化する。
    void StartPreparingAttack(Enemy& enemy, EnemyStatus& status);

    // 現在の位置・方向を攻撃基準として保存し、攻撃タイマー・カウンター受付・命中履歴を初期化する。
    void StartAttacking(Enemy& enemy, EnemyStatus& status);

    // 接地中で対象がいるIdleのボスに、設定確率で退避を開始する。開始時true、条件不成立時false。
    bool TryStartPostAttackRetreat(Enemy& enemy, const EnemyStatus& status);
    void StartLaunched(Enemy& enemy);

    // KnockedBackへ移り、秒数と押し戻し方向を設定して頂点待機タイマーを解除する。速度は設定しない。
    void StartKnockedBack(Enemy& enemy, EnemyStatus& status, float knockBackTimer);

    // HPを0にして死亡演出を開始する。ボス戦では他の通常敵の撃破、演出用の再配置、BGM停止も行う。
    void StartDying(Enemy& enemy, EnemyStatus& status);

    // ガードを最大値へ戻し、通常着地判定を再開して頂点待機を解除する。行動状態は変更しない。
    void FinishLaunched(Enemy& enemy, EnemyStatus& status);

    // 撃破演出と必要なジュエル生成を依頼し、Deadへ移して非アクティブ化する。所属惑星へ死亡を通知する。
    void FinishDying(Enemy& enemy, const EnemyStatus& status);

    LifeState GetLifeState() const { return mLifeState; }

    // 生存状態の値だけを変更する。死亡演出や後処理は開始しない。
    void SetLifeState(LifeState lifeState) { mLifeState = lifeState; }

    ActionState GetActionState() const { return mActionState; }

    // 行動状態の値だけを変更する。各Start…が行うタイマーなどの初期化は含まない。
    void SetActionState(ActionState actionState) { mActionState = actionState; }

    // 予兆未通知で、攻撃準備の残り時間が1秒以下かを返す。通知済みフラグは変更しない。
    bool IsJustBeforeAttack(const EnemyStatus& status) const;
    bool IsAttackImpactActive(const EnemyStatus& status) const;
    bool ShouldPreservePreparationTimer() const
    {
        return mShouldPreservePreparationTimer;
    }
    bool IsProgressing(const EnemyStatus& status) const;
    bool IsAlive() const { return mLifeState == LifeState::Alive; }
    bool IsDead() const { return mLifeState == LifeState::Dead; }
    bool IsAlive(const Enemy& enemy) const;
    const EnemyAttackFrame& GetActiveAttackFrame() const
    {
        return mActiveAttackFrame;
    }

private:
    bool ShouldTriggerProbability(float probabilityPercent);

    LifeState mLifeState;
    ActionState mActionState;
    float mPreAttackApproachProbabilityPercent = 0.0f;
    float mPreAttackApproachSpeed = 12.0f;
    float mPreAttackApproachStopDistance = 2.5f;
    float mPostAttackRetreatProbabilityPercent = 0.0f;
    float mPostAttackRetreatDelaySeconds = 1.0f;
    float mPostAttackRetreatDelayRemainingSeconds = 0.0f;
    float mPostAttackRetreatSpeed = 12.0f;
    float mPostAttackRetreatDistance = 4.0f;
    float mPostAttackRetreatRemainingDistance = 0.0f;
    float mPostRetreatRecoverySeconds = 2.0f;
    float mPostRetreatRecoveryRemainingSeconds = 0.0f;
    float mPostRetreatFollowupApproachProbabilityPercent = 0.0f;
    bool mHasEvaluatedPreAttackApproach = false;
    bool mShouldPreservePreparationTimer = false;
    EnemyAttackFrame mActiveAttackFrame;
    std::mt19937 mManeuverRandomEngine;
};
