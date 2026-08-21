#include "Star.h"
#include "Game.h"
#include "actor/Player.h"
#include "component/CollectableComponent.h"
#include "system/ParticleSystem.h"

#include <cmath>
#include <fstream>
#include <glm/gtc/constants.hpp>
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

        const YAML::Node collection = starNode["collectionAnimation"];
        if (collection) {
            mCollectionSettings.orbitDuration = collection["orbitDuration"].as<float>(mCollectionSettings.orbitDuration);
            mCollectionSettings.orbitStartRadius = collection["orbitStartRadius"].as<float>(mCollectionSettings.orbitStartRadius);
            mCollectionSettings.orbitSpinDegreesPerSecond = collection["orbitSpinDegreesPerSecond"].as<float>(mCollectionSettings.orbitSpinDegreesPerSecond);
            mCollectionSettings.finalHeight = collection["finalHeight"].as<float>(mCollectionSettings.finalHeight);
            mCollectionSettings.waitAbovePlayerDuration = collection["waitAbovePlayerDuration"].as<float>(mCollectionSettings.waitAbovePlayerDuration);
            mCollectionSettings.fallDuration = collection["fallDuration"].as<float>(mCollectionSettings.fallDuration);
        }
    }
}

bool Star::SaveCollectionAnimationSettings() const
{
    YAML::Emitter emitter;
    emitter << YAML::BeginMap << YAML::Key << "stars" << YAML::Value << YAML::BeginSeq;
    emitter << YAML::BeginMap;
    emitter << YAML::Key << "modelPath" << YAML::Value << GetModelPath();
    emitter << YAML::Key << "scale" << YAML::Value << GetScale().x;
    emitter << YAML::Key << "collectionAnimation" << YAML::Value << YAML::BeginMap;
    emitter << YAML::Key << "orbitDuration" << YAML::Value << mCollectionSettings.orbitDuration;
    emitter << YAML::Key << "orbitStartRadius" << YAML::Value << mCollectionSettings.orbitStartRadius;
    emitter << YAML::Key << "orbitSpinDegreesPerSecond" << YAML::Value << mCollectionSettings.orbitSpinDegreesPerSecond;
    emitter << YAML::Key << "finalHeight" << YAML::Value << mCollectionSettings.finalHeight;
    emitter << YAML::Key << "waitAbovePlayerDuration" << YAML::Value << mCollectionSettings.waitAbovePlayerDuration;
    emitter << YAML::Key << "fallDuration" << YAML::Value << mCollectionSettings.fallDuration;
    emitter << YAML::EndMap << YAML::EndMap << YAML::EndSeq << YAML::EndMap;

    std::ofstream file("../assets/data/actor/stars.yaml");
    if (!file) {
        return false;
    }
    file << emitter.c_str();
    return file.good();
}

