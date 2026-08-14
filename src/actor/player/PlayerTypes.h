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

struct PlayerRaySegment {
    glm::vec3 from;
    glm::vec3 to;
};
