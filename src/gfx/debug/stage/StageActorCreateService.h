#pragma once

#include "gfx/debug/DebugEditorContext.h"

#include <glm/glm.hpp>
#include <string>
#include <yaml-cpp/yaml.h>

class StageActorCreateService {
public:
    explicit StageActorCreateService(DebugEditorContext& context);

    bool AddPlatform(int currentPlanetNum, const std::string& modelPath, const glm::vec3& scale);
    bool AddPlanet(const std::string& modelPath);
    bool AddEnemy(const std::string& type, int currentPlanetNum);
    bool AddNPC(const std::string& type, int currentPlanetNum);
    bool AddCrystal(const std::string& type, int currentPlanetNum);
    bool AddBoatParts(const std::string& type, int currentPlanetNum);
    bool AddBoat(int startPlanetNum, int destPlanetNum, int destStage);
    bool AddStar(int currentPlanetNum);

private:
    bool CanCreateActor() const;
    bool IsValidPlanetIndex(int planetIndex, const char* label) const;

    void EnsureSequence(YAML::Node& config, const std::string& sequenceName) const;

    YAML::Node CreatePlatformNode(int currentPlanetNum, const std::string& modelPath, const glm::vec3& scale) const;
    YAML::Node CreatePlanetNode(int planetIndex, const std::string& modelPath) const;
    YAML::Node CreateEnemyNode(const std::string& type, int currentPlanetNum) const;
    YAML::Node CreateNPCNode(const std::string& type, int currentPlanetNum) const;
    YAML::Node CreateCrystalNode(const std::string& type, int currentPlanetNum) const;
    YAML::Node CreateBoatPartsNode(const std::string& type, int currentPlanetNum) const;
    YAML::Node CreateBoatNode(int startPlanetNum, int destPlanetNum, int destStage) const;
    YAML::Node CreateStarNode(int currentPlanetNum) const;

private:
    DebugEditorContext& mContext;
};