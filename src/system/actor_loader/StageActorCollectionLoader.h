#pragma once

#include <yaml-cpp/yaml.h>

class Game;
class StageActorCreationService;
class StageActorFactory;
class StagePlayerLoader;

// ステージYAMLを読み込み、依存順に各種Actorを生成・登録する。依存先は借用する。
class StageActorCollectionLoader {
public:
    StageActorCollectionLoader(Game* game, StageActorFactory& actorFactory, StageActorCreationService& creationService,
                               StagePlayerLoader& playerLoader);

    // 既定設定とステージYAMLを読み込み、Actorを生成する。読込失敗はfalse。生成中の例外は呼び出し側へ伝わる。
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

    Game* mGame = nullptr;
    StageActorFactory& mActorFactory;
    StageActorCreationService& mCreationService;
    StagePlayerLoader& mPlayerLoader;
};
