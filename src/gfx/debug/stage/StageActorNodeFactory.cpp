#include "gfx/debug/stage/StageActorNodeFactory.h"

#include <algorithm>
#include <utility>

StageActorNodeFactory::StageActorNodeFactory(
    SurfaceDistanceCalculator calculateSurfaceDistance)
    : mCalculateSurfaceDistance(std::move(calculateSurfaceDistance))
{
}

YAML::Node StageActorNodeFactory::CreatePlatform(
    int planetIndex,
    const std::string& modelPath,
    const glm::vec3& scale) const
{
    YAML::Node node;
    node["currentPlanetNum"] = planetIndex;
    node["theta"] = 0.0f;
    node["phi"] = 0.0f;
    node["height"] = 1.0f;
    node["facingYaw"] = 0.0f;
    node["rotation"][0] = 0.0f;
    node["rotation"][1] = 0.0f;
    node["rotation"][2] = 0.0f;
    node["scale"][0] = scale.x;
    node["scale"][1] = scale.y;
    node["scale"][2] = scale.z;
    node["modelPath"] = modelPath;
    return node;
}

YAML::Node StageActorNodeFactory::CreateRideMovingPlatform(
    int planetIndex,
    const std::string& modelPath,
    const glm::vec3& scale) const
{
    YAML::Node node = CreatePlatform(planetIndex, modelPath, scale);
    YAML::Node movement = node["components"]["movement"];
    movement["moveOnPlayer"] = true;
    movement["moveDuration"] = 3.0f;
    movement["returnDelay"] = 1.0f;
    movement["endpointWaitSeconds"] = 0.0f;
    movement["moveOffset"][0] = 0.0f;
    movement["moveOffset"][1] = 5.0f;
    movement["moveOffset"][2] = 0.0f;
    return node;
}

YAML::Node StageActorNodeFactory::CreatePlanet(
    int planetIndex,
    const std::string& modelPath) const
{
    YAML::Node node;
    node["center"][0] = static_cast<float>(planetIndex) * 32.0f;
    node["center"][1] = 0.0f;
    node["center"][2] = 0.0f;
    node["scale"][0] = 4.0f;
    node["scale"][1] = 4.0f;
    node["scale"][2] = 4.0f;
    node["color"][0] = 1.0f;
    node["color"][1] = 1.0f;
    node["color"][2] = 1.0f;
    node["color"][3] = 1.0f;
    node["model"] = modelPath;
    node["shape"] = "Sphere";
    node["stageNum"] = planetIndex;
    node["canAttractNearbyPlayer"] = true;
    node["reactsToOverheadGravityRay"] = false;
    node["rocketSpawnCondition"] = "";
    return node;
}

YAML::Node StageActorNodeFactory::CreateEnemy(
    const std::string& type,
    int planetIndex) const
{
    YAML::Node node;
    node["editorName"] = type == "boss" ? "新しいボス敵" : "新しい通常敵";
    node["type"] = type;
    node["currentPlanetNum"] = planetIndex;
    node["theta"] = 0.0f;
    node["phi"] = 0.0f;
    node["height"] = 1.0f;
    node["pos"][0] = CalculateInitialSurfaceDistance(planetIndex, 1.0f);
    node["pos"][1] = 0.0f;
    node["pos"][2] = 0.0f;
    return node;
}

YAML::Node StageActorNodeFactory::CreateNPC(
    const std::string& modelPath,
    int planetIndex,
    const std::string& name,
    const std::vector<std::string>& talkTexts,
    float radius,
    float scale) const
{
    YAML::Node node;
    node["modelPath"] = modelPath;
    node["currentPlanetNum"] = planetIndex;
    node["theta"] = 0.0f;
    node["phi"] = 0.0f;
    node["height"] = 1.0f;
    node["facingYaw"] = 0.0f;
    node["radius"] = std::max(0.1f, radius);
    const float safeScale = std::max(0.01f, scale);
    node["scale"][0] = safeScale;
    node["scale"][1] = safeScale;
    node["scale"][2] = safeScale;
    node["name"] = name;
    for (const std::string& talkText : talkTexts) {
        node["talkTexts"].push_back(talkText);
    }
    if (talkTexts.empty()) {
        node["talkTexts"].push_back("");
    }
    return node;
}

YAML::Node StageActorNodeFactory::CreateTutorialTrigger(
    int planetIndex,
    const std::string& modelPath,
    const std::vector<std::string>& talkTexts,
    const glm::vec3& scale) const
{
    YAML::Node node;
    node["modelPath"] = modelPath;
    node["currentPlanetNum"] = planetIndex;
    node["theta"] = 0.0f;
    node["phi"] = 0.0f;
    node["height"] = 1.0f;
    node["scale"][0] = std::max(0.01f, scale.x);
    node["scale"][1] = std::max(0.01f, scale.y);
    node["scale"][2] = std::max(0.01f, scale.z);
    for (const std::string& talkText : talkTexts) {
        node["talkTexts"].push_back(talkText);
    }
    if (talkTexts.empty()) {
        node["talkTexts"].push_back("");
    }
    return node;
}

