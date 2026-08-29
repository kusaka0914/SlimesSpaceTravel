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

class ActorLoadSystem {
public:
    ActorLoadSystem(Game* game, GameWorld& world);

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
