#include "Star.h"
#include "Game.h"
#include "actor/Player.h"
#include "component/CollectableComponent.h"
#include "system/ParticleSystem.h"

#include <cmath>
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
    if (!mIsActive) {
        mGlowEmitTimer = 0.0f;
        mSparkleEmitTimer = 0.0f;
        return;
    }

    ParticleSystem* particleSystem = mGame ? mGame->GetParticleSystem() : nullptr;

    mGlowEmitTimer -= deltaTime;
    if (mGlowEmitTimer <= 0.0f && particleSystem) {
        ParticleSpawnContext glowContext;
        glowContext.position = GetPos();
        glowContext.normal = GetUpVec();
        glowContext.direction = GetForwardVec();
        glowContext.scale = 1.0f;
        particleSystem->Emit("star_glow", glowContext);

        constexpr float glowEmitInterval = 0.5f;
        mGlowEmitTimer = glowEmitInterval;
    }

    mSparkleEmitTimer -= deltaTime;
    if (mSparkleEmitTimer <= 0.0f) {
        if (particleSystem) {
            mSparklePhase += 2.17f;

            const float ringRadius = 0.55f + std::sin(mSparklePhase * 1.37f) * 0.2f;
            const glm::vec3 ringOffset =
                GetForwardVec() * std::cos(mSparklePhase) * ringRadius +
                GetLeftVec() * std::sin(mSparklePhase) * ringRadius;
            const glm::vec3 heightOffset =
                GetUpVec() * std::sin(mSparklePhase * 1.71f) * 0.38f;

            ParticleSpawnContext context;
            context.position = GetPos() + ringOffset + heightOffset;
            context.normal = GetUpVec();
            context.direction = GetForwardVec();
            context.scale = 1.0f;
            particleSystem->Emit("star_sparkle", context);
        }

        constexpr float sparkleEmitInterval = 0.13f;
        mSparkleEmitTimer = sparkleEmitInterval;
    }

    const bool shouldStartOnObtained = mCollectableComponent->GetIsObtained() && mIsActive;
    if (shouldStartOnObtained) {
        OnObtained();
    }
}

void Star::OnObtained()
{
    mIsActive = false;

    Player* player = mGame ? mGame->GetMainPlayer() : nullptr;
    if (player) {
        player->MoveToCurrentPlanetOrigin();
    }

    mGame->OnStarObtained();
}
