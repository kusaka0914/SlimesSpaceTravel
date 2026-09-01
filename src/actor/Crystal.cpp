#include "Crystal.h"
#include "Game.h"
#include "component/DestructibleComponent.h"
#include "system/AudioSystem.h"
#include <iostream>
#include <yaml-cpp/yaml.h>

Crystal::Crystal(Game* game)
    : Actor(game)
{
    AddDestructibleComponent();
}

void Crystal::ApplyConfig(
    const YAML::Node& crystalRoot,
    const std::string& type)
{
    if (!crystalRoot["crystals"] || !crystalRoot["crystals"].IsSequence()) {
        return;
    }

    for (const YAML::Node& crystalNode : crystalRoot["crystals"]) {
        const std::string nodeType = crystalNode["type"] ? crystalNode["type"].as<std::string>() : "";

        if (nodeType == "common") {
            const std::string modelPath = crystalNode["modelPath"] ? crystalNode["modelPath"].as<std::string>() : "";
            SetModelPath(modelPath);
            continue;
        }

        if (type != nodeType) {
            continue;
        }

        const float hp = crystalNode["hp"] ? crystalNode["hp"].as<float>() : 80.0f;
        mDestructibleComponent->SetDestroyHp(hp);

        const float scale = crystalNode["scale"] ? crystalNode["scale"].as<float>() : 0.25f;
        SetScale(glm::vec3(scale));

        const float radius = crystalNode["radius"] ? crystalNode["radius"].as<float>() : 1.0f;
        SetRadius(radius);
    }
}

void Crystal::AddDestructibleComponent()
{
    std::unique_ptr<DestructibleComponent> destructibleComponent = std::make_unique<DestructibleComponent>(this, 100);
    mDestructibleComponent = destructibleComponent.get();
    AddComponent(std::move(destructibleComponent));
}

void Crystal::UpdateActor(float deltaTime)
{
    const bool shouldStartOnDestroyed = mDestructibleComponent->GetIsDestroyed() && mIsActive;
    if (shouldStartOnDestroyed) {
        OnDestroyed();
    }
}

void Crystal::OnDestroyed()
{
    mIsActive = false;
    mGame->GetAudioSystem()->PlaySE("destroy_se");
}
