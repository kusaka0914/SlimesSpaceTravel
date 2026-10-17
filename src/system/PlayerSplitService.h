#pragma once

#include <vector>

class Player;

// 既存Player間の分裂・合体に伴う配置、操作状態、共有リソースの同期を行う。
class PlayerSplitService {
public:
    // 先頭2人を分裂形態にし、2人目の配置・速度を同期して有効化する。2人揃わなければfalse。
    static bool ActivateSplit(const std::vector<Player*>& players);

    // sourcePlayerIndex（0か1）の配置を引き継いで先頭へ合体し、2人目を無効化する。
    // Player不足・不正インデックスの場合はfalse。Actorは破棄しない。
    static bool MergeIntoMainPlayer(const std::vector<Player*>& players, int sourcePlayerIndex);
    static bool ArePlayersCloseEnoughToMerge(const std::vector<Player*>& players, float maximumDistance);
    static void SynchronizeSecondPlayerAfterStageReload(const std::vector<Player*>& players);

    // sourcePlayerのHP・最大HP・ジュエル数を他のPlayerへ反映する。sourcePlayerは先頭2人のいずれかであること。
    static void SynchronizeSharedResources(const std::vector<Player*>& players, const Player& sourcePlayer);

private:
    static void CopyPlacementAndMovement(const Player& sourcePlayer, Player& destinationPlayer,
                                         bool shouldCopyVelocity);
};
