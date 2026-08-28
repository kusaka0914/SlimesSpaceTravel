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

class ActorLoadSystem {
public:
    explicit ActorLoadSystem(Game* game);

    void LoadData();

    StageActorCreationService& GetCreationService();
    StagePlayerLoader& GetPlayerLoader();
    StageActorLocator& GetActorLocator();

private:
    ActorPlacementLoader mPlacementLoader;
    StagePlayerLoader mPlayerLoader;
    StageBoatCreator mBoatCreator;
    StagePlanetCreator mPlanetCreator;
    StageActorFactory mActorFactory;
    StageActorCreationService mCreationService;
    StageActorCollectionLoader mCollectionLoader;
    StageActorLocator mActorLocator;
};
