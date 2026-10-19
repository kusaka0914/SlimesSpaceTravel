#pragma once

#include "world/Stage.h"

#include <initializer_list>
#include <vector>

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
