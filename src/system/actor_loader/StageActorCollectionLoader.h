#pragma once

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

    void LoadAll();

private:
    void LoadPlayers(const char* path);
    void LoadNPCs(const char* path);
    void LoadEnemies(const char* path);
    void LoadPlanets(const char* path);
    void LoadBoats(const char* path);
    void LoadBoatParts(const char* path);
    void LoadKeys(const char* path);
    void LoadCrystals(const char* path);
    void LoadStar(const char* path);
    void LoadPlatforms(const char* path);
    void LoadLegacyMovingPlatforms(const char* path);
    void LoadBoatArrivalPoints(const char* path);
    void LoadFallRespawnPoints(const char* path);
    void LoadStageObjects(const char* path);
    void LoadTutorialTriggers(const char* path);
    void LoadJewelItems(const char* path);
    void LoadHazardActors(const char* path);

private:
    Game* mGame = nullptr;
    StageActorFactory& mActorFactory;
    StageActorCreationService& mCreationService;
    StagePlayerLoader& mPlayerLoader;
};
