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





    // ボートは編集時にプレイヤーと別の開始惑星へ割り当てられるため、現在惑星だけでなくステージ内の全ボートを調べる。
    for (Planet* planet : currentStage->GetPlanets()) {
        if (!planet) {
            continue;
        }

        for (Boat* boat : planet->GetBoats()) {
            if (!boat || !boat->GetIsActive()) {
                continue;
            }


            // 移動中の近くのボートが、搭乗していないプレイヤーを引き寄せないようにする。
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


    // 分裂体はロケット内で待機し、両方が搭乗してから発進する。搭乗時には合体しない。
    player.SetIsActive(false);





    // 1人分裂中は搭乗した側を非選択にし、残った側へ自動で操作を渡す。
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



    // リスタートには接線方向だけを保存する。上方向は到着先惑星から再計算し、出発元の姿勢を持ち込まない。
    respawn.CaptureRestartFacingDirection(player);
}
