#include "ActorLoadSystem.h"

#include "system/GameWorld.h"

#include <exception>
#include <iostream>

ActorLoadSystem::ActorLoadSystem(Game* game, GameWorld& world)
    : mWorld(world),
      mPlayerLoader(game, mPlacementLoader),
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

bool ActorLoadSystem::LoadData()
{
    const int stageCount = static_cast<int>(mWorld.GetStages().size());
    const int currentStageNum = mWorld.GetCurrentStageNum();
    if (stageCount == 0) {
        std::cerr << "Cannot load stage actors before stages are created.\n";
        return false;
    }

    GameWorld previousWorld;
    mWorld.SwapRuntimeState(previousWorld);
    mWorld.CreateStages(stageCount);
    if (!mWorld.ChangeStage(currentStageNum)) {
        mWorld.SwapRuntimeState(previousWorld);
        return false;
    }

    try {
        if (!mCollectionLoader.LoadAll()) {
            mWorld.SwapRuntimeState(previousWorld);
            return false;
        }
    } catch (const std::exception& exception) {
        std::cerr << "Failed to build stage actors: "
                  << exception.what() << '\n';
        mWorld.SwapRuntimeState(previousWorld);
        return false;
    } catch (...) {
        std::cerr << "Failed to build stage actors due to an unknown error.\n";
        mWorld.SwapRuntimeState(previousWorld);
        return false;
    }

    return true;
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