bool Star::StartCollectionPreview(Player* player)
{
    if (!player || mCollectionState != CollectionState::Waiting) {
        return false;
    }

    mPreviewOriginalActive = mIsActive;
    mPreviewOriginalPos = GetPos();
    mPreviewOriginalUp = GetUpVec();
    mPreviewOriginalFacingYaw = GetFacingYaw();
    mIsActive = true;
    mObtainingPlayer = player;
    mCollectionTimer = 0.0f;
    mCollectionState = CollectionState::Orbiting;
    mIsCollectionPreview = true;
    mCollectionBaseFacingYaw = GetFacingYaw();
    return true;
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

    if (mCollectionState != CollectionState::Waiting) {
        UpdateCollectionAnimation(deltaTime);
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
    if (mCollectionState != CollectionState::Waiting) {
        return;
    }

    mObtainingPlayer = mCollectableComponent
        ? mCollectableComponent->GetObtainingPlayer()
        : nullptr;
    if (!mObtainingPlayer && mGame) {
        mObtainingPlayer = mGame->GetMainPlayer();
    }

    // Keep the star active while it circles the player.  The collectable
    // component is already marked obtained, so it cannot be picked up twice.
    mCollectionState = CollectionState::Orbiting;
    mCollectionTimer = 0.0f;
    mCollectionBaseFacingYaw = GetFacingYaw();

    // The stage-clear screen starts as soon as the player reaches the star.
    // The star actor remains active independently for its collection animation.
    if (mGame) {
        mGame->OnStarObtained();
    }
}

void Star::UpdateCollectionAnimation(float deltaTime)
{
    if (!mObtainingPlayer || !mObtainingPlayer->GetIsActive()) {
        FinishCollection();
        return;
    }

    glm::vec3 up = mObtainingPlayer->GetUpVec();
    if (glm::dot(up, up) < 0.000001f) {
        up = glm::vec3(0.0f, 1.0f, 0.0f);
    } else {
        up = glm::normalize(up);
    }

    glm::vec3 forward = mObtainingPlayer->GetForwardVec();
    forward -= up * glm::dot(forward, up);
    if (glm::dot(forward, forward) < 0.000001f) {
        forward = glm::vec3(0.0f, 0.0f, -1.0f);
    } else {
        forward = glm::normalize(forward);
    }
    glm::vec3 left = glm::normalize(glm::cross(up, forward));

    const float orbitDuration = std::max(0.01f, mCollectionSettings.orbitDuration);
    const float orbitStartRadius = std::max(0.0f, mCollectionSettings.orbitStartRadius);
    const float orbitSpinRadiansPerSecond = glm::radians(
        mCollectionSettings.orbitSpinDegreesPerSecond);
    const float finalHeight = std::max(0.0f, mCollectionSettings.finalHeight);
    const float waitAbovePlayerDuration = std::max(0.0f, mCollectionSettings.waitAbovePlayerDuration);
    const float fallDuration = std::max(0.01f, mCollectionSettings.fallDuration);

    mCollectionTimer += std::max(0.0f, deltaTime);
    const glm::vec3 playerPos = mObtainingPlayer->GetPos();

    if (mCollectionState == CollectionState::Orbiting) {
        const float progress = glm::clamp(mCollectionTimer / orbitDuration, 0.0f, 1.0f);
        const float angle = glm::two_pi<float>() * progress;
        const float radius = orbitStartRadius * (1.0f - progress);
        const float height = finalHeight * progress;
        SetPos(playerPos +
               (forward * std::cos(angle) + left * std::sin(angle)) * radius +
               up * height);
        SetUpVec(up);
        SetFacingYaw(
            mCollectionBaseFacingYaw + orbitSpinRadiansPerSecond * mCollectionTimer);

        if (progress >= 1.0f) {
            mCollectionState = CollectionState::WaitingAbovePlayer;
            mCollectionTimer = 0.0f;
        }
        return;
    }

    if (mCollectionState == CollectionState::WaitingAbovePlayer) {
        SetPos(playerPos + up * finalHeight);
        SetUpVec(up);
        SetFacingYaw(mCollectionBaseFacingYaw);
        if (mCollectionTimer >= waitAbovePlayerDuration) {
            mCollectionState = CollectionState::Falling;
            mCollectionTimer = 0.0f;
        }
        return;
    }

    const float progress = glm::clamp(mCollectionTimer / fallDuration, 0.0f, 1.0f);
    SetPos(playerPos + up * glm::mix(finalHeight, 0.2f, progress));
    SetUpVec(up);
    SetFacingYaw(mCollectionBaseFacingYaw);
    if (progress >= 1.0f) {
        FinishCollection();
    }
}

void Star::FinishCollection()
{
    if (mIsCollectionPreview) {
        SetPos(mPreviewOriginalPos);
        SetUpVec(mPreviewOriginalUp);
        SetFacingYaw(mPreviewOriginalFacingYaw);
        mIsActive = mPreviewOriginalActive;
        mIsCollectionPreview = false;
        mCollectionState = CollectionState::Waiting;
        mCollectionTimer = 0.0f;
        mObtainingPlayer = nullptr;
        return;
    }

    mIsActive = false;
    mCollectionState = CollectionState::Waiting;
    mCollectionTimer = 0.0f;
    mObtainingPlayer = nullptr;

}
