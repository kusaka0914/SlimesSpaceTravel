#pragma once

#include <vector>

class Planet;

class Stage {
public:
    void AddPlanet(Planet* planet) { mPlanets.emplace_back(planet); }
    void RemoveAllPlanet() { mPlanets.clear(); }

    const std::vector<Planet*>& GetPlanets() const { return mPlanets; }

private:
    std::vector<Planet*> mPlanets;
};