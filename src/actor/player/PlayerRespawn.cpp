#include "actor/player/PlayerRespawn.h"

#include "actor/FallRespawnPoint.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "actor/player/PlayerCombat.h"
#include "actor/player/PlayerModuleContext.h"
#include "actor/player/PlayerStatus.h"
#include "system/PhysicsSystem.h"

#include <algorithm>
#include <glm/glm.hpp>

void PlayerRespawn::ApplyFallDamageAndRespawn(PlayerModuleContext& context, float damage)
{
    Player& player = context.player;
    PlayerCombat& combat = context.combat;
    PlayerStatus& status = context.status;

    if (!status.IsAlive()) {
        return;
    }

    status.hp = std::max(0.0f, status.hp - damage);

    if (!status.IsAlive()) {
        return;
    }

    combat.actionState = PlayerActionState::Idle;

    player.SetVelocity(glm::vec3(0.0f));
    Respawn(context);

    status.damageTimer = status.defaultDamageTimer;
    status.invincibleTimer = status.defaultInvincibleTimer;
}

void PlayerRespawn::Respawn(PlayerModuleContext& context)
{
    context.player.ModulePos() = restartPos;
}

void PlayerRespawn::Restart(PlayerModuleContext& context)
{
    context.combat.actionState = PlayerActionState::Idle;
    context.status.hp = context.status.maxHp;

    Respawn(context);
}

bool PlayerRespawn::IsFallIntoPlanetInside(PlayerModuleContext& context)
{
    const Player& player = context.player;

    if (!player.GetCurrentPlanet()) {
        return false;
    }

    if (player.GetCurrentPlanet()->GetPlanetShape() != Planet::PlanetShape::Sphere) {
        return false;
    }

    const float dist = glm::length(player.GetPos() - player.GetCurrentPlanet()->GetPos());
    const float planetHalfRadius = player.GetCurrentPlanet()->GetRadius() * 0.5f;

    return dist < planetHalfRadius;
}

void PlayerRespawn::CheckFallRespawn(PlayerModuleContext& context, const glm::vec3& prevPos)
{
    Player& player = context.player;

    if (!player.GetGame() || !player.GetGame()->GetPhysicsSystem()) {
        return;
    }

    if (!player.GetIsActive()) {
        return;
    }

    if (!context.status.IsAlive()) {
        return;
    }

    auto hit = player.GetGame()->GetPhysicsSystem()->CheckFallRespawnBySweep(prevPos, player.GetPos());

    if (!hit || !hit->actor) {
        return;
    }

    FallRespawnPoint* point = dynamic_cast<FallRespawnPoint*>(hit->actor);

    if (!point) {
        return;
    }

    ApplyFallDamageAndRespawn(context, point->GetDamage());
}