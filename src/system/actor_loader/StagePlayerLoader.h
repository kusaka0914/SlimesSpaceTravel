#pragma once

#include "actor/player/PlayerConfig.h"

#include <yaml-cpp/yaml.h>

class ActorPlacementLoader;
class Game;
class Player;

// Player設定と配置を読み込み、Gameが所有するPlayerを生成して参照一覧へ登録する。
class StagePlayerLoader {
public:
    StagePlayerLoader(Game* game, const ActorPlacementLoader& placementLoader);

    void SetPlayerConfig(PlayerConfig config);

    // Player参照一覧を作り直す。不足時は先頭定義から補充し、分裂用を含む2体の生成を試みる。
    void LoadPlayers(const YAML::Node& stageRoot);

    // 1始まりのPlayer番号で生成する。Stage・Planetを解決できなければnullptr。戻り値はGame所有の借用参照。
    Player* CreatePlayerFromStageNode(const YAML::Node& node, int playerNum);

    // 現在のYAMLから1始まりのPlayer番号に対応する定義を生成する。trueは定義の存在を表し、生成成功は保証しない。
    bool CreatePlayerFromCurrentStage(int playerNum);

private:
    Game* mGame = nullptr;
    const ActorPlacementLoader& mPlacementLoader;
    PlayerConfig mPlayerConfig;
};
