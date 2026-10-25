#pragma once

#include <glm/glm.hpp>
#include <vector>

class Enemy;
class Player;
class PlayerCombat;
class PhysicsSystem;

// 同じ惑星面にいる敵への攻撃命中を幾何判定する。ダメージや状態変更は行わない。
class PlayerAttackHitDetector {
public:
    explicit PlayerAttackHitDetector(PhysicsSystem& physicsSystem);

    // 戻り値は非所有参照の一覧。惑星未設定なら空。Strongは接地中の敵を対象外とする。
    std::vector<Enemy*> FindHitEnemies(Player& player, const PlayerCombat& combat) const;

    // Playerの位置から敵モデルの境界までを判定する。rangeはワールド距離で、負値は0として扱う。
    std::vector<Enemy*> FindEnemiesInRadius(Player& player, float range) const;

    // ワールド座標の移動区間を判定し、フレーム間に通過した敵も拾う。scaleは水平・垂直の判定倍率。
    std::vector<Enemy*> FindEnemiesTouchingAirDodgeMovement(Player& player, const glm::vec3& movementStart,
                                                            const glm::vec3& movementEnd, float horizontalHitboxScale,
                                                            float verticalHitboxScale) const;

private:
    bool IsGroundedEnemyWithinAirAttackHeight(const Player& player, const Enemy& enemy) const;
    bool IsEnemyModelHitByAttack(const Player& player, const Enemy& enemy, float attackRange, float attackAngle) const;
    bool IsEnemyModelWithinRadius(const Player& player, const Enemy& enemy, float range) const;
    bool DoesAirDodgePathTouchEnemyModel(const Player& player, const Enemy& enemy, const glm::vec3& movementStart,
                                         const glm::vec3& movementEnd, float horizontalHitboxScale,
                                         float verticalHitboxScale) const;
    bool IsEnemyHitByAttack(float dist, float dot, float effectiveRange, float attackAngle) const;

    PhysicsSystem& mPhysicsSystem;
};
