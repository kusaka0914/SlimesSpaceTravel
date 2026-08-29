#include "Key.h"
#include "Game.h"
#include "actor/Boat.h"
#include "actor/Planet.h"
#include "component/CollectableComponent.h"
#include "component/FocusComponent.h"
#include "system/AudioSystem.h"

#include <yaml-cpp/yaml.h>

Key::Key(Game* game)
    : Actor(game),
      mIsActivePrev(false)
{
    mIsActive = false;

    AddCollectableComponent();
    AddFocusComponent();
}

void Key::ApplyConfig(const YAML::Node& keyRoot)
{
    if (!keyRoot["keys"] || !keyRoot["keys"].IsSequence()) {
        return;
    }

    for (const YAML::Node& keyNode : keyRoot["keys"]) {
        const std::string modelPath = keyNode["modelPath"] ? keyNode["modelPath"].as<std::string>() : "key.obj";
        SetModelPath(modelPath);

        const float scale = keyNode["scale"] ? keyNode["scale"].as<float>() : 0.25f;
        SetScale(glm::vec3(scale));
    }
}

void Key::AddCollectableComponent()
{
    std::unique_ptr<CollectableComponent> collectableComponent = std::make_unique<CollectableComponent>(this, 100);
    mCollectableComponent = collectableComponent.get();
    AddComponent(std::move(collectableComponent));
}

void Key::AddFocusComponent()
{
    std::unique_ptr<FocusComponent> focusComponent = std::make_unique<FocusComponent>(this, 100);
    mFocusComponent = focusComponent.get();
    AddComponent(std::move(focusComponent));
}

void Key::UpdateActor(float deltaTime)
{
    const bool isJustShown = !mIsActivePrev && mIsActive;
    if (isJustShown) {
        OnShown();
    }

    const bool shouldStartOnObtained = mCollectableComponent->GetIsObtained() && mIsActive;
    if (shouldStartOnObtained) {
        OnObtained();
    }

    mIsActivePrev = mIsActive;
}

void Key::OnShown() const
{
    mGame->GetAudioSystem()->PlaySE("show_key_se");
}

void Key::OnObtained()
{
    mIsActive = false;

    for (Boat* boat : mCurrentPlanet->GetBoats()) {
        if (!boat || !boat->GetIsActive()) {
            continue;
        }
        boat->GetFocusComponent()->StartFocus();
    }
}
