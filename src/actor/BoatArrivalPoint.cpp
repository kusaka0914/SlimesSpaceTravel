#include "BoatArrivalPoint.h"

#include <glm/glm.hpp>
#include <yaml-cpp/yaml.h>

BoatArrivalPoint::BoatArrivalPoint(Game* game)
    : Actor(game)
{
}

void BoatArrivalPoint::ApplyConfig()
{
    YAML::Node root = YAML::LoadFile("../assets/data/actor/boatarrivalpoints.yaml");

    if (!root["boatArrivalPoints"] || !root["boatArrivalPoints"].IsSequence()) {
        return;
    }

    for (const YAML::Node& pointNode : root["boatArrivalPoints"]) {
        const std::string modelPath =
            pointNode["modelPath"] ? pointNode["modelPath"].as<std::string>() : "platform.obj";
        SetModelPath(modelPath);

        const float scale = pointNode["scale"] ? pointNode["scale"].as<float>() : 0.4f;
        SetScale(glm::vec3(scale));
    }
}