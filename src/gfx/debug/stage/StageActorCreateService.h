#pragma once

#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/stage/StageActorNodeFactory.h"
#include "gfx/debug/stage/StageActorRuntimeCreationService.h"
#include "gfx/debug/stage/StageEditorTypes.h"

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

class StageActorCreateService {
public:
    explicit StageActorCreateService(DebugEditorContext& context);

    bool AddPlatform(int currentPlanetNum, const std::string& modelPath, const glm::vec3& scale,
                     const StageActorPlacement* placement = nullptr);
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
    bool AddMovingPlatform(
        int currentPlanetNum,
        const StageActorPlacement& startPlacement,
        const StageActorPlacement& endPlacement,
        const glm::vec3& scale);
    bool AddFadingPlatform(
        int currentPlanetNum,
        const glm::vec3& scale,
        const StageActorPlacement* placement = nullptr);
    bool AddAdhesivePlatform(
        int currentPlanetNum,
        const glm::vec3& scale,
        const StageActorPlacement* placement = nullptr);
    bool AddTwoPlayerSwitchPair(
        int currentPlanetNum,
        const StageActorPlacement& firstPlacement,
        const StageActorPlacement& secondPlacement);
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
    void ApplyPlacementToNode(YAML::Node& node, int planetIndex,
                              const StageActorPlacement* placement) const;

    void EnsureSequence(YAML::Node& config, const std::string& sequenceName) const;

private:
    DebugEditorContext& mContext;
    StageActorNodeFactory mNodeFactory;
    StageActorRuntimeCreationService mRuntimeCreationService;
};
