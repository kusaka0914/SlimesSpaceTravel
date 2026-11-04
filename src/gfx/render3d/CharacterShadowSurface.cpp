#include "gfx/render3d/CharacterShadowSurface.h"

#include "actor/CharacterActor.h"
#include "actor/Player.h"
#include "system/PhysicsSystem.h"

#include <algorithm>

std::optional<CharacterShadowSurface> CharacterShadow::FindSurface(const CharacterActor& actor,
                                                                   const PhysicsSystem& physics)
{
    const auto* player = dynamic_cast<const Player*>(&actor);
    const auto upDirection =
        player && !actor.GetOnGround() ? player->CalculateAirbornePhysicsUpDirection() : actor.GetUpVec();
    const float upLength = glm::length(upDirection);
    if (upLength <= 0.0001f) {
        return std::nullopt;
    }
    const auto up = upDirection / upLength;
    if (actor.GetOnGround() && !player) {
        return CharacterShadowSurface{actor.GetPos(), up, 0.0f};
    }
    constexpr float rayStartOffset = 0.15f;
    constexpr float maximumDistance = 12.0f;
    const auto from = actor.GetPos() + up * rayStartOffset;
    const auto to = actor.GetPos() - up * maximumDistance;
    for (const auto& hit : physics.RaycastStageSurfaces(from, to)) {
        if (hit.actor == &actor || dynamic_cast<const CharacterActor*>(hit.actor)) {
            continue;
        }
        const float normalLength = glm::length(hit.hitNormal);
        if (normalLength <= 0.0001f) {
            continue;
        }
        const auto normal = hit.hitNormal / normalLength;
        const bool acceptsSurface =
            player ? CharacterActor::IsWalkableGroundNormal(normal, up) : glm::dot(normal, up) >= 0.35f;
        if (!acceptsSurface) {
            continue;
        }
        const float distance = std::max(0.0f, glm::dot(actor.GetPos() - hit.hitPos, up));
        if (distance < maximumDistance) {
            return CharacterShadowSurface{hit.hitPos, normal, distance};
        }
    }
    return std::nullopt;
}
