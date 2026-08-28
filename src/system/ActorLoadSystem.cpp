#include "ActorLoadSystem.h"

ActorLoadSystem::ActorLoadSystem(Game* game)
    : mPlayerLoader(game, mPlacementLoader),
      mBoatCreator(game, mPlacementLoader),
      mPlanetCreator(game),
      mActorFactory(game, mPlacementLoader),
      mCreationService(
          mActorFactory,
          mPlayerLoader,
          mBoatCreator,
          mPlanetCreator),
      mCollectionLoader(
          game,
          mActorFactory,
          mCreationService,
          mPlayerLoader)
{
}

void ActorLoadSystem::LoadData()
{
    mCollectionLoader.LoadAll();
}

StageActorCreationService& ActorLoadSystem::GetCreationService()
{
    return mCreationService;
}

StagePlayerLoader& ActorLoadSystem::GetPlayerLoader()
{
    return mPlayerLoader;
}

StageActorLocator& ActorLoadSystem::GetActorLocator()
{
    return mActorLocator;
}
