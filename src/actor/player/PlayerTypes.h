#pragma once

#include <glm/glm.hpp>

enum class PlayerActionState {
    Idle,
    Dodging,
    Attacking,
    Charging,
    StrongAttacking,
    KnockedBack,
};

enum class PlayerAttackKind {
    Normal,
    Wide,
    Strong,
};

struct PlayerRaySegment {
    glm::vec3 from;
    glm::vec3 to;
};
