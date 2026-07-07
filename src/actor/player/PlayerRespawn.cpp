#include "actor/player/PlayerRespawn.h"

#include "actor/FallRespawnPoint.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "actor/player/PlayerCombat.h"
#include "actor/player/PlayerStateMachine.h"
#include "actor/player/PlayerStatus.h"
#include "actor/player/PlayerTypes.h"
#include "system/PhysicsSystem.h"

#include <glm/glm.hpp>

void PlayerRespawn::ApplyFallDamageAndRespawn(Player& player, PlayerStateMachine& stateMachine, PlayerCombat& combat,
                                              PlayerStatus& status, float damage)
{
    if (!status.IsAlive()) {
        return;
    }

    status.TakeFallDamage(damage);

    if (!status.IsAlive()) {
        return;
    }

    stateMachine.ChangeState(PlayerActionState::Idle);
    combat.CancelSpecialAttack();
    player.SetVelocity(glm::vec3(0.0f));
    Respawn(player);
}

void PlayerRespawn::Respawn(Player& player)
{
    player.SetPos(mRestartPos);
}

void PlayerRespawn::Restart(Player& player, PlayerStateMachine& stateMachine, PlayerStatus& status)
{
    stateMachine.ChangeState(PlayerActionState::Idle);
    status.RestoreFullHp();
    Respawn(player);
}

bool PlayerRespawn::IsFallIntoPlanetInside(const Player& player) const
{
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

void PlayerRespawn::CheckFallRespawn(Player& player, PlayerStateMachine& stateMachine, PlayerCombat& combat,
                                     PlayerStatus& status, const glm::vec3& prevPos)
{
    if (!player.GetGame() || !player.GetGame()->GetPhysicsSystem()) {
        return;
    }

    if (!player.GetIsActive()) {
        return;
    }

    if (!status.IsAlive()) {
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

    ApplyFallDamageAndRespawn(player, stateMachine, combat, status, point->GetDamage());
}
