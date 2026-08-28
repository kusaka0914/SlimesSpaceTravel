#include "StageActorCollectionLoader.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Boat.h"
#include "actor/BoatArrivalPoint.h"
#include "actor/BoatParts.h"
#include "actor/Crystal.h"
#include "actor/Enemy.h"
#include "actor/FallRespawnPoint.h"
#include "actor/HazardActor.h"
#include "actor/JewelItem.h"
#include "actor/Key.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/StageObject.h"
#include "actor/Star.h"
#include "actor/TutorialTrigger.h"
#include "system/StageActorPlanetBindingService.h"
#include "system/actor_loader/StageActorCreationService.h"
#include "system/actor_loader/StageActorFactory.h"
#include "system/actor_loader/StagePlayerLoader.h"

#include <string>
#include <yaml-cpp/yaml.h>

StageActorCollectionLoader::StageActorCollectionLoader(
    Game* game,
    StageActorFactory& actorFactory,
    StageActorCreationService& creationService,
    StagePlayerLoader& playerLoader)
    : mGame(game),
      mActorFactory(actorFactory),
      mCreationService(creationService),
      mPlayerLoader(playerLoader)
{
}

void StageActorCollectionLoader::LoadAll()
{
    if (!mGame) {
        return;
    }

    const std::string& path = mGame->GetCurrentStageYamlPath();

    LoadPlanets(path.c_str());
    LoadEnemies(path.c_str());
    LoadBoatArrivalPoints(path.c_str());
    LoadBoats(path.c_str());
    LoadBoatParts(path.c_str());
    LoadKeys(path.c_str());
    LoadCrystals(path.c_str());
    LoadStar(path.c_str());
    LoadNPCs(path.c_str());
    LoadTutorialTriggers(path.c_str());
    LoadJewelItems(path.c_str());
    LoadHazardActors(path.c_str());
    LoadPlatforms(path.c_str());
    LoadLegacyMovingPlatforms(path.c_str());
    LoadStageObjects(path.c_str());
    LoadFallRespawnPoints(path.c_str());
    LoadPlayers(path.c_str());

    StageActorPlanetBindingService::RefreshNearestPlanetBindings(
        mGame->GetCurrentStage());
}

void StageActorCollectionLoader::LoadPlayers(const char* path)
{
    mPlayerLoader.LoadPlayersFromFile(path);
}

void StageActorCollectionLoader::LoadNPCs(const char* path)
{
    mActorFactory.LoadActorSequence<NPC>(
        path, "NPCs", [](Planet* planet) { planet->RemoveAllNPCs(); },
        [this](const YAML::Node& node, int index) { return mCreationService.CreateNPC(node, index); });
}

void StageActorCollectionLoader::LoadTutorialTriggers(const char* path)
{
    mActorFactory.LoadActorSequence<TutorialTrigger>(
        path,
        "tutorialTriggers",
        [](Planet* planet) { planet->RemoveAllTutorialTriggers(); },
        [this](const YAML::Node& node, int index) {
            return mCreationService.CreateTutorialTrigger(node, index);
        });
}

void StageActorCollectionLoader::LoadEnemies(const char* path)
{
    mActorFactory.LoadActorSequence<Enemy>(
        path, "enemies", [](Planet* planet) { planet->RemoveAllEnemy(); },
        [this](const YAML::Node& node, int index) { return mCreationService.CreateEnemy(node, index); });
}

void StageActorCollectionLoader::LoadPlanets(const char* path)
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return;
    }

    mGame->GetCurrentStage()->RemoveAllPlanet();

    YAML::Node root = YAML::LoadFile(path);

    if (!root["planets"] || !root["planets"].IsSequence()) {
        return;
    }

    for (const YAML::Node& node : root["planets"]) {
        mCreationService.CreatePlanet(node);
    }
}

void StageActorCollectionLoader::LoadBoats(const char* path)
{
    mActorFactory.LoadActorSequence<Boat>(
        path, "boats", [](Planet* planet) { planet->RemoveAllBoat(); },
        [this](const YAML::Node& node, int index) { return mCreationService.CreateBoat(node, index); });
}

