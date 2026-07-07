#include "Star.h"
#include "Game.h"
#include "component/CollectableComponent.h"

#include <yaml-cpp/yaml.h>

Star::Star(Game* game)
    : Actor(game)
{
    mIsActive = false;
    AddCollectableComponent();
}

void Star::ApplyConfig()
{
    YAML::Node starRoot = YAML::LoadFile("../assets/data/actor/stars.yaml");

    if (!starRoot["stars"] || !starRoot["stars"].IsSequence()) {
        return;
    }

    for (const YAML::Node& starNode : starRoot["stars"]) {
        const std::string modelPath = starNode["modelPath"] ? starNode["modelPath"].as<std::string>() : "star.obj";
        SetModelPath(modelPath);

        const float scale = starNode["scale"] ? starNode["scale"].as<float>() : 0.0f;
        SetScale(glm::vec3(scale));
    }
}

void Star::AddCollectableComponent()
{
    std::unique_ptr<CollectableComponent> collectableComponent = std::make_unique<CollectableComponent>(this, 100);
    mCollectableComponent = collectableComponent.get();
    AddComponent(std::move(collectableComponent));
}

void Star::UpdateActor(float deltaTime)
{
    const bool shouldStartOnObtained = mCollectableComponent->GetIsObtained() && mIsActive;
    if (shouldStartOnObtained) {
        OnObtained();
    }
}

void Star::OnObtained()
{
    mIsActive = false;
    mGame->OnStarObtained();
}