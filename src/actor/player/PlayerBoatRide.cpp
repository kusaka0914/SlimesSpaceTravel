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
    Game* game = player.GetGame();
    Stage* currentStage = game ? game->GetCurrentStage() : nullptr;
    if (!currentStage) {
        return;
    }

    // Boarding is a world-space interaction. A boat can be deliberately
    // assigned to a different start planet in the editor while still placed
    // beside the player, so inspect every boat in the stage rather than only
    // the player's current planet.
    for (Planet* planet : currentStage->GetPlanets()) {
        if (!planet) {
            continue;
        }

        for (Boat* boat : planet->GetBoats()) {
            if (!boat || !boat->GetIsActive()) {
                continue;
            }

            // Nearby travelling boats must not pull in an unrelated player.
            if (boat->GetIsMoving()) {
                if (boat->HasBoardedPlayer(&player)) {
                    FollowMovingBoat(player, boat);
                    return;
                }
                continue;
            }

            if (IsTouchingBoat(player, boat)) {
                StartRidingBoat(player, boat);
                return;
            }
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

    Game* game = player.GetGame();
    boat->BoardPlayer(&player);
    // A boarded split slime waits inside the rocket.  The rocket launches
    // only once both split slimes have boarded, instead of merging them.
    player.SetIsActive(false);

    // In solo split play, boarding one half must immediately hand control
    // to the remaining half after a short boarding beat.  This keeps the
    // player controllable without a manual switch while the first half waits
    // in the rocket.
    if (game && game->GetIsPlayerSplit() && !game->GetIsPlayer2Joined()) {
        game->RequestSoloSplitControlSwitchAfterBoarding();
    }

    const bool waitsForBothPlayers =
        game && (game->GetIsPlayerSplit() || game->GetIsPlayer2Joined());
    if (waitsForBothPlayers) {
        for (Player* passenger : game->GetPlayers()) {
            if (!boat->HasBoardedPlayer(passenger)) {
                return;
            }
        }
    }

    boat->StartTravel();
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
    // Store only the tangent-facing direction.  The up direction is always
    // recalculated from the destination planet on respawn, avoiding stale
    // quaternions or camera vectors from the planet the player departed.
    respawn.CaptureRestartFacingDirection(player);
}
