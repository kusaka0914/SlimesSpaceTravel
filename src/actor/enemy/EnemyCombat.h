#pragma once

#include "actor/enemy/EnemyGuardGauge.h"

#include <glm/glm.hpp>

class Enemy;
class EnemyMovement;
class EnemyStateMachine;
class EnemyStatus;
class Player;

// ガード破壊時の打ち上げと、攻撃範囲内のPlayerへの被弾処理を担当する。
class EnemyCombat {
public:
    // Aliveの敵だけに適用する。全破壊時、接地中またはKnockedBackなら打ち上げも開始する。
    EnemyGuardDamageResult ApplyGuardDamage(
        Enemy& enemy,
        EnemyStatus& status,
        EnemyMovement& movement,
        EnemyStateMachine& stateMachine,
        float guardDamage,
        float deltaTime);

    // 現在区画の破壊に必要な量までダメージを補い、ApplyGuardDamageと同じ打ち上げ条件を適用する。
    EnemyGuardDamageResult ApplyGuardDamageEnsuringCurrentSegmentBreak(
        Enemy& enemy,
        EnemyStatus& status,
        EnemyMovement& movement,
        EnemyStateMachine& stateMachine,
        float minimumGuardDamage,
        float deltaTime);

    // 全ガードを破壊する。Alive以外は変更せず空の結果を返し、全破壊時は打ち上げ条件も評価する。
    EnemyGuardDamageResult BreakGuard(
        Enemy& enemy,
        EnemyStatus& status,
        EnemyMovement& movement,
        EnemyStateMachine& stateMachine,
        float deltaTime);

    // 攻撃の有効時間中、開始時の判定基準を使って同じ惑星面のPlayerへ被弾処理を依頼する。
    // 命中履歴に登録済みのPlayerには繰り返さない。履歴の解除は攻撃開始側で行う。
    void TryApplyAttack(Enemy& enemy, EnemyStatus& status, const EnemyStateMachine& stateMachine,
                        float deltaTime);

    // 扇形攻撃を判定する。rangeはワールド距離、angleRadiansは全体の角度で、有効時間・命中履歴も確認する。
    void TryApplyFanAttack(Enemy& enemy, EnemyStatus& status, const EnemyStateMachine& stateMachine,
                           float range, float angleRadians,
                           float deltaTime);

    // 接地中のPlayerだけを円形範囲で判定する。rangeはワールド距離で、有効時間・命中履歴も確認する。
    void TryApplyGroundRadialAttack(Enemy& enemy, EnemyStatus& status, const EnemyStateMachine& stateMachine,
                                    float range,
                                    float deltaTime);

    // 同じ惑星面の有効なPlayerとのワールド距離を判定する。nullptr・惑星未設定ならfalse。
    bool IsPlayerInRange(const Enemy& enemy, Player* player, float range) const;

private:
    void LaunchWhenGuardBreaks(
        Enemy& enemy,
        EnemyStatus& status,
        EnemyMovement& movement,
        EnemyStateMachine& stateMachine,
        const EnemyGuardDamageResult& damageResult,
        float deltaTime);
    bool CanHitPlayer(const Enemy& enemy, const Player* player) const;
};
