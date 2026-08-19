#pragma once

#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/stage/StageEditorTypes.h"

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

struct StageActorPlacement {
    glm::vec3 worldPosition{0.0f};
    glm::vec3 surfaceNormal{0.0f, 1.0f, 0.0f};
};

class StageActorCreateService {
public:
    explicit StageActorCreateService(DebugEditorContext& context);

    bool AddPlatform(int currentPlanetNum, const std::string& modelPath, const glm::vec3& scale,
                     const StageActorPlacement* placement = nullptr);
    bool AddUGCPlatformCell(
        int currentPlanetNum,
        const StageActorPlacement& placement,
        float gridSize,
        int footprintSideLength = 1);
    bool RefreshUGCPlatformCells();
    bool TranslateUGCPlatformCells(
        const StageActorRef& generatedPlatformRef,
        const glm::vec3& worldDelta);
    bool TranslateUGCPlatformCells(
        const std::vector<StageActorRef>& generatedPlatformRefs,
        const glm::vec3& worldDelta);
    bool RemoveUGCPlatformCell(
        const StageActorRef& generatedPlatformRef,
        const glm::vec3& hitPosition);
    bool RemoveUGCPlatformCellAtGridPosition(
        int planetIndex,
        const glm::vec3& worldPosition,
        float gridSize,
        int gridLayer);
    bool ResolveUGCPlatformLayerAtGridPosition(
        int planetIndex,
        const glm::vec3& worldPosition,
        float gridSize,
        int preferredGridLayer,
        int& outGridLayer) const;
    int ResolveUGCPlatformPlacementLayerAtGridPosition(
        int planetIndex,
        const glm::vec3& worldPosition,
        float gridSize,
        int emptyColumnGridLayer) const;
    bool AddPressureSwitchPlatform(
        int currentPlanetNum,
        const std::string& modelPath,
        const glm::vec3& scale,
        const StageActorPlacement* placement = nullptr);
    bool AddRideMovingPlatform(
        int currentPlanetNum,
        const std::string& modelPath,
        const glm::vec3& scale,
        const StageActorPlacement* placement = nullptr);
    bool AddPlanet(const std::string& modelPath);
    bool AddEllipsePlanet(const std::string& modelPath);
    bool AddEnemy(const std::string& type, int currentPlanetNum,
                  const StageActorPlacement* placement = nullptr);
    bool AddNPC(const std::string& modelPath, int currentPlanetNum, const std::string& name,
                const std::vector<std::string>& talkTexts, float radius, float scale,
                const StageActorPlacement* placement = nullptr);
    bool AddTutorialTrigger(
        int currentPlanetNum,
        const std::string& modelPath,
        const std::vector<std::string>& talkTexts,
        const glm::vec3& scale,
        const StageActorPlacement* placement = nullptr);
    bool AddCrystal(const std::string& type, int currentPlanetNum,
                    const StageActorPlacement* placement = nullptr);
    bool AddBoatParts(const std::string& type, int currentPlanetNum,
                      const StageActorPlacement* placement = nullptr);
    bool AddBoat(int startPlanetNum, int destPlanetNum, int destStage,
                 const StageActorPlacement* placement = nullptr);
    bool AddBoatArrivalPoint(
        int currentPlanetNum,
        const std::string& modelPath,
        const glm::vec3& scale,
        const StageActorPlacement* placement = nullptr);
    bool AddStar(int currentPlanetNum, const StageActorPlacement* placement = nullptr);
    bool AddJewelItem(
        int currentPlanetNum,
        const std::string& modelPath,
        const std::string& texturePath,
        const glm::vec3& scale,
        const StageActorPlacement* placement = nullptr);
    bool AddHazardActor(
        int currentPlanetNum,
        const std::string& modelPath,
        const std::string& texturePath,
        const glm::vec3& scale,
        float triggerRadius,
        float damage,
        float damageIntervalSeconds,
        const StageActorPlacement* placement = nullptr);
    bool AddStageObject(int currentPlanetNum, const std::string& modelPath, bool collisionEnabled,
                        const StageActorPlacement* placement = nullptr);
    bool DuplicateActorAtPlacement(
        const StageActorRef& sourceRef,
        const YAML::Node& sourceNode,
        int targetPlanetIndex,
        const StageActorPlacement& placement);

private:
    bool CanCreateActor() const;
    bool IsValidPlanetIndex(int planetIndex, const char* label) const;
    void RefreshPhysicsWorld() const;
    void ApplyPlacementToNode(YAML::Node& node, int planetIndex,
                              const StageActorPlacement* placement) const;
    bool CreateActorFromStageNode(
        const StageActorRef& actorRef,
        const YAML::Node& actorNode,
        int stageYamlIndex) const;

    void EnsureSequence(YAML::Node& config, const std::string& sequenceName) const;

    YAML::Node CreatePlatformNode(int currentPlanetNum, const std::string& modelPath, const glm::vec3& scale) const;
    bool RebuildUGCPlatformNodes(YAML::Node& config) const;
    YAML::Node CreateRideMovingPlatformNode(
        int currentPlanetNum,
        const std::string& modelPath,
        const glm::vec3& scale) const;
    YAML::Node CreatePlanetNode(int planetIndex, const std::string& modelPath) const;
    YAML::Node CreateEnemyNode(const std::string& type, int currentPlanetNum) const;
    YAML::Node CreateNPCNode(const std::string& modelPath, int currentPlanetNum,
                             const std::string& name,
                             const std::vector<std::string>& talkTexts,
                             float radius,
                             float scale) const;
    YAML::Node CreateTutorialTriggerNode(
        int currentPlanetNum,
        const std::string& modelPath,
        const std::vector<std::string>& talkTexts,
        const glm::vec3& scale) const;
    YAML::Node CreateCrystalNode(const std::string& type, int currentPlanetNum) const;
    YAML::Node CreateBoatPartsNode(const std::string& type, int currentPlanetNum) const;
    YAML::Node CreateBoatNode(int startPlanetNum, int destPlanetNum, int destStage) const;
    YAML::Node CreateBoatArrivalPointNode(
        int currentPlanetNum,
        const std::string& modelPath,
        const glm::vec3& scale) const;
    YAML::Node CreateStarNode(int currentPlanetNum) const;
    YAML::Node CreateJewelItemNode(
        int currentPlanetNum,
        const std::string& modelPath,
        const std::string& texturePath,
        const glm::vec3& scale) const;
    YAML::Node CreateHazardActorNode(
        int currentPlanetNum,
        const std::string& modelPath,
        const std::string& texturePath,
        const glm::vec3& scale,
        float triggerRadius,
        float damage,
        float damageIntervalSeconds) const;
    YAML::Node CreateStageObjectNode(int currentPlanetNum, const std::string& modelPath,
                                     bool collisionEnabled) const;

private:
    DebugEditorContext& mContext;
};
