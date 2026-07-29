#pragma once

#include "actor/Actor.h"
#include "actor/planet/PlanetActorRegistry.h"
#include "actor/planet/PlanetProgressController.h"
#include <glm/glm.hpp>
#include <string>
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
class StageObject;

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

    void AddEnemy(Enemy* enemy) { mActorRegistry.AddEnemy(enemy); }
    void AddBoat(Boat* boat) { mActorRegistry.AddBoat(boat); }
    void AddBoatParts(BoatParts* boatParts) { mActorRegistry.AddBoatParts(boatParts); }
    void AddCrystal(Crystal* crystal) { mActorRegistry.AddCrystal(crystal); }
    void AddNPC(NPC* npc) { mActorRegistry.AddNPC(npc); }
    void AddPlatform(Platform* platform) { mActorRegistry.AddPlatform(platform); }
    void AddMovingPlatform(MovingPlatform* platform) { mActorRegistry.AddMovingPlatform(platform); }
    void AddBoatArrivalPoint(BoatArrivalPoint* point) { mActorRegistry.AddBoatArrivalPoint(point); }
    void AddFallRespawnPoint(FallRespawnPoint* point) { mActorRegistry.AddFallRespawnPoint(point); }
    void AddStageObject(StageObject* stageObject) { mActorRegistry.AddStageObject(stageObject); }

    void RemoveAllEnemy() { mActorRegistry.RemoveAllEnemy(); }
    void RemoveAllBoat() { mActorRegistry.RemoveAllBoat(); }
    void RemoveBoat(Boat* boat) { mActorRegistry.RemoveBoat(boat); }
    void RemoveAllBoatParts() { mActorRegistry.RemoveAllBoatParts(); }
    void RemoveAllCrystals() { mActorRegistry.RemoveAllCrystals(); }
    void RemoveAllNPCs() { mActorRegistry.RemoveAllNPCs(); }
    void RemoveAllPlatforms() { mActorRegistry.RemoveAllPlatforms(); }
    void RemoveAllMovingPlatforms() { mActorRegistry.RemoveAllMovingPlatforms(); }
    void RemoveKey() { mActorRegistry.RemoveKey(); }
    void RemoveStar() { mActorRegistry.RemoveStar(); }
    void RemoveAllBoatArrivalPoints() { mActorRegistry.RemoveAllBoatArrivalPoints(); }
    void RemoveAllFallRespawnPoints() { mActorRegistry.RemoveAllFallRespawnPoints(); }
    void RemoveAllStageObjects() { mActorRegistry.RemoveAllStageObjects(); }

    void SetCurrentStage(Stage* currentStage) { mCurrentStage = currentStage; }
    void SetStageNum(int stageNum) { mStageNum = stageNum; }
    void SetColor(glm::vec4 color) { mColor = color; }
    void SetBackTextureOverridePath(const std::string& texturePath)
    {
        mBackTextureOverridePath = texturePath;
    }
    void SetTextureSideBlendWidth(float width)
    {
        mTextureSideBlendWidth = glm::clamp(width, 0.0f, 0.5f);
    }
    void SetKey(Key* key) { mActorRegistry.SetKey(key); }
    void SetStar(Star* star) { mActorRegistry.SetStar(star); }

    void SetRocketSpawnCondition(const std::string& rocketSpawnCondition)
    {
        mProgressController.SetRocketSpawnCondition(rocketSpawnCondition);
    }
    std::string GetRocketSpawnCondition() const
    {
        return mProgressController.GetRocketSpawnCondition();
    }

    void SetPlanetShape(const std::string& planetShape)
    {
        if (planetShape == "Normal") {
            mPlanetShape = PlanetShape::Normal;
        } else if (planetShape == "Sphere") {
            mPlanetShape = PlanetShape::Sphere;
        } else if (planetShape == "Ellipse") {
            mPlanetShape = PlanetShape::Ellipse;
        }
    }

    Stage* GetCurrentStage() const { return mCurrentStage; }

    int GetRemainBoatPartsCount() const { return mProgressController.GetRemainBoatPartsCount(); }

    const glm::vec4& GetColor() const { return mColor; }
    const std::string& GetBackTextureOverridePath() const
    {
        return mBackTextureOverridePath;
    }
    float GetTextureSideBlendWidth() const
    {
        return mTextureSideBlendWidth;
    }

    const std::vector<Enemy*>& GetEnemies() const { return mActorRegistry.GetEnemies(); }
    const std::vector<Boat*>& GetBoats() const { return mActorRegistry.GetBoats(); }
    const std::vector<BoatParts*>& GetBoatParts() const { return mActorRegistry.GetBoatParts(); }
    const std::vector<Crystal*>& GetCrystals() const { return mActorRegistry.GetCrystals(); }
    const std::vector<NPC*>& GetNPCs() const { return mActorRegistry.GetNPCs(); }
    const std::vector<Platform*>& GetPlatforms() const { return mActorRegistry.GetPlatforms(); }
    const std::vector<MovingPlatform*>& GetMovingPlatforms() const { return mActorRegistry.GetMovingPlatforms(); }
    const std::vector<BoatArrivalPoint*>& GetBoatArrivalPoints() const { return mActorRegistry.GetBoatArrivalPoints(); }
    const std::vector<FallRespawnPoint*>& GetFallRespawnPoints() const { return mActorRegistry.GetFallRespawnPoints(); }
    const std::vector<StageObject*>& GetStageObjects() const { return mActorRegistry.GetStageObjects(); }

    Key* GetKey() const { return mActorRegistry.GetKey(); }
    Star* GetStar() const { return mActorRegistry.GetStar(); }
    PlanetShape GetPlanetShape() const { return mPlanetShape; }

private:
    int mStageNum;

    glm::vec4 mColor;

    Stage* mCurrentStage;

    PlanetActorRegistry mActorRegistry;
    PlanetProgressController mProgressController;

    PlanetShape mPlanetShape;
    std::string mBackTextureOverridePath;
    float mTextureSideBlendWidth = 0.05f;
};
