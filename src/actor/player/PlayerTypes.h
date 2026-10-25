#pragma once

#include <glm/glm.hpp>

enum class PlayerControlStyle {
    Standard,
    Assist,
};

enum class PlayerActionState {
    Idle,
    Dodging,
    Attacking,
    StrongAttacking,
    AirSlamAttacking,
    KnockedBack,
};

enum class PlayerAttackKind {
    Normal,
    Wide,
    Strong,
    Charged,
};

enum class PlayerAttackInputKind {
    None,
    Normal,
    Wide,
};

// 攻撃レイの始点と終点（ワールド座標）。
struct PlayerRaySegment {
    glm::vec3 from;
    glm::vec3 to;
};
