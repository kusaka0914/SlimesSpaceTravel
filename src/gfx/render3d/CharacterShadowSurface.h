#pragma once

#include <glm/glm.hpp>
#include <optional>

class CharacterActor;
class PhysicsSystem;

struct CharacterShadowSurface {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    float distance = 0.0f;
};

namespace CharacterShadow {
// Playerは接地状態にかかわらず真下の床を調べる。ほかのCharacterは接地中の既存位置を使う。
// 空中では最大12ワールド単位まで探し、床がなければnullopt。光源方向には依存しない。
std::optional<CharacterShadowSurface> FindSurface(const CharacterActor& actor, const PhysicsSystem& physics);
} // namespace CharacterShadow