YAML::Node StageActorNodeFactory::CreateCrystal(
    const std::string& type,
    int planetIndex) const
{
    YAML::Node node;
    node["type"] = type;
    node["currentPlanetNum"] = planetIndex;
    node["theta"] = 0.0f;
    node["phi"] = 0.0f;
    node["height"] = 1.0f;
    return node;
}

YAML::Node StageActorNodeFactory::CreateBoatParts(
    const std::string& type,
    int planetIndex) const
{
    return CreateCrystal(type, planetIndex);
}

YAML::Node StageActorNodeFactory::CreateBoat(
    int startPlanetIndex,
    int destinationPlanetIndex,
    int destinationStage) const
{
    YAML::Node node;
    node["startPlanet"] = startPlanetIndex;
    node["destPlanet"] = destinationPlanetIndex;
    node["destStage"] = destinationStage;
    node["theta"] = 0.0f;
    node["phi"] = 0.0f;
    node["height"] = 1.0f;
    node["facingYaw"] = 0.0f;
    node["pos"][0] = CalculateInitialSurfaceDistance(startPlanetIndex, 1.0f);
    node["pos"][1] = 0.0f;
    node["pos"][2] = 0.0f;
    return node;
}

YAML::Node StageActorNodeFactory::CreateBoatArrivalPoint(
    int planetIndex,
    const std::string& modelPath,
    const glm::vec3& scale) const
{
    YAML::Node node;
    node["currentPlanetNum"] = planetIndex;
    node["theta"] = 0.0f;
    node["phi"] = 0.0f;
    node["height"] = 1.0f;
    node["facingYaw"] = 0.0f;
    node["modelPath"] = modelPath;
    node["rotation"][0] = 0.0f;
    node["rotation"][1] = 0.0f;
    node["rotation"][2] = 0.0f;
    node["scale"][0] = std::max(0.01f, scale.x);
    node["scale"][1] = std::max(0.01f, scale.y);
    node["scale"][2] = std::max(0.01f, scale.z);
    return node;
}

YAML::Node StageActorNodeFactory::CreateStar(int planetIndex) const
{
    YAML::Node node;
    node["currentPlanetNum"] = planetIndex;
    node["theta"] = 0.0f;
    node["phi"] = 0.0f;
    node["height"] = 1.0f;
    node["isActive"] = false;
    return node;
}

YAML::Node StageActorNodeFactory::CreateJewelItem(
    int planetIndex,
    const std::string& modelPath,
    const std::string& texturePath,
    const glm::vec3& scale) const
{
    YAML::Node node;
    node["currentPlanetNum"] = planetIndex;
    node["theta"] = 0.0f;
    node["phi"] = 0.0f;
    node["height"] = 0.15f;
    node["facingYaw"] = 0.0f;
    node["modelPath"] = modelPath;
    if (!texturePath.empty()) {
        node["textureOverride"] = texturePath;
    }
    node["scale"][0] = std::max(0.01f, scale.x);
    node["scale"][1] = std::max(0.01f, scale.y);
    node["scale"][2] = std::max(0.01f, scale.z);
    return node;
}

YAML::Node StageActorNodeFactory::CreateHazardActor(
    int planetIndex,
    const std::string& modelPath,
    const std::string& texturePath,
    const glm::vec3& scale,
    float triggerRadius,
    float damage,
    float damageIntervalSeconds) const
{
    YAML::Node node;
    node["currentPlanetNum"] = planetIndex;
    node["theta"] = 0.0f;
    node["phi"] = 0.0f;
    node["height"] = 0.75f;
    node["facingYaw"] = 0.0f;
    node["modelPath"] = modelPath;
    if (!texturePath.empty()) {
        node["textureOverride"] = texturePath;
    }
    node["scale"][0] = std::max(0.01f, scale.x);
    node["scale"][1] = std::max(0.01f, scale.y);
    node["scale"][2] = std::max(0.01f, scale.z);
    node["triggerRadius"] = std::max(0.01f, triggerRadius);
    node["damage"] = std::max(0.0f, damage);
    node["damageIntervalSeconds"] = std::max(0.0f, damageIntervalSeconds);
    return node;
}

YAML::Node StageActorNodeFactory::CreateStageObject(
    int planetIndex,
    const std::string& modelPath,
    bool isCollisionEnabled) const
{
    YAML::Node node;
    node["currentPlanetNum"] = planetIndex;
    node["theta"] = 0.0f;
    node["phi"] = 0.0f;
    node["height"] = 1.0f;
    node["facingYaw"] = 0.0f;
    node["modelPath"] = modelPath;
    node["collision"] = isCollisionEnabled;
    node["rotation"][0] = 0.0f;
    node["rotation"][1] = 0.0f;
    node["rotation"][2] = 0.0f;
    node["scale"][0] = 1.0f;
    node["scale"][1] = 1.0f;
    node["scale"][2] = 1.0f;
    node["pos"][0] = CalculateInitialSurfaceDistance(planetIndex, 1.0f);
    node["pos"][1] = 0.0f;
    node["pos"][2] = 0.0f;
    return node;
}

float StageActorNodeFactory::CalculateInitialSurfaceDistance(
    int planetIndex,
    float height) const
{
    return mCalculateSurfaceDistance(planetIndex, height);
}
