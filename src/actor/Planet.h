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
class Stage;
class Key;
class Star;
class BoatArrivalPoint;
class FallRespawnPoint;
class StageObject;
class TutorialTrigger;
class JewelItem;
class HazardActor;

class Planet : public Actor {
public:
    struct EllipseSurfaceProjection {
        glm::vec3 position{0.0f};
        glm::vec3 outwardNormal{0.0f, 1.0f, 0.0f};
        float distance = 0.0f;
        bool isOutside = true;
    };

    enum class RocketSpawnCondition { AllEnemiesDead, AllBoatPartsCollected, None };

    enum class PlanetShape { Normal, Sphere, Ellipse };

    enum class EllipseSurfaceFace {
        Front,
        Side,
        Back,
    };

    Planet(Game* game);

    void ApplyConfig(const YAML::Node& node);
    void Initialize() override;

    void OnBoatPartsObtained();
    void OnEnemyDead();

    glm::vec3 CalculateSurfacePos(float theta, float phi, float height) const;
    glm::vec3 CalculateEllipseVerticalDirection(
        const glm::vec3& worldPosition) const;
    EllipseSurfaceProjection CalculateEllipseSurfaceProjection(
        const glm::vec3& worldPosition) const;
    EllipseSurfaceFace ResolveEllipseSurfaceFace(
        const glm::vec3& worldPosition) const;
    EllipseSurfaceFace ResolveEllipseSurfaceHemisphere(
        const glm::vec3& worldPosition) const;
    bool ArePositionsOnSameSurfaceFace(
        const glm::vec3& firstWorldPosition,
        const glm::vec3& secondWorldPosition) const;

    void AddEnemy(Enemy* enemy) { mActorRegistry.AddEnemy(enemy); }
    void AddBoat(Boat* boat) { mActorRegistry.AddBoat(boat); }
    void AddBoatParts(BoatParts* boatParts) { mActorRegistry.AddBoatParts(boatParts); }
    void AddCrystal(Crystal* crystal) { mActorRegistry.AddCrystal(crystal); }
    void AddNPC(NPC* npc) { mActorRegistry.AddNPC(npc); }
    void AddPlatform(Platform* platform) { mActorRegistry.AddPlatform(platform); }
    void AddBoatArrivalPoint(BoatArrivalPoint* point) { mActorRegistry.AddBoatArrivalPoint(point); }
    void AddFallRespawnPoint(FallRespawnPoint* point) { mActorRegistry.AddFallRespawnPoint(point); }
    void AddStageObject(StageObject* stageObject) { mActorRegistry.AddStageObject(stageObject); }
    void AddTutorialTrigger(TutorialTrigger* trigger) { mActorRegistry.AddTutorialTrigger(trigger); }
    void AddJewelItem(JewelItem* jewelItem) { mActorRegistry.AddJewelItem(jewelItem); }
    void AddHazardActor(HazardActor* hazardActor) { mActorRegistry.AddHazardActor(hazardActor); }

