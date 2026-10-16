#pragma once

#include <vector>

class Enemy;
class Player;
class PlayerCombat;
class PlayerJewelGauge;
class PlayerMovement;
class PlayerStatus;

// 探索済みの敵へダメージ・ガード破壊・外力を適用し、命中演出と攻撃後の状態を更新する。
class PlayerAttackResolver {
public:
    // 命中一覧と障害物への反応から結果を適用する。空振りでも攻撃後の硬直・コンボを更新する。
    void ResolveAttack(Player& player, PlayerMovement& movement, PlayerStatus& status, PlayerCombat& combat,
                       const std::vector<Enemy*>& hitEnemies, bool didHitHazardActor, float deltaTime) const;

    // 地上の敵のガードを破壊する。カウンター可能な敵への命中時はジュエルも回復する。
    void ResolveSpecialAttack(Player& player, PlayerJewelGauge& jewelGauge, const std::vector<Enemy*>& hitEnemies,
                              float chargedAttackDamage, float deltaTime) const;

    // 接地中の敵へ着地衝撃を適用し、1体以上に命中した場合にtrueを返す。
    bool ResolveAirSlamAttack(Player& player, const PlayerMovement& movement, PlayerCombat& combat,
                              const std::vector<Enemy*>& hitEnemies, float deltaTime) const;

    // 空中の敵の叩き落としを開始し、1体以上で開始できればtrue。ダメージは敵の着地時に確定する。
    // shouldAssignImpactFeedbackがtrueでも、着地時の命中演出は最初に開始した1体だけに割り当てる。
    bool ResolveAirSlamContact(Player& player, const PlayerMovement& movement, const std::vector<Enemy*>& hitEnemies,
                               float enemyDownwardSpeed, float maximumDamage, float fullDamageHeight,
                               float minimumDamageRatio, bool shouldAssignImpactFeedback) const;

    // ダメージと回避方向の外力を適用する。enemyLiftHeightが正なら持ち上げ、1体以上の命中でtrue。
    bool ResolveAirDodgeAttack(Player& player, const PlayerMovement& movement, const std::vector<Enemy*>& hitEnemies,
                               float damage, float guardDamage, float enemyPushSpeed, float enemyPushDampingPerSecond,
                               float enemyLiftHeight, float deltaTime) const;
};
