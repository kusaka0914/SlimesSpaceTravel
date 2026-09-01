#pragma once

#include <functional>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

struct DebugEditorContext;

class StageActorNodeFactory {
public:
    using SurfaceDistanceCalculator =
        std::function<float(int planetIndex, float height)>;

    explicit StageActorNodeFactory(DebugEditorContext& context);
    explicit StageActorNodeFactory(
        SurfaceDistanceCalculator calculateSurfaceDistance);

    YAML::Node CreatePlatform(int planetIndex, const std::string& modelPath, const glm::vec3& scale) const;
    YAML::Node CreateRideMovingPlatform(int planetIndex, const std::string& modelPath, const glm::vec3& scale) const;
    YAML::Node CreatePlanet(int planetIndex, const std::string& modelPath) const;
    YAML::Node CreateEnemy(const std::string& type, int planetIndex) const;
    YAML::Node CreateNPC(
        const std::string& modelPath,
        int planetIndex,
        const std::string& name,
        const std::vector<std::string>& talkTexts,
        float radius,
        float scale) const;
    YAML::Node CreateTutorialTrigger(
        int planetIndex,
        const std::string& modelPath,
        const std::vector<std::string>& talkTexts,
        const glm::vec3& scale) const;
    YAML::Node CreateCrystal(const std::string& type, int planetIndex) const;
    YAML::Node CreateBoatParts(const std::string& type, int planetIndex) const;
    YAML::Node CreateBoat(int startPlanetIndex, int destinationPlanetIndex, int destinationStage) const;
    YAML::Node CreateBoatArrivalPoint(int planetIndex, const std::string& modelPath, const glm::vec3& scale) const;
    YAML::Node CreateStar(int planetIndex) const;
    YAML::Node CreateJewelItem(
        int planetIndex,
        const std::string& modelPath,
        const std::string& texturePath,
        const glm::vec3& scale) const;
    YAML::Node CreateHazardActor(
        int planetIndex,
        const std::string& modelPath,
        const std::string& texturePath,
        const glm::vec3& scale,
        float triggerRadius,
        float damage,
        float damageIntervalSeconds) const;
    YAML::Node CreateStageObject(
        int planetIndex,
        const std::string& modelPath,
        bool isCollisionEnabled) const;

private:
    float CalculateInitialSurfaceDistance(int planetIndex, float height) const;

    SurfaceDistanceCalculator mCalculateSurfaceDistance;
};
