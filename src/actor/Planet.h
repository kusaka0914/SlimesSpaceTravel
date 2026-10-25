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

// 惑星の表面計算と設定の窓口。所属Actorの登録と進行状況は専用クラスへ委譲する。
class Planet : public Actor {
public:
    enum class Biome {
        None,
        Grassland,
        Rocky,
    };

    struct VisualSettings {
        Biome biome = Biome::None;
        glm::vec3 atmosphereColor{0.28f, 0.62f, 1.0f};
        float atmosphereStrength = 0.22f;
        float decorationDensity = 1.0f;
    };

    // ワールド空間の表面位置と外向き単位法線。distanceは非負で、内外はisOutsideで区別する。
    struct EllipseSurfaceProjection {
        glm::vec3 position{0.0f};
        glm::vec3 outwardNormal{0.0f, 1.0f, 0.0f};
        float distance = 0.0f;
        bool isOutside = true;
    };

    enum class PlanetShape { Normal, Sphere, Ellipse };

    enum class EllipseSurfaceFace {
        Front,
        Side,
        Back,
    };

    Planet(Game* game);

    // 惑星1件分の設定ノードを適用する。他Actorのルート配列を受け取るApplyConfigとは区別する。
    void ApplyConfig(const YAML::Node& node);

    // 所属Actorの登録後に呼び、進行条件の初期状態を確定する。
    void Initialize() override;

    void OnBoatPartsObtained();
    void OnEnemyDead();

    // 球面上のワールド位置。thetaはXZ方位、phiは仰角（ラジアン）、heightは半径からの追加距離。
    glm::vec3 CalculateSurfacePos(float theta, float phi, float height) const;

    // 最短スケール軸の単位方向を、worldPositionが属する表裏に応じた符号で返す。
    glm::vec3 CalculateEllipseVerticalDirection(const glm::vec3& worldPosition) const;

    // 楕円体の外部点は最近傍の表面へ、内部点は中心からの放射方向へ投影する。
    // 中心付近で方向が定まらない場合は、最短半径軸の正方向を選ぶ。
    EllipseSurfaceProjection CalculateEllipseSurfaceProjection(const glm::vec3& worldPosition) const;

    // 最短スケール軸を上下軸とし、中央の帯をSideとして表裏を判定する。
    EllipseSurfaceFace ResolveEllipseSurfaceFace(const glm::vec3& worldPosition) const;

    // Sideを設けず表裏だけを判定する。面をまたぐ追跡・接触の抑制に使う。
    EllipseSurfaceFace ResolveEllipseSurfaceHemisphere(const glm::vec3& worldPosition) const;
    bool ArePositionsOnSameSurfaceFace(const glm::vec3& firstWorldPosition, const glm::vec3& secondWorldPosition) const;

    // 所属Actorの非所有参照を登録・解除する。登録解除はActor自体の破棄を意味しない。
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

    void RemoveAllEnemies() { mActorRegistry.RemoveAllEnemies(); }
    void RemoveEnemy(Enemy* enemy) { mActorRegistry.RemoveEnemy(enemy); }
    void RemoveAllBoats() { mActorRegistry.RemoveAllBoats(); }
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
    void SetBackTextureOverridePath(const std::string& texturePath) { mBackTextureOverridePath = texturePath; }
    void SetTextureSideBlendWidth(float width) { mTextureSideBlendWidth = glm::clamp(width, 0.0f, 0.5f); }
    void SetCanAttractNearbyPlayer(bool canAttractNearbyPlayer) { mCanAttractNearbyPlayer = canAttractNearbyPlayer; }
    void SetKey(Key* key) { mActorRegistry.SetKey(key); }
    void SetStar(Star* star) { mActorRegistry.SetStar(star); }

    void SetRocketSpawnCondition(const std::string& rocketSpawnCondition)
    {
        mProgressController.SetRocketSpawnCondition(rocketSpawnCondition);
    }
    std::string GetRocketSpawnCondition() const { return mProgressController.GetRocketSpawnCondition(); }
    bool HasAppearedRocket() const;

    Stage* GetCurrentStage() const { return mCurrentStage; }

    int GetRemainBoatPartsCount() const { return mProgressController.GetRemainBoatPartsCount(); }

    const glm::vec4& GetColor() const { return mColor; }
    const std::string& GetBackTextureOverridePath() const { return mBackTextureOverridePath; }
    float GetTextureSideBlendWidth() const { return mTextureSideBlendWidth; }
    bool CanAttractNearbyPlayer() const { return mCanAttractNearbyPlayer; }

    const std::vector<Enemy*>& GetEnemies() const { return mActorRegistry.GetEnemies(); }
    const std::vector<Boat*>& GetBoats() const { return mActorRegistry.GetBoats(); }
    const std::vector<BoatParts*>& GetBoatParts() const { return mActorRegistry.GetBoatParts(); }
    const std::vector<Crystal*>& GetCrystals() const { return mActorRegistry.GetCrystals(); }
    const std::vector<NPC*>& GetNPCs() const { return mActorRegistry.GetNPCs(); }
    const std::vector<Platform*>& GetPlatforms() const { return mActorRegistry.GetPlatforms(); }
    const std::vector<BoatArrivalPoint*>& GetBoatArrivalPoints() const { return mActorRegistry.GetBoatArrivalPoints(); }
    const std::vector<FallRespawnPoint*>& GetFallRespawnPoints() const { return mActorRegistry.GetFallRespawnPoints(); }
    const std::vector<StageObject*>& GetStageObjects() const { return mActorRegistry.GetStageObjects(); }
    const std::vector<TutorialTrigger*>& GetTutorialTriggers() const { return mActorRegistry.GetTutorialTriggers(); }
    const std::vector<JewelItem*>& GetJewelItems() const { return mActorRegistry.GetJewelItems(); }
    const std::vector<HazardActor*>& GetHazardActors() const { return mActorRegistry.GetHazardActors(); }

    Key* GetKey() const { return mActorRegistry.GetKey(); }
    Star* GetStar() const { return mActorRegistry.GetStar(); }

    // スケールの絶対値から分類する。全軸がほぼ0ならNormal、等方ならSphere、それ以外はEllipse。
    PlanetShape GetPlanetShape() const;
    const VisualSettings& GetVisualSettings() const { return mVisualSettings; }

private:
    int mStageNum;

    glm::vec4 mColor;

    Stage* mCurrentStage;

    PlanetActorRegistry mActorRegistry;
    PlanetProgressController mProgressController;

    std::string mBackTextureOverridePath;
    float mTextureSideBlendWidth = 0.05f;
    bool mCanAttractNearbyPlayer = true;
    VisualSettings mVisualSettings;
};