    void RemoveAllEnemy() { mActorRegistry.RemoveAllEnemy(); }
    void RemoveEnemy(Enemy* enemy) { mActorRegistry.RemoveEnemy(enemy); }
    void RemoveAllBoat() { mActorRegistry.RemoveAllBoat(); }
    void RemoveBoat(Boat* boat) { mActorRegistry.RemoveBoat(boat); }
    void RemoveAllBoatParts() { mActorRegistry.RemoveAllBoatParts(); }
    void RemoveBoatParts(BoatParts* boatParts) { mActorRegistry.RemoveBoatParts(boatParts); }
    void RemoveAllCrystals() { mActorRegistry.RemoveAllCrystals(); }
    void RemoveCrystal(Crystal* crystal) { mActorRegistry.RemoveCrystal(crystal); }
    void RemoveAllNPCs() { mActorRegistry.RemoveAllNPCs(); }
    void RemoveNPC(NPC* npc) { mActorRegistry.RemoveNPC(npc); }
    void RemoveAllPlatforms() { mActorRegistry.RemoveAllPlatforms(); }
    void RemovePlatform(Platform* platform) { mActorRegistry.RemovePlatform(platform); }
    void RemovePlatformsByStageSequence(const std::string& sequenceName)
    {
        mActorRegistry.RemovePlatformsByStageSequence(sequenceName);
    }
    void RemoveKey() { mActorRegistry.RemoveKey(); }
    void RemoveStar() { mActorRegistry.RemoveStar(); }
    void RemoveAllBoatArrivalPoints() { mActorRegistry.RemoveAllBoatArrivalPoints(); }
    void RemoveBoatArrivalPoint(BoatArrivalPoint* point) { mActorRegistry.RemoveBoatArrivalPoint(point); }
    void RemoveAllFallRespawnPoints() { mActorRegistry.RemoveAllFallRespawnPoints(); }
    void RemoveFallRespawnPoint(FallRespawnPoint* point) { mActorRegistry.RemoveFallRespawnPoint(point); }
    void RemoveAllStageObjects() { mActorRegistry.RemoveAllStageObjects(); }
    void RemoveStageObject(StageObject* stageObject) { mActorRegistry.RemoveStageObject(stageObject); }
    void RemoveAllTutorialTriggers() { mActorRegistry.RemoveAllTutorialTriggers(); }
    void RemoveTutorialTrigger(TutorialTrigger* trigger) { mActorRegistry.RemoveTutorialTrigger(trigger); }
    void RemoveAllJewelItems() { mActorRegistry.RemoveAllJewelItems(); }
    void RemoveJewelItem(JewelItem* jewelItem) { mActorRegistry.RemoveJewelItem(jewelItem); }
    void RemoveAllHazardActors() { mActorRegistry.RemoveAllHazardActors(); }
    void RemoveHazardActor(HazardActor* hazardActor) { mActorRegistry.RemoveHazardActor(hazardActor); }

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
    void SetCanAttractNearbyPlayer(bool canAttractNearbyPlayer)
    {
        mCanAttractNearbyPlayer = canAttractNearbyPlayer;
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
    bool HasAppearedRocket() const;

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
    bool CanAttractNearbyPlayer() const
    {
        return mCanAttractNearbyPlayer;
    }

    const std::vector<Enemy*>& GetEnemies() const { return mActorRegistry.GetEnemies(); }
    const std::vector<Boat*>& GetBoats() const { return mActorRegistry.GetBoats(); }
    const std::vector<BoatParts*>& GetBoatParts() const { return mActorRegistry.GetBoatParts(); }
    const std::vector<Crystal*>& GetCrystals() const { return mActorRegistry.GetCrystals(); }
    const std::vector<NPC*>& GetNPCs() const { return mActorRegistry.GetNPCs(); }
    const std::vector<Platform*>& GetPlatforms() const { return mActorRegistry.GetPlatforms(); }
    const std::vector<BoatArrivalPoint*>& GetBoatArrivalPoints() const { return mActorRegistry.GetBoatArrivalPoints(); }
    const std::vector<FallRespawnPoint*>& GetFallRespawnPoints() const { return mActorRegistry.GetFallRespawnPoints(); }
    const std::vector<StageObject*>& GetStageObjects() const { return mActorRegistry.GetStageObjects(); }
    const std::vector<TutorialTrigger*>& GetTutorialTriggers() const
    {
        return mActorRegistry.GetTutorialTriggers();
    }
    const std::vector<JewelItem*>& GetJewelItems() const
    {
        return mActorRegistry.GetJewelItems();
    }
    const std::vector<HazardActor*>& GetHazardActors() const
    {
        return mActorRegistry.GetHazardActors();
    }

    Key* GetKey() const { return mActorRegistry.GetKey(); }
    Star* GetStar() const { return mActorRegistry.GetStar(); }
    PlanetShape GetPlanetShape() const;

private:
    int mStageNum;

    glm::vec4 mColor;

    Stage* mCurrentStage;

    PlanetActorRegistry mActorRegistry;
    PlanetProgressController mProgressController;

    std::string mBackTextureOverridePath;
    float mTextureSideBlendWidth = 0.05f;
    bool mCanAttractNearbyPlayer = true;
};
