#include "system/actor_loader/StageBoatCreator.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Boat.h"
#include "actor/BoatArrivalPoint.h"
#include "actor/Planet.h"
#include "system/MeshLoadSystem.h"
#include "system/actor_loader/ActorPlacementLoader.h"

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <utility>
#include <yaml-cpp/yaml.h>

StageBoatCreator::StageBoatCreator(
    Game* game,
    const ActorPlacementLoader& placementLoader)
    : mGame(game),
      mPlacementLoader(placementLoader)
{
}

Boat* StageBoatCreator::CreateFromStageNode(const YAML::Node& node, int stageYamlIndex)
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return nullptr;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();

    const int startPlanetNum = node["startPlanet"] ? node["startPlanet"].as<int>() : 0;
    const int destPlanetNum = node["destPlanet"] ? node["destPlanet"].as<int>() : 0;

    if (startPlanetNum < 0 || startPlanetNum >= static_cast<int>(planets.size())) {
        return nullptr;
    }

    if (destPlanetNum < 0 || destPlanetNum >= static_cast<int>(planets.size())) {
        return nullptr;
    }

    Planet* currentPlanet = planets[startPlanetNum];
    Planet* destPlanet = planets[destPlanetNum];

    if (!currentPlanet || !destPlanet) {
        return nullptr;
    }

    std::unique_ptr<Boat> boat = std::make_unique<Boat>(mGame);

    boat->SetCurrentPlanet(currentPlanet);
    boat->SetDestPlanet(destPlanet);

    const int arrivalPointIndex = node["arrivalPointIndex"] ? node["arrivalPointIndex"].as<int>() : -1;

    if (arrivalPointIndex >= 0) {
        for (BoatArrivalPoint* point : destPlanet->GetBoatArrivalPoints()) {
            if (point && point->GetStageYamlIndex() == arrivalPointIndex) {
                boat->SetArrivalPoint(point);
                break;
            }
        }
    }

    const int destStage = node["destStage"] ? node["destStage"].as<int>() : 0;
    boat->SetDestStage(destStage);

    const bool hasTravelSpeed = static_cast<bool>(node["travelSpeed"]);
    const float travelSpeed =
        hasTravelSpeed ? node["travelSpeed"].as<float>() : 10.0f;
    const bool hasLegacyTravelDuration =
        static_cast<bool>(node["travelDuration"]);
    const float legacyTravelDuration =
        hasLegacyTravelDuration
            ? node["travelDuration"].as<float>()
            : 3.0f;

    const float destMargin =
        node["destMargin"] ? node["destMargin"].as<float>() : 4.0f;
    boat->SetDestMargin(destMargin);

    const std::string launchSequenceId =
        node["launchSequenceId"]
            ? node["launchSequenceId"].as<std::string>()
            : std::string("launch_rocket_from_base");
    boat->SetLaunchSequenceId(launchSequenceId);

    const float facingYaw = node["facingYaw"] ? node["facingYaw"].as<float>() : 0.0f;
    boat->SetFacingYaw(facingYaw);

    mPlacementLoader.ApplyPlacementFromStageNode(boat.get(), node, currentPlanet, stageYamlIndex, 1.0f);
    mPlacementLoader.ApplyRotationFromStageNode(boat.get(), node);

    YAML::Node boatRoot = YAML::LoadFile("../assets/data/actor/boats.yaml");
    for (const YAML::Node& boatNode : boatRoot["boats"]) {
        const std::string modelPath = boatNode["modelPath"] ? boatNode["modelPath"].as<std::string>() : "";
        boat->SetModelPath(modelPath);

        const float scale = boatNode["scale"] ? boatNode["scale"].as<float>() : 0.25f;
        boat->SetScale(glm::vec3(scale));
    }

    if (node["modelPath"]) {
        boat->SetModelPath(node["modelPath"].as<std::string>());
    }

    mPlacementLoader.ApplyScaleFromStageNode(boat.get(), node);

    boat->Initialize();
    if (hasTravelSpeed) {
        boat->SetTravelSpeed(travelSpeed);
    } else if (hasLegacyTravelDuration) {
        // 旧ステージデータは初回保存まで従来の所要時間を維持する。
        boat->SetTravelSpeedFromLegacyDuration(
            legacyTravelDuration);
    }

    Boat* boatPtr = boat.get();
    mGame->GetMeshLoadSystem()->SetActorMesh(boatPtr);
    mGame->AddActor(std::move(boat));
    currentPlanet->AddBoat(boatPtr);

    return boatPtr;
}


