#pragma once

#include "app/Game.h"
#include "editor/stage/selection/StageSelectionController.h"
#include "world/Stage.h"

#include <initializer_list>
#include <vector>

inline StageSelectionScene MakeTestStageSelectionScene(Game& game)
{
    return {game.GetCurrentStage(),    game.GetPhysicsSystem(),         game.GetCameraSystem(),
            game.GetWindow(),          game.GetCurrentStageYamlPath(),  game.GetIsUGCMode(),
            game.GetIsPlayer2Joined(), game.GetIsUGCOrthographicView(), game.GetUGCOrthographicHalfHeight(),
            game.GetUGCGridSize()};
}

// 検証中だけ惑星の借用一覧を置換し、例外時も元へ戻す。渡す惑星はこのガードより長く保持する。
class ScopedTestStagePlanets {
public:
    ScopedTestStagePlanets(Stage& stage, std::initializer_list<Planet*> planets)
        : mStage(stage),
          mOriginal(stage.GetPlanets())
    {
        mStage.RemoveAllPlanet();
        for (Planet* planet : planets) {
            mStage.AddPlanet(planet);
        }
    }
    ~ScopedTestStagePlanets()
    {
        mStage.RemoveAllPlanet();
        for (Planet* planet : mOriginal) {
            mStage.AddPlanet(planet);
        }
    }
    ScopedTestStagePlanets(const ScopedTestStagePlanets&) = delete;
    ScopedTestStagePlanets& operator=(const ScopedTestStagePlanets&) = delete;

private:
    Stage& mStage;
    std::vector<Planet*> mOriginal;
};