void StageActorCollectionLoader::LoadBoatParts(const char* path)
{
    mActorFactory.LoadActorSequence<BoatParts>(
        path, "boatParts", [](Planet* planet) { planet->RemoveAllBoatParts(); },
        [this](const YAML::Node& node, int index) { return mCreationService.CreateBoatParts(node, index); });
}

void StageActorCollectionLoader::LoadKeys(const char* path)
{
    mActorFactory.LoadActorSequence<Key>(
        path, "keys", [](Planet* planet) { planet->RemoveKey(); },
        [this](const YAML::Node& node, int index) { return mCreationService.CreateKey(node, index); });
}

void StageActorCollectionLoader::LoadCrystals(const char* path)
{
    mActorFactory.LoadActorSequence<Crystal>(
        path, "crystals", [](Planet* planet) { planet->RemoveAllCrystals(); },
        [this](const YAML::Node& node, int index) { return mCreationService.CreateCrystal(node, index); });
}

void StageActorCollectionLoader::LoadStar(const char* path)
{
    mActorFactory.LoadActorSequence<Star>(
        path, "star", [](Planet* planet) { planet->RemoveStar(); },
        [this](const YAML::Node& node, int index) { return mCreationService.CreateStar(node, index); });
}

void StageActorCollectionLoader::LoadPlatforms(const char* path)
{
    mActorFactory.LoadActorSequence<Platform>(
        path, "platforms", [](Planet* planet) { planet->RemoveAllPlatforms(); },
        [this](const YAML::Node& node, int index) { return mCreationService.CreatePlatform(node, index); });
}

void StageActorCollectionLoader::LoadLegacyMovingPlatforms(const char* path)
{
    mActorFactory.LoadActorSequence<Platform>(
        path, "movingPlatforms",
        [](Planet* planet) {
            planet->RemovePlatformsByStageSequence("movingPlatforms");
        },
        [this](const YAML::Node& node, int index) {
            return mCreationService.CreateLegacyMovingPlatform(node, index);
        });
}

void StageActorCollectionLoader::LoadBoatArrivalPoints(const char* path)
{
    mActorFactory.LoadActorSequence<BoatArrivalPoint>(
        path, "boatArrivalPoints", [](Planet* planet) { planet->RemoveAllBoatArrivalPoints(); },
        [this](const YAML::Node& node, int index) { return mCreationService.CreateBoatArrivalPoint(node, index); });
}

void StageActorCollectionLoader::LoadJewelItems(const char* path)
{
    mActorFactory.LoadActorSequence<JewelItem>(
        path,
        "jewelItems",
        [](Planet* planet) { planet->RemoveAllJewelItems(); },
        [this](const YAML::Node& node, int index) {
            return mCreationService.CreateJewelItem(node, index);
        });
}

void StageActorCollectionLoader::LoadHazardActors(const char* path)
{
    mActorFactory.LoadActorSequence<HazardActor>(
        path,
        "hazardActors",
        [](Planet* planet) { planet->RemoveAllHazardActors(); },
        [this](const YAML::Node& node, int index) {
            return mCreationService.CreateHazardActor(node, index);
        });
}

void StageActorCollectionLoader::LoadFallRespawnPoints(const char* path)
{
    mActorFactory.LoadActorSequence<FallRespawnPoint>(
        path, "fallRespawnPoints", [](Planet* planet) { planet->RemoveAllFallRespawnPoints(); },
        [this](const YAML::Node& node, int index) { return mCreationService.CreateFallRespawnPoint(node, index); });
}

void StageActorCollectionLoader::LoadStageObjects(const char* path)
{
    mActorFactory.LoadActorSequence<StageObject>(
        path, "stageObjects", [](Planet* planet) { planet->RemoveAllStageObjects(); },
        [this](const YAML::Node& node, int index) { return mCreationService.CreateStageObject(node, index); });
}

