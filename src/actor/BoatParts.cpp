#include "BoatParts.h"
#include "Game.h"
#include "component/CollectableComponent.h"
#include "system/AudioSystem.h"

#include <yaml-cpp/yaml.h>

BoatParts::BoatParts(Game* game)
    : Actor(game)
{
    AddCollectableComponent();
}

void BoatParts::ApplyConfig(
    const YAML::Node& boatPartsRoot,
    const std::string& type)
{
    if (!boatPartsRoot["boatParts"] || !boatPartsRoot["boatParts"].IsSequence()) {
        return;
    }

    for (const YAML::Node& boatPartsNode : boatPartsRoot["boatParts"]) {
        const std::string nodeType = boatPartsNode["type"] ? boatPartsNode["type"].as<std::string>() : "";

        if (nodeType == "common") {
            const float scale = boatPartsNode["scale"] ? boatPartsNode["scale"].as<float>() : 0.25f;
            SetScale(glm::vec3(scale));
            continue;
        }

        if (type != nodeType) {
            continue;
        }

        const std::string modelPath = boatPartsNode["modelPath"] ? boatPartsNode["modelPath"].as<std::string>() : "";
        SetModelPath(modelPath);
    }
}

void BoatParts::AddCollectableComponent()
{
    std::unique_ptr<CollectableComponent> collectableComponent = std::make_unique<CollectableComponent>(this, 100);
    mCollectableComponent = collectableComponent.get();
    AddComponent(std::move(collectableComponent));
}

void BoatParts::UpdateActor(float deltaTime)
{
    bool shouldStartOnObtained = mCollectableComponent->GetIsObtained() && mIsActive;
    if (shouldStartOnObtained) {
        OnObtained();
    }
}

void BoatParts::OnObtained()
{
    mIsActive = false;
    mGame->OnBoatPartsObtained();
}
