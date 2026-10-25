#pragma once

#include <glm/glm.hpp>

class Enemy;
class PhysicsSystem;

// 移動経路上の地面を探索し、接地中の敵が足場の端から踏み出す移動を制限する。
class EnemyGrounding {
public:
    explicit EnemyGrounding(PhysicsSystem& physicsSystem);

    // 現在位置からdesiredPosへ進む経路を調べ、足場が途切れる手前のワールド位置までに制限する。
    glm::vec3 ClampMoveToGround(const Enemy& enemy, const glm::vec3& desiredPos) const;

    // checkPosの下に傾斜判定を通る地面があるかを調べる。物理ワールドなし・上方向が無効ならtrueを返す。
    bool HasGroundBelow(const Enemy& enemy, const glm::vec3& checkPos) const;

private:
    PhysicsSystem& mPhysicsSystem;
};
