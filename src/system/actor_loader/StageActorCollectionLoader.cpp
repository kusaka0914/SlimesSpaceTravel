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
#include <array>
#include <iostream>
#include <yaml-cpp/yaml.h>

namespace {
bool HasValidStageCollections(const YAML::Node& stageRoot)
{
    if (!stageRoot || !stageRoot.IsMap() ||
        !stageRoot["planets"] ||
        !stageRoot["planets"].IsSequence() ||
        stageRoot["planets"].size() == 0) {
        return false;
    }

    constexpr std::array<const char*, 17> collectionNames = {
        "planets", "players", "NPCs", "enemies", "boats",
        "boatParts", "keys", "crystals", "star", "platforms",
        "movingPlatforms", "boatArrivalPoints", "fallRespawnPoints",
        "stageObjects", "tutorialTriggers", "jewelItems", "hazardActors",
    };
    for (const char* collectionName : collectionNames) {
        const YAML::Node collection = stageRoot[collectionName];
        if (collection && !collection.IsSequence()) {
            return false;
        }
    }
    return true;
}
}

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

bool StageActorCollectionLoader::LoadAll()
{
    if (!mGame) {
        return false;
    }

    const std::string& path = mGame->GetCurrentStageYamlPath();
    YAML::Node stageRoot;
    try {
        stageRoot = YAML::LoadFile(path);
    } catch (const YAML::Exception& exception) {
        std::cerr << "Failed to load stage YAML: " << path << '\n'
                  << exception.what() << '\n';
        return false;
    }
    if (!HasValidStageCollections(stageRoot)) {
        std::cerr << "Stage YAML has an invalid collection structure: "
                  << path << '\n';
        return false;
    }
    if (!mCreationService.ReloadActorDefaultConfigs()) {
        return false;
    }

    LoadPlanets(stageRoot);
    LoadEnemies(stageRoot);
    LoadBoatArrivalPoints(stageRoot);
    LoadBoats(stageRoot);
    LoadBoatParts(stageRoot);
    LoadKeys(stageRoot);
    LoadCrystals(stageRoot);
    LoadStar(stageRoot);
    LoadNPCs(stageRoot);
    LoadTutorialTriggers(stageRoot);
    LoadJewelItems(stageRoot);
    LoadHazardActors(stageRoot);
    LoadPlatforms(stageRoot);
    LoadLegacyMovingPlatforms(stageRoot);
    LoadStageObjects(stageRoot);
    LoadFallRespawnPoints(stageRoot);
    LoadPlayers(stageRoot);

    StageActorPlanetBindingService::RefreshNearestPlanetBindings(
        mGame->GetCurrentStage());
    return true;
}

void StageActorCollectionLoader::LoadPlayers(const YAML::Node& stageRoot)
{
    mPlayerLoader.LoadPlayers(stageRoot);
}

void StageActorCollectionLoader::LoadNPCs(const YAML::Node& stageRoot)
{
    mActorFactory.LoadActorSequence<NPC>(
        stageRoot, "NPCs", [](Planet* planet) { planet->RemoveAllNPCs(); },
        [this](const YAML::Node& node, int index) { return mCreationService.CreateNPC(node, index); });
}

void StageActorCollectionLoader::LoadTutorialTriggers(const YAML::Node& stageRoot)
{
    mActorFactory.LoadActorSequence<TutorialTrigger>(
        stageRoot,
        "tutorialTriggers",
        [](Planet* planet) { planet->RemoveAllTutorialTriggers(); },
        [this](const YAML::Node& node, int index) {
            return mCreationService.CreateTutorialTrigger(node, index);
        });
}

void StageActorCollectionLoader::LoadEnemies(const YAML::Node& stageRoot)
{
    mActorFactory.LoadActorSequence<Enemy>(
        stageRoot, "enemies", [](Planet* planet) { planet->RemoveAllEnemy(); },
        [this](const YAML::Node& node, int index) { return mCreationService.CreateEnemy(node, index); });
}

void StageActorCollectionLoader::LoadPlanets(const YAML::Node& stageRoot)
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return;
    }

    mGame->GetCurrentStage()->RemoveAllPlanet();

    if (!stageRoot["planets"] || !stageRoot["planets"].IsSequence()) {
        return;
    }

    for (const YAML::Node& node : stageRoot["planets"]) {
        mCreationService.CreatePlanet(node);
    }
}

