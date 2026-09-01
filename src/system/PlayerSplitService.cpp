#include "system/PlayerSplitService.h"

#include "actor/Player.h"

#include <glm/glm.hpp>

bool PlayerSplitService::ActivateSplit(
    const std::vector<Player*>& players)
{
    if (players.size() < 2 || !players[0] || !players[1]) {
        return false;
    }

    Player& mainPlayer = *players[0];
    Player& splitPlayer = *players[1];
    mainPlayer.SetSplitForm(true);
    splitPlayer.SetSplitForm(true);
    CopyPlacementAndMovement(mainPlayer, splitPlayer, true);
    splitPlayer.SetShouldJudgeLanding(!mainPlayer.GetOnGround());
    splitPlayer.SetControlLocked(false);
    splitPlayer.SetIsActive(true);
    return true;
}

bool PlayerSplitService::MergeIntoMainPlayer(
    const std::vector<Player*>& players,
    int sourcePlayerIndex)
{
    if (players.size() < 2 ||
        sourcePlayerIndex < 0 ||
        sourcePlayerIndex >= 2 ||
        !players[0] ||
        !players[1]) {
        return false;
    }

    Player& mainPlayer = *players[0];
    Player& splitPlayer = *players[1];
    if (sourcePlayerIndex == 1) {
        CopyPlacementAndMovement(splitPlayer, mainPlayer, true);
        mainPlayer.SetShouldJudgeLanding(true);
    }

    mainPlayer.SetSplitForm(false);
    splitPlayer.SetSplitForm(false);
    splitPlayer.SetVelocity(glm::vec3(0.0f));
    splitPlayer.SetControlLocked(true);
    splitPlayer.SetIsActive(false);
    return true;
}

bool PlayerSplitService::ArePlayersCloseEnoughToMerge(
    const std::vector<Player*>& players,
    float maximumDistance)
{
    if (players.size() < 2 || !players[0] || !players[1] ||
        !players[0]->GetIsActive() || !players[1]->GetIsActive()) {
        return false;
    }

    const float playerDistance =
        glm::length(players[0]->GetPos() - players[1]->GetPos());
    return playerDistance <= maximumDistance;
}

void PlayerSplitService::SynchronizeSecondPlayerAfterStageReload(
    const std::vector<Player*>& players)
{
    if (players.size() < 2 || !players[0] || !players[1]) {
        return;
    }

    CopyPlacementAndMovement(*players[0], *players[1], false);
    players[1]->SetVelocity(glm::vec3(0.0f));
    players[1]->SetShouldJudgeLanding(!players[0]->GetOnGround());
}

void PlayerSplitService::SynchronizeSharedResources(
    const std::vector<Player*>& players,
    const Player& sourcePlayer)
{
    if (players.size() < 2 ||
        (players[0] != &sourcePlayer && players[1] != &sourcePlayer)) {
        return;
    }

    for (Player* player : players) {
        if (!player || player == &sourcePlayer) {
            continue;
        }

        if (player->GetMaxHp() != sourcePlayer.GetMaxHp()) {
            player->SetMaxHp(sourcePlayer.GetMaxHp());
        }
        if (player->GetHp() != sourcePlayer.GetHp()) {
            player->SetHp(sourcePlayer.GetHp());
        }
        if (player->GetJewelCount() != sourcePlayer.GetJewelCount()) {
            player->SetJewelCount(sourcePlayer.GetJewelCount());
        }
    }
}

void PlayerSplitService::CopyPlacementAndMovement(
    const Player& sourcePlayer,
    Player& destinationPlayer,
    bool shouldCopyVelocity)
{
    destinationPlayer.SetCurrentPlanet(sourcePlayer.GetCurrentPlanet());
    destinationPlayer.SetCurrentPlanetNum(sourcePlayer.GetCurrentPlanetNum());
    destinationPlayer.SetSphericalPlacement(
        sourcePlayer.GetTheta(),
        sourcePlayer.GetPhi(),
        sourcePlayer.GetHeight());
    destinationPlayer.SetOrientation(sourcePlayer.GetOrientation());
    destinationPlayer.SetFacingForwardVec(sourcePlayer.GetFacingForwardVec());
    destinationPlayer.SetCameraForwardDirection(
        -sourcePlayer.GetFacingForwardVec(),
        sourcePlayer.GetUpVec());
    destinationPlayer.SetCameraYaw(sourcePlayer.GetCameraYaw());
    destinationPlayer.SetPos(sourcePlayer.GetPos());
    if (shouldCopyVelocity) {
        destinationPlayer.SetVelocity(sourcePlayer.GetVelocity());
    }
    destinationPlayer.SetOnGround(sourcePlayer.GetOnGround());
    destinationPlayer.RefreshFallbackUpVec();
}
