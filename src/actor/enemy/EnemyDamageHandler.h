#pragma once

class Enemy;
class EnemyMovement;
class EnemyStateMachine;
class EnemyStatus;
class Player;

// HPへのダメージを適用し、被弾リアクション・ノックバック・死亡開始へつなぐ。
class EnemyDamageHandler {
public:
    // Aliveの敵だけにHPダメージを適用し、HPが0以下なら死亡を開始する。playerはこの処理では参照しない。
    // 強攻撃を受けて生存した場合、打ち上げ終了処理によりガードも回復する。
    void ApplyDamage(Enemy& enemy, EnemyStatus& status, EnemyStateMachine& stateMachine, EnemyMovement& movement,
                     float damage, Player* player);

    // Playerの攻撃力を基に2倍のダメージを適用し、被弾状態と攻撃待ち時間を更新する。
    // playerがnullptrなら何もしない。生存状態の確認は呼び出し側で行う。
    void ApplyCounter(Enemy& enemy, EnemyStatus& status, EnemyStateMachine& stateMachine, Player* player);
};
