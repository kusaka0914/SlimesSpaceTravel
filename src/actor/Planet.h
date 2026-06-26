#pragma once

#include "actor/Actor.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

class Game;
class Enemy;
class Boat;
class BoatParts;
class Crystal;
class NPC;
class Platform;
class MovingPlatform;
class Stage;
class Key;
class Star;
class BoatArrivalPoint;
class FallRespawnPoint;

class Planet : public Actor {
public:
    enum class RocketSpawnCondition { AllEnemiesDead, AllBoatPartsCollected, None };

    enum class PlanetShape { Normal, Sphere, Ellipse };

    Planet(Game* game);
    void ApplyConfig(const YAML::Node& node);
    void Initialize() override;
    void OnBoatPartsObtained();
    void OnEnemyDead();

    glm::vec3 CalculateSurfacePos(float theta, float phi, float height) const;

    void AddEnemy(Enemy* enemy) { mEnemies.emplace_back(enemy); }
    void AddBoat(Boat* boat) { mBoats.emplace_back(boat); }
    void AddBoatParts(BoatParts* boatParts) { mBoatParts.emplace_back(boatParts); }
    void AddCrystal(Crystal* crystal) { mCrystals.emplace_back(crystal); }
    void AddNPC(NPC* NPC) { mNPCs.emplace_back(NPC); }
    void AddPlatform(Platform* platform) { mPlatforms.emplace_back(platform); }
    void AddMovingPlatform(MovingPlatform* platform) { mMovingPlatforms.emplace_back(platform); }
    void AddBoatArrivalPoint(BoatArrivalPoint* point) { mBoatArrivalPoints.emplace_back(point); }
    void AddFallRespawnPoint(FallRespawnPoint* point) { mFallRespawnPoints.emplace_back(point); }

    void RemoveAllEnemy() { mEnemies.clear(); }
    void RemoveAllBoat() { mBoats.clear(); }
    void RemoveAllBoatParts() { mBoatParts.clear(); }
    void RemoveAllCrystals() { mCrystals.clear(); }
    void RemoveAllNPCs() { mNPCs.clear(); }
    void RemoveAllPlatforms() { mPlatforms.clear(); }
    void RemoveAllMovingPlatforms() { mMovingPlatforms.clear(); }
    void RemoveKey() { mKey = nullptr; }
    void RemoveStar() { mStar = nullptr; }
    void RemoveAllBoatArrivalPoints() { mBoatArrivalPoints.clear(); }
    void RemoveAllFallRespawnPoints() { mFallRespawnPoints.clear(); }

    void SetCurrentStage(Stage* currentStage) { mCurrentStage = currentStage; }
    void SetStageNum(int stageNum) { mStageNum = stageNum; }
    void SetColor(glm::vec4 color) { mColor = color; }
    void SetKey(Key* key) { mKey = key; }
    void SetStar(Star* star) { mStar = star; }

    void SetRocketSpawnCondition(const std::string& rocketSpawnCondition)
    {
        if (rocketSpawnCondition == "AllEnemiesDead") {
            mRocketSpawnCondition = RocketSpawnCondition::AllEnemiesDead;
        } else if (rocketSpawnCondition == "AllBoatPartsCollected") {
            mRocketSpawnCondition = RocketSpawnCondition::AllBoatPartsCollected;
        } else {
            mRocketSpawnCondition = RocketSpawnCondition::None;
        }
    }

    void SetPlanetShape(const std::string& PlanetShape)
    {
        if (PlanetShape == "Normal") {
            mPlanetShape = PlanetShape::Normal;
        } else if (PlanetShape == "Sphere") {
            mPlanetShape = PlanetShape::Sphere;
        } else if (PlanetShape == "Ellipse") {
            mPlanetShape = PlanetShape::Ellipse;
        }
    }

    Stage* GetCurrentStage() const { return mCurrentStage; }

    int GetRemainBoatPartsCount() const { return mRemainBoatPartsCount; }

    const glm::vec4& GetColor() const { return mColor; }

    const std::vector<Enemy*>& GetEnemies() const { return mEnemies; }
    const std::vector<Boat*>& GetBoats() const { return mBoats; }
    const std::vector<BoatParts*>& GetBoatParts() const { return mBoatParts; }
    const std::vector<Crystal*>& GetCrystals() const { return mCrystals; }
    const std::vector<NPC*>& GetNPCs() const { return mNPCs; }
    const std::vector<Platform*>& GetPlatforms() const { return mPlatforms; }
    const std::vector<MovingPlatform*>& GetMovingPlatforms() const { return mMovingPlatforms; }
    const std::vector<BoatArrivalPoint*>& GetBoatArrivalPoints() const { return mBoatArrivalPoints; }
    const std::vector<FallRespawnPoint*>& GetFallRespawnPoints() const { return mFallRespawnPoints; }

    Key* GetKey() const { return mKey; }
    Star* GetStar() const { return mStar; }
    PlanetShape GetPlanetShape() const { return mPlanetShape; }

private:
    void InitRemainBoatPartsCount();
    bool CheckIsAllEnemiesDead();
    bool CheckIsAllBoatPartsCollected();
    void StartBoatFocus();

private:
    int mStageNum;
    int mRemainBoatPartsCount;

    glm::vec4 mColor;

    std::vector<Enemy*> mEnemies;
    std::vector<Boat*> mBoats;
    std::vector<BoatParts*> mBoatParts;
    std::vector<Crystal*> mCrystals;
    std::vector<NPC*> mNPCs;
    std::vector<Platform*> mPlatforms;
    std::vector<MovingPlatform*> mMovingPlatforms;
    std::vector<BoatArrivalPoint*> mBoatArrivalPoints;
    std::vector<FallRespawnPoint*> mFallRespawnPoints;

    Key* mKey;
    Star* mStar;
    Stage* mCurrentStage;

    RocketSpawnCondition mRocketSpawnCondition;
    PlanetShape mPlanetShape;
};