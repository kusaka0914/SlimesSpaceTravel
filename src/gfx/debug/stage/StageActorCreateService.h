#pragma once

#include "gfx/debug/DebugEditorContext.h"

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

class StageActorCreateService {
public:
    explicit StageActorCreateService(DebugEditorContext& context);

    bool AddPlatform(int currentPlanetNum, const std::string& modelPath, const glm::vec3& scale);
    bool AddRideMovingPlatform(
        int currentPlanetNum,
        const std::string& modelPath,
        const glm::vec3& scale);
    bool AddPlanet(const std::string& modelPath);
    bool AddEnemy(const std::string& type, int currentPlanetNum);
    bool AddNPC(const std::string& modelPath, int currentPlanetNum, const std::string& name,
                const std::vector<std::string>& talkTexts, float radius, float scale);
    bool AddCrystal(const std::string& type, int currentPlanetNum);
    bool AddBoatParts(const std::string& type, int currentPlanetNum);
    bool AddBoat(int startPlanetNum, int destPlanetNum, int destStage);
    bool AddStar(int currentPlanetNum);
    bool AddStageObject(int currentPlanetNum, const std::string& modelPath, bool collisionEnabled);

private:
    bool CanCreateActor() const;
    bool IsValidPlanetIndex(int planetIndex, const char* label) const;
    void RefreshPhysicsWorld() const;

    void EnsureSequence(YAML::Node& config, const std::string& sequenceName) const;

    YAML::Node CreatePlatformNode(int currentPlanetNum, const std::string& modelPath, const glm::vec3& scale) const;
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
    YAML::Node CreateCrystalNode(const std::string& type, int currentPlanetNum) const;
    YAML::Node CreateBoatPartsNode(const std::string& type, int currentPlanetNum) const;
    YAML::Node CreateBoatNode(int startPlanetNum, int destPlanetNum, int destStage) const;
    YAML::Node CreateStarNode(int currentPlanetNum) const;
    YAML::Node CreateStageObjectNode(int currentPlanetNum, const std::string& modelPath,
                                     bool collisionEnabled) const;

private:
    DebugEditorContext& mContext;
};
