#pragma once

#include "app/Game.h"
#include "world/Stage.h"
#include "actor/Planet.h"
#include "system/MeshLoadSystem.h"
#include "system/actor_loader/ActorPlacementLoader.h"

#include <cstddef>
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <yaml-cpp/yaml.h>

class Actor;

// 配置・設定・初期化・Gameへの所有権移譲をまとめる、Actor生成の共通手順。
class StageActorFactory {
public:
    StageActorFactory(Game* game, const ActorPlacementLoader& placementLoader)
        : mGame(game),
          mPlacementLoader(placementLoader)
    {
    }

    // Gameへ所有権を移し、借用ポインターを返す。Stage・所属Planetを解決できなければnullptr。
    // 既定設定、ステージ上書き、上書き後処理、Initializeの順に適用する。
    template <class TActor>
    TActor*
    CreatePlacedActorFromStageNode(const YAML::Node& node, int stageYamlIndex, float defaultHeight,
                                   const glm::vec3& defaultScale, const std::string& defaultModelPath,
                                   const std::function<void(Planet*, TActor*)>& registerToPlanet,
                                   const std::function<void(TActor*, const YAML::Node&)>& applyConfig = {},
                                   const std::function<void(TActor*, const YAML::Node&)>& applyAfterStageOverride = {});

    // 指定配列がある場合だけPlanetの参照一覧をクリアして生成する。既存Actorの所有権は変更しない。
    template <class TActor>
    void LoadActorSequence(const YAML::Node& stageRoot, const std::string& sequenceName,
                           const std::function<void(Planet*)>& clearFromPlanet,
                           const std::function<TActor*(const YAML::Node&, int)>& createActor);

private:
    Game* mGame = nullptr;
    const ActorPlacementLoader& mPlacementLoader;
};

template <class TActor>
TActor* StageActorFactory::CreatePlacedActorFromStageNode(
    const YAML::Node& node, int stageYamlIndex, float defaultHeight, const glm::vec3& defaultScale,
    const std::string& defaultModelPath, const std::function<void(Planet*, TActor*)>& registerToPlanet,
    const std::function<void(TActor*, const YAML::Node&)>& applyConfig,
    const std::function<void(TActor*, const YAML::Node&)>& applyAfterStageOverride)
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return nullptr;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    const int currentPlanetNum = node["currentPlanetNum"] ? node["currentPlanetNum"].as<int>() : 0;

    if (currentPlanetNum < 0 || currentPlanetNum >= static_cast<int>(planets.size())) {
        return nullptr;
    }

    Planet* currentPlanet = planets[currentPlanetNum];

    if (!currentPlanet) {
        return nullptr;
    }

    std::unique_ptr<TActor> actor = std::make_unique<TActor>(mGame);

    actor->SetCurrentPlanet(currentPlanet);

    mPlacementLoader.ApplyPlacementFromStageNode(actor.get(), node, currentPlanet, stageYamlIndex, defaultHeight);
    mPlacementLoader.ApplyRotationFromStageNode(actor.get(), node);

    actor->SetScale(defaultScale);
    actor->SetModelPath(defaultModelPath);

    // 既定設定を先に適用し、ステージで指定されたモデル・スケールを優先する。
    if (applyConfig) {
        applyConfig(actor.get(), node);
    }

    if (node["modelPath"]) {
        actor->SetModelPath(node["modelPath"].as<std::string>());
    }

    mPlacementLoader.ApplyScaleFromStageNode(actor.get(), node);

    if (applyAfterStageOverride) {
        applyAfterStageOverride(actor.get(), node);
    }

    actor->Initialize();

    TActor* createdActor = actor.get();

    mGame->GetMeshLoadSystem()->SetActorMesh(createdActor);
    mGame->AddActor(std::move(actor));

    if (registerToPlanet) {
        registerToPlanet(currentPlanet, createdActor);
    }

    return createdActor;
}

template <class TActor>
void StageActorFactory::LoadActorSequence(const YAML::Node& stageRoot, const std::string& sequenceName,
                                          const std::function<void(Planet*)>& clearFromPlanet,
                                          const std::function<TActor*(const YAML::Node&, int)>& createActor)
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return;
    }

    if (!stageRoot[sequenceName] || !stageRoot[sequenceName].IsSequence()) {
        return;
    }

    for (Planet* planet : mGame->GetCurrentStage()->GetPlanets()) {
        if (planet && clearFromPlanet) {
            clearFromPlanet(planet);
        }
    }

    const YAML::Node sequence = stageRoot[sequenceName];

    for (std::size_t i = 0; i < sequence.size(); ++i) {
        TActor* actor = createActor(sequence[i], static_cast<int>(i));
        if (actor) {
            actor->SetStageSequenceName(sequenceName);
        }
    }
}
