#pragma once

#include <algorithm>
#include <string>
#include <vector>

class Boat;
class BoatArrivalPoint;
class BoatParts;
class Crystal;
class Enemy;
class FallRespawnPoint;
class Key;
class NPC;
class Platform;
class Star;
class StageObject;

class PlanetActorRegistry {
public:
    void AddEnemy(Enemy* enemy) { mEnemies.emplace_back(enemy); }
    void AddBoat(Boat* boat) { mBoats.emplace_back(boat); }
    void AddBoatParts(BoatParts* boatParts) { mBoatParts.emplace_back(boatParts); }
    void AddCrystal(Crystal* crystal) { mCrystals.emplace_back(crystal); }
    void AddNPC(NPC* npc) { mNPCs.emplace_back(npc); }
    void AddPlatform(Platform* platform) { mPlatforms.emplace_back(platform); }
    void AddBoatArrivalPoint(BoatArrivalPoint* point) { mBoatArrivalPoints.emplace_back(point); }
    void AddFallRespawnPoint(FallRespawnPoint* point) { mFallRespawnPoints.emplace_back(point); }
    void AddStageObject(StageObject* stageObject) { mStageObjects.emplace_back(stageObject); }

    void RemoveAllEnemy() { mEnemies.clear(); }
    void RemoveAllBoat() { mBoats.clear(); }
    void RemoveBoat(Boat* boat)
    {
        mBoats.erase(
            std::remove(mBoats.begin(), mBoats.end(), boat),
            mBoats.end());
    }
    void RemoveAllBoatParts() { mBoatParts.clear(); }
    void RemoveAllCrystals() { mCrystals.clear(); }
    void RemoveAllNPCs() { mNPCs.clear(); }
    void RemoveAllPlatforms();
    void RemovePlatformsByStageSequence(const std::string& sequenceName);
    void RemoveKey() { mKey = nullptr; }
    void RemoveStar() { mStar = nullptr; }
    void RemoveAllBoatArrivalPoints() { mBoatArrivalPoints.clear(); }
    void RemoveAllFallRespawnPoints() { mFallRespawnPoints.clear(); }
    void RemoveAllStageObjects() { mStageObjects.clear(); }

    void SetKey(Key* key) { mKey = key; }
    void SetStar(Star* star) { mStar = star; }

    const std::vector<Enemy*>& GetEnemies() const { return mEnemies; }
    const std::vector<Boat*>& GetBoats() const { return mBoats; }
    const std::vector<BoatParts*>& GetBoatParts() const { return mBoatParts; }
    const std::vector<Crystal*>& GetCrystals() const { return mCrystals; }
    const std::vector<NPC*>& GetNPCs() const { return mNPCs; }
    const std::vector<Platform*>& GetPlatforms() const { return mPlatforms; }
    const std::vector<BoatArrivalPoint*>& GetBoatArrivalPoints() const { return mBoatArrivalPoints; }
    const std::vector<FallRespawnPoint*>& GetFallRespawnPoints() const { return mFallRespawnPoints; }
    const std::vector<StageObject*>& GetStageObjects() const { return mStageObjects; }

    Key* GetKey() const { return mKey; }
    Star* GetStar() const { return mStar; }

private:
    std::vector<Enemy*> mEnemies;
    std::vector<Boat*> mBoats;
    std::vector<BoatParts*> mBoatParts;
    std::vector<Crystal*> mCrystals;
    std::vector<NPC*> mNPCs;
    std::vector<Platform*> mPlatforms;
    std::vector<BoatArrivalPoint*> mBoatArrivalPoints;
    std::vector<FallRespawnPoint*> mFallRespawnPoints;
    std::vector<StageObject*> mStageObjects;

    Key* mKey = nullptr;
    Star* mStar = nullptr;
};
