#pragma once

#include <vector>

class Planet;

// ステージに所属するPlanetの参照一覧を保持する。Planetの所有権と寿命はGameWorldが管理する。
class Stage {
public:
    // 所有権を移さず一覧へ追加する。重複やnullptrの検証は呼び出し元が行う。
    void AddPlanet(Planet* planet) { mPlanets.emplace_back(planet); }

    // 登録した参照だけを解除し、Planet自体は削除しない。
    void RemoveAllPlanet() { mPlanets.clear(); }

    const std::vector<Planet*>& GetPlanets() const { return mPlanets; }

private:
    std::vector<Planet*> mPlanets;
};
