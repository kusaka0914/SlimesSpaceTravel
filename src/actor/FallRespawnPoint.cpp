#include "FallRespawnPoint.h"

#include <yaml-cpp/yaml.h>

FallRespawnPoint::FallRespawnPoint(Game* game)
    : Actor(game),
      mDamage(1.0f)
{
    SetModelPath("platform.obj");
    SetScale(glm::vec3(4.0f, 1.0f, 4.0f));
    SetRadius(2.0f);
    mIsUpVecInitialized = true;
}

void FallRespawnPoint::ApplyConfig(const YAML::Node& root)
{
    if (!root["fallRespawnPoints"] || !root["fallRespawnPoints"].IsSequence()) {
        return;
    }

    for (const YAML::Node& pointNode : root["fallRespawnPoints"]) {
        const std::string modelPath =
            pointNode["modelPath"] ? pointNode["modelPath"].as<std::string>() : "platform.obj";
        SetModelPath(modelPath);

        mDamage = pointNode["damage"] ? pointNode["damage"].as<float>() : 1.0f;
    }
}
