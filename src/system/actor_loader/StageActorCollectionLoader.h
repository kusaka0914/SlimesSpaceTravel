#pragma once

#include <yaml-cpp/yaml.h>

class Game;
class StageActorCreationService;
class StageActorFactory;
class StagePlayerLoader;

class StageActorCollectionLoader {
public:
    StageActorCollectionLoader(
        Game* game,
        StageActorFactory& actorFactory,
        StageActorCreationService& creationService,
        StagePlayerLoader& playerLoader);

    bool LoadAll();

private:
    void LoadPlayers(const YAML::Node& stageRoot);
    void LoadNPCs(const YAML::Node& stageRoot);
    void LoadEnemies(const YAML::Node& stageRoot);
    void LoadPlanets(const YAML::Node& stageRoot);
    void LoadBoats(const YAML::Node& stageRoot);
    void LoadBoatParts(const YAML::Node& stageRoot);
    void LoadKeys(const YAML::Node& stageRoot);
    void LoadCrystals(const YAML::Node& stageRoot);
    void LoadStar(const YAML::Node& stageRoot);
    void LoadPlatforms(const YAML::Node& stageRoot);
    void LoadLegacyMovingPlatforms(const YAML::Node& stageRoot);
    void LoadBoatArrivalPoints(const YAML::Node& stageRoot);
    void LoadFallRespawnPoints(const YAML::Node& stageRoot);
    void LoadStageObjects(const YAML::Node& stageRoot);
    void LoadTutorialTriggers(const YAML::Node& stageRoot);
    void LoadJewelItems(const YAML::Node& stageRoot);
    void LoadHazardActors(const YAML::Node& stageRoot);

private:
    Game* mGame = nullptr;
    StageActorFactory& mActorFactory;
    StageActorCreationService& mCreationService;
    StagePlayerLoader& mPlayerLoader;
};