void StageActorCollectionLoader::LoadBoats(const YAML::Node& stageRoot)
{
    mActorFactory.LoadActorSequence<Boat>(
        stageRoot, "boats", [](Planet* planet) { planet->RemoveAllBoat(); },
        [this](const YAML::Node& node, int index) { return mCreationService.CreateBoat(node, index); });
}

void StageActorCollectionLoader::LoadBoatParts(const YAML::Node& stageRoot)
{
    mActorFactory.LoadActorSequence<BoatParts>(
        stageRoot, "boatParts", [](Planet* planet) { planet->RemoveAllBoatParts(); },
        [this](const YAML::Node& node, int index) { return mCreationService.CreateBoatParts(node, index); });
}

void StageActorCollectionLoader::LoadKeys(const YAML::Node& stageRoot)
{
    mActorFactory.LoadActorSequence<Key>(
        stageRoot, "keys", [](Planet* planet) { planet->RemoveKey(); },
        [this](const YAML::Node& node, int index) { return mCreationService.CreateKey(node, index); });
}

void StageActorCollectionLoader::LoadCrystals(const YAML::Node& stageRoot)
{
    mActorFactory.LoadActorSequence<Crystal>(
        stageRoot, "crystals", [](Planet* planet) { planet->RemoveAllCrystals(); },
        [this](const YAML::Node& node, int index) { return mCreationService.CreateCrystal(node, index); });
}

void StageActorCollectionLoader::LoadStar(const YAML::Node& stageRoot)
{
    mActorFactory.LoadActorSequence<Star>(
        stageRoot, "star", [](Planet* planet) { planet->RemoveStar(); },
        [this](const YAML::Node& node, int index) { return mCreationService.CreateStar(node, index); });
}

void StageActorCollectionLoader::LoadPlatforms(const YAML::Node& stageRoot)
{
    mActorFactory.LoadActorSequence<Platform>(
        stageRoot, "platforms", [](Planet* planet) { planet->RemoveAllPlatforms(); },
        [this](const YAML::Node& node, int index) { return mCreationService.CreatePlatform(node, index); });
}

void StageActorCollectionLoader::LoadLegacyMovingPlatforms(const YAML::Node& stageRoot)
{
    mActorFactory.LoadActorSequence<Platform>(
        stageRoot, "movingPlatforms",
        [](Planet* planet) {
            planet->RemovePlatformsByStageSequence("movingPlatforms");
        },
        [this](const YAML::Node& node, int index) {
            return mCreationService.CreateLegacyMovingPlatform(node, index);
        });
}

void StageActorCollectionLoader::LoadBoatArrivalPoints(const YAML::Node& stageRoot)
{
    mActorFactory.LoadActorSequence<BoatArrivalPoint>(
        stageRoot, "boatArrivalPoints", [](Planet* planet) { planet->RemoveAllBoatArrivalPoints(); },
        [this](const YAML::Node& node, int index) { return mCreationService.CreateBoatArrivalPoint(node, index); });
}

void StageActorCollectionLoader::LoadJewelItems(const YAML::Node& stageRoot)
{
    mActorFactory.LoadActorSequence<JewelItem>(
        stageRoot,
        "jewelItems",
        [](Planet* planet) { planet->RemoveAllJewelItems(); },
        [this](const YAML::Node& node, int index) {
            return mCreationService.CreateJewelItem(node, index);
        });
}

void StageActorCollectionLoader::LoadHazardActors(const YAML::Node& stageRoot)
{
    mActorFactory.LoadActorSequence<HazardActor>(
        stageRoot,
        "hazardActors",
        [](Planet* planet) { planet->RemoveAllHazardActors(); },
        [this](const YAML::Node& node, int index) {
            return mCreationService.CreateHazardActor(node, index);
        });
}

void StageActorCollectionLoader::LoadFallRespawnPoints(const YAML::Node& stageRoot)
{
    mActorFactory.LoadActorSequence<FallRespawnPoint>(
        stageRoot, "fallRespawnPoints", [](Planet* planet) { planet->RemoveAllFallRespawnPoints(); },
        [this](const YAML::Node& node, int index) { return mCreationService.CreateFallRespawnPoint(node, index); });
}

void StageActorCollectionLoader::LoadStageObjects(const YAML::Node& stageRoot)
{
    mActorFactory.LoadActorSequence<StageObject>(
        stageRoot, "stageObjects", [](Planet* planet) { planet->RemoveAllStageObjects(); },
        [this](const YAML::Node& node, int index) { return mCreationService.CreateStageObject(node, index); });
}
