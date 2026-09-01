#pragma once

#include <vector>

class Player;

class PlayerSplitService {
public:
    static bool ActivateSplit(const std::vector<Player*>& players);
    static bool MergeIntoMainPlayer(
        const std::vector<Player*>& players,
        int sourcePlayerIndex);
    static bool ArePlayersCloseEnoughToMerge(
        const std::vector<Player*>& players,
        float maximumDistance);
    static void SynchronizeSecondPlayerAfterStageReload(
        const std::vector<Player*>& players);
    static void SynchronizeSharedResources(
        const std::vector<Player*>& players,
        const Player& sourcePlayer);

private:
    static void CopyPlacementAndMovement(
        const Player& sourcePlayer,
        Player& destinationPlayer,
        bool shouldCopyVelocity);
};
