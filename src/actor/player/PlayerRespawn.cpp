#include "actor/player/PlayerRespawn.h"

#include "Game.h"
#include "Stage.h"
#include "actor/FallRespawnPoint.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "actor/player/PlayerCombat.h"
#include "actor/player/PlayerStateMachine.h"
#include "actor/player/PlayerStatus.h"
#include "actor/player/PlayerTypes.h"
#include "system/PhysicsSystem.h"
#include "system/SceneSystem.h"

#include <algorithm>
#include <glm/glm.hpp>

void PlayerRespawn::ApplyFallDamageAndRespawn(Player& player, PlayerStatus& status, float damage)
{
    if (!status.IsAlive()) {
        return;
    }

    status.TakeFallDamage(damage);

    if (!status.IsAlive()) {
        return;
    }

    SceneSystem* sceneSystem = player.GetGame() ? player.GetGame()->GetSceneSystem() : nullptr;
    if (sceneSystem) {
        sceneSystem->RequestPlayerRespawn(&player);
    }
}

void PlayerRespawn::Respawn(Player& player)
{
    Stage* currentStage = player.GetGame() ? player.GetGame()->GetCurrentStage() : nullptr;
    if (currentStage) {
        const std::vector<Planet*>& planets = currentStage->GetPlanets();
        if (mRestartPlanetIndex >= 0 && mRestartPlanetIndex < static_cast<int>(planets.size()) &&
            planets[mRestartPlanetIndex]) {
            player.SetCurrentPlanetNum(mRestartPlanetIndex);
            player.SetCurrentPlanet(planets[mRestartPlanetIndex]);
        }
    }

    player.SetPos(mRestartPos);
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
    (void)stateMachine;
    (void)combat;

    if (!player.GetGame() || !player.GetGame()->GetPhysicsSystem()) {
        return;
    }

    if (!player.GetIsActive()) {
        return;
    }

    if (!status.IsAlive()) {
        return;
    }

    auto hit =
        player.GetGame()->GetPhysicsSystem()->CheckFallRespawnBySweep(
            &player,
            prevPos,
            player.GetPos());

    if (!hit || !hit->actor) {
        return;
    }

    FallRespawnPoint* point = dynamic_cast<FallRespawnPoint*>(hit->actor);

    if (!point) {
        return;
    }

    ApplyFallDamageAndRespawn(player, status, point->GetDamage());
}

bool PlayerRespawn::UpdateMissingGroundSurfaceRespawn(
    Player& player,
    const PlayerStatus& status,
    float deltaTime)
{
    const bool groundSurfaceDetectedThisFrame =
        mGroundSurfaceDetectedThisFrame;
    mGroundSurfaceDetectedThisFrame = false;

    if (!player.GetIsActive() || !status.IsAlive() || mRespawnFadeRequested) {
        return false;
    }

    const bool hasGroundSupport =
        player.GetOnGround() ||
        groundSurfaceDetectedThisFrame ||
        player.IsEllipseAirborneGravityActive();
    if (hasGroundSupport) {
        mMissingGroundSurfaceDurationSeconds = 0.0f;
        return false;
    }

    mMissingGroundSurfaceDurationSeconds += std::max(0.0f, deltaTime);
    if (mMissingGroundSurfaceDurationSeconds <
        missingGroundSurfaceRespawnDelaySeconds) {
        return false;
    }

    SceneSystem* sceneSystem = player.GetGame() ? player.GetGame()->GetSceneSystem() : nullptr;
    if (!sceneSystem || !sceneSystem->RequestPlayerRespawn(&player)) {
        return false;
    }

    mRespawnFadeRequested = true;
    return true;
}

void PlayerRespawn::OnRespawnCompleted()
{
    mMissingGroundSurfaceDurationSeconds = 0.0f;
    mGroundSurfaceDetectedThisFrame = false;
    mRespawnFadeRequested = false;
}
