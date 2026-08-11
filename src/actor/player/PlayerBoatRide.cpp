#include "actor/player/PlayerBoatRide.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Boat.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "actor/player/PlayerMovement.h"
#include "actor/player/PlayerRespawn.h"

#include <glm/glm.hpp>
#include <vector>

void PlayerBoatRide::Update(Player& player, PlayerMovement& movement, PlayerRespawn& respawn)
{
    if (!player.GetCurrentPlanet()) {
        return;
    }

    const std::vector<Boat*>& boats = player.GetCurrentPlanet()->GetBoats();
    if (boats.empty()) {
        return;
    }

    for (Boat* boat : boats) {
        if (!boat->GetIsActive()) {
            continue;
        }

        if (boat->GetIsMoving()) {
            FollowMovingBoat(player, boat);
            return;
        }

        if (IsTouchingBoat(player, boat)) {
            StartRidingBoat(player, boat);
            return;
        }
    }
}

void PlayerBoatRide::FollowMovingBoat(Player& player, Boat* boat) const
{
    player.SetPos(boat->GetPos());
    player.SetIsActive(false);
}

bool PlayerBoatRide::IsTouchingBoat(const Player& player, Boat* boat) const
{
    constexpr float boatTouchRadius = 0.9f;
    const float distToBoat = glm::length(player.GetPos() - boat->GetPos());

    return distToBoat <= boatTouchRadius;
}

void PlayerBoatRide::StartRidingBoat(Player& player, Boat* boat) const
{
    if (!boat || !player.GetIsActive()) {
        return;
    }

    Player* ridingPlayer = &player;
    if (Game* game = player.GetGame()) {
        ridingPlayer = game->MergeSplitPlayerForBoatRide(&player);
    }

    boat->StartTravel();
    if (ridingPlayer) {
        ridingPlayer->SetIsActive(false);
    }
}

void PlayerBoatRide::OnBoatArrived(Player& player, PlayerMovement& movement, PlayerRespawn& respawn, Boat* boat) const
{
    if (!boat || !boat->GetDestPlanet()) {
        return;
    }

    Planet* destinationPlanet = boat->GetDestPlanet();
    player.SetCurrentPlanet(destinationPlanet);

    Stage* currentStage = player.GetGame()
        ? player.GetGame()->GetCurrentStage()
        : nullptr;
    if (currentStage) {
        const std::vector<Planet*>& planets = currentStage->GetPlanets();
        for (int planetIndex = 0;
             planetIndex < static_cast<int>(planets.size());
             ++planetIndex) {
            if (planets[planetIndex] == destinationPlanet) {
                movement.SetCurrentPlanetNum(planetIndex);
                break;
            }
        }
    }

    player.SetPos(boat->GetDestPos());

    respawn.SetRestartPos(player.GetPos());
    respawn.SetRestartPlanetIndex(movement.GetCurrentPlanetNum());

    player.SetVelocity(glm::vec3(0.0f));
    player.SetIsActive(true);

    player.RefreshFallbackUpVec();
}
