#pragma once

#include "system/actor_loader/ActorPlacementLoader.h"
#include "system/actor_loader/StageActorCollectionLoader.h"
#include "system/actor_loader/StageActorCreationService.h"
#include "system/actor_loader/StageActorFactory.h"
#include "system/actor_loader/StageActorLocator.h"
#include "system/actor_loader/StageBoatCreator.h"
#include "system/actor_loader/StagePlanetCreator.h"
#include "system/actor_loader/StagePlayerLoader.h"

class Game;
class GameWorld;

// StageとActorの読込をまとめ、成功した実行時状態だけをGameWorldへ反映する。
class ActorLoadSystem {
public:
    ActorLoadSystem(Game* game, GameWorld& world);

    // 成功時は既存のStage・Actorを置き換える。失敗時は以前の実行時状態へ戻してfalseを返す。
    // 物理ワールドの再構築と、置換で無効になるActor参照の整理は呼び出し側が行う。
    bool LoadData();

    StageActorCreationService& GetCreationService();
    StagePlayerLoader& GetPlayerLoader();
    StageActorLocator& GetActorLocator();

private:
    GameWorld& mWorld;
    ActorPlacementLoader mPlacementLoader;
    StagePlayerLoader mPlayerLoader;
    StageBoatCreator mBoatCreator;
    StagePlanetCreator mPlanetCreator;
    StageActorFactory mActorFactory;
    StageActorCreationService mCreationService;
    StageActorCollectionLoader mCollectionLoader;
    StageActorLocator mActorLocator;
};
