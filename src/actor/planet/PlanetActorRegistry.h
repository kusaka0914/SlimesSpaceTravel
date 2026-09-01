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
class HazardActor;
class Key;
class JewelItem;
class NPC;
class Platform;
class Star;
class StageObject;
class TutorialTrigger;

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
    void AddTutorialTrigger(TutorialTrigger* trigger) { mTutorialTriggers.emplace_back(trigger); }
    void AddJewelItem(JewelItem* jewelItem) { mJewelItems.emplace_back(jewelItem); }
    void AddHazardActor(HazardActor* hazardActor) { mHazardActors.emplace_back(hazardActor); }

    void RemoveAllEnemy() { mEnemies.clear(); }
    void RemoveEnemy(Enemy* enemy) { RemovePointer(mEnemies, enemy); }
    void RemoveAllBoat() { mBoats.clear(); }
    void RemoveBoat(Boat* boat) { RemovePointer(mBoats, boat); }
    void RemoveAllBoatParts() { mBoatParts.clear(); }
    void RemoveBoatParts(BoatParts* boatParts) { RemovePointer(mBoatParts, boatParts); }
    void RemoveAllCrystals() { mCrystals.clear(); }
    void RemoveCrystal(Crystal* crystal) { RemovePointer(mCrystals, crystal); }
    void RemoveAllNPCs() { mNPCs.clear(); }
    void RemoveNPC(NPC* npc) { RemovePointer(mNPCs, npc); }
    void RemoveAllPlatforms();
    void RemovePlatform(Platform* platform) { RemovePointer(mPlatforms, platform); }
    void RemovePlatformsByStageSequence(const std::string& sequenceName);
    void RemoveKey() { mKey = nullptr; }
    void RemoveStar() { mStar = nullptr; }
    void RemoveAllBoatArrivalPoints() { mBoatArrivalPoints.clear(); }
    void RemoveBoatArrivalPoint(BoatArrivalPoint* point) { RemovePointer(mBoatArrivalPoints, point); }
    void RemoveAllFallRespawnPoints() { mFallRespawnPoints.clear(); }
    void RemoveFallRespawnPoint(FallRespawnPoint* point) { RemovePointer(mFallRespawnPoints, point); }
    void RemoveAllStageObjects() { mStageObjects.clear(); }
    void RemoveStageObject(StageObject* stageObject) { RemovePointer(mStageObjects, stageObject); }
    void RemoveAllTutorialTriggers() { mTutorialTriggers.clear(); }
    void RemoveTutorialTrigger(TutorialTrigger* trigger) { RemovePointer(mTutorialTriggers, trigger); }
    void RemoveAllJewelItems() { mJewelItems.clear(); }
    void RemoveJewelItem(JewelItem* jewelItem) { RemovePointer(mJewelItems, jewelItem); }
    void RemoveAllHazardActors() { mHazardActors.clear(); }
    void RemoveHazardActor(HazardActor* hazardActor) { RemovePointer(mHazardActors, hazardActor); }

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
    const std::vector<TutorialTrigger*>& GetTutorialTriggers() const { return mTutorialTriggers; }
    const std::vector<JewelItem*>& GetJewelItems() const { return mJewelItems; }
    const std::vector<HazardActor*>& GetHazardActors() const { return mHazardActors; }

    Key* GetKey() const { return mKey; }
    Star* GetStar() const { return mStar; }

private:
    template <class TActor>
    static void RemovePointer(std::vector<TActor*>& actors, TActor* actor)
    {
        actors.erase(
            std::remove(actors.begin(), actors.end(), actor),
            actors.end());
    }

    std::vector<Enemy*> mEnemies;
    std::vector<Boat*> mBoats;
    std::vector<BoatParts*> mBoatParts;
    std::vector<Crystal*> mCrystals;
    std::vector<NPC*> mNPCs;
    std::vector<Platform*> mPlatforms;
    std::vector<BoatArrivalPoint*> mBoatArrivalPoints;
    std::vector<FallRespawnPoint*> mFallRespawnPoints;
    std::vector<StageObject*> mStageObjects;
    std::vector<TutorialTrigger*> mTutorialTriggers;
    std::vector<JewelItem*> mJewelItems;
    std::vector<HazardActor*> mHazardActors;

    Key* mKey = nullptr;
    Star* mStar = nullptr;
};
