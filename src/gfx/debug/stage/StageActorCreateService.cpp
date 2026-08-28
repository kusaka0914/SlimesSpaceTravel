#include "gfx/debug/stage/StageActorCreateService.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Planet.h"
#include "gfx/debug/stage/StagePlatformIdentifiers.h"
#include "gfx/debug/stage/StageYamlRepository.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace {

bool TryReadVec3(
    const YAML::Node& node,
    const char* key,
    glm::vec3& outValue)
{
    const YAML::Node value = node[key];
    if (!value || !value.IsSequence() || value.size() < 3) {
        return false;
    }

    try {
        outValue = glm::vec3(
            value[0].as<float>(),
            value[1].as<float>(),
            value[2].as<float>());
        return true;
    } catch (const YAML::Exception&) {
        return false;
    }
}

void ShiftVec3IfPresent(
    YAML::Node node,
    const char* key,
    const glm::vec3& offset)
{
    glm::vec3 value;
    if (!TryReadVec3(node, key, value)) {
        return;
    }

    value += offset;
    node[key][0] = value.x;
    node[key][1] = value.y;
    node[key][2] = value.z;
}

void ShiftPlatformMovementEndpoints(
    YAML::Node& platformNode,
    const glm::vec3& offset)
{
    ShiftVec3IfPresent(platformNode, "startLocalPos", offset);
    ShiftVec3IfPresent(platformNode, "endLocalPos", offset);

    const YAML::Node components = platformNode["components"];
    if (!components || !components.IsMap()) {
        return;
    }

    YAML::Node movement = components["movement"];
    if (!movement || !movement.IsMap()) {
        return;
    }

    ShiftVec3IfPresent(movement, "startLocalPos", offset);
    ShiftVec3IfPresent(movement, "endLocalPos", offset);
}


}

StageActorCreateService::StageActorCreateService(DebugEditorContext& context)
    : mContext(context),
      mNodeFactory(context),
      mRuntimeCreationService(context)
{
}

bool StageActorCreateService::CanCreateActor() const
{
    return mContext.game && mContext.game->GetCurrentStage() && mContext.game->GetActorLoadSystem();
}

bool StageActorCreateService::IsValidPlanetIndex(int planetIndex, const char* label) const
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return false;
    }

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

    if (planetIndex < 0 || planetIndex >= static_cast<int>(planets.size())) {
        std::cerr << "Invalid " << label << " planet index: " << planetIndex << std::endl;
        return false;
    }

    return true;
}

void StageActorCreateService::ApplyPlacementToNode(
    YAML::Node& node,
    int planetIndex,
    const StageActorPlacement* placement) const
{
    if (!placement || !IsValidPlanetIndex(planetIndex, "placement")) {
        return;
    }

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();
    const Planet* planet = planets[planetIndex];
    if (!planet) {
        return;
    }

    const glm::vec3 localPosition = placement->worldPosition - planet->GetPos();
    node["pos"][0] = localPosition.x;
    node["pos"][1] = localPosition.y;
    node["pos"][2] = localPosition.z;

    const float localDistance = glm::length(localPosition);
    if (localDistance > 1e-6f) {
        const glm::vec3 radialDirection = localPosition / localDistance;
        node["theta"] = std::atan2(radialDirection.z, radialDirection.x);
        node["phi"] = std::asin(std::clamp(radialDirection.y, -1.0f, 1.0f));
        node["height"] = localDistance - planet->GetRadius();
    }

    glm::vec3 surfaceNormal = placement->surfaceNormal;
    if (glm::length(surfaceNormal) < 1e-6f) {
        surfaceNormal = localDistance > 1e-6f
                            ? localPosition / localDistance
                            : glm::vec3(0.0f, 1.0f, 0.0f);
    }
    surfaceNormal = glm::normalize(surfaceNormal);
    node["upVec"][0] = surfaceNormal.x;
    node["upVec"][1] = surfaceNormal.y;
    node["upVec"][2] = surfaceNormal.z;
}

void StageActorCreateService::EnsureSequence(YAML::Node& config, const std::string& sequenceName) const
{
    if (!config[sequenceName] || !config[sequenceName].IsSequence()) {
        config[sequenceName] = YAML::Node(YAML::NodeType::Sequence);
    }
}

bool StageActorCreateService::DuplicateActorAtPlacement(
    const StageActorRef& sourceRef,
    const YAML::Node& sourceNode,
    int targetPlanetIndex,
    const StageActorPlacement& placement)
{
    if (!CanCreateActor() ||
        !sourceNode ||
        !sourceNode.IsMap() ||
        sourceRef.sequenceName.empty() ||
        !IsValidPlanetIndex(targetPlanetIndex, "duplicated actor")) {
        return false;
    }

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    EnsureSequence(config, sourceRef.sequenceName);

    YAML::Node duplicatedNode = YAML::Clone(sourceNode);
    glm::vec3 previousLocalPosition(0.0f);
    const bool hadPreviousLocalPosition =
        TryReadVec3(duplicatedNode, "pos", previousLocalPosition);

    if (sourceRef.type == StageActorType::Boat) {
        duplicatedNode["startPlanet"] = targetPlanetIndex;
        duplicatedNode.remove("currentPlanetNum");
    } else {
        duplicatedNode["currentPlanetNum"] = targetPlanetIndex;
    }

    ApplyPlacementToNode(
        duplicatedNode,
        targetPlanetIndex,
        &placement);




    // 保存済みQuaternionは複製元の地表法線を含む。クリック先の法線で再構成してもローカルの向きを保つため削除する。
    duplicatedNode.remove("rotationQuat");

    if (sourceRef.type == StageActorType::Platform) {
        duplicatedNode["platformId"] =
            StagePlatformIdentifiers::CreateUniqueId(config);

        glm::vec3 newLocalPosition(0.0f);
        if (hadPreviousLocalPosition &&
            TryReadVec3(duplicatedNode, "pos", newLocalPosition)) {
            ShiftPlatformMovementEndpoints(
                duplicatedNode,
                newLocalPosition - previousLocalPosition);
        }
    }

    YAML::Node targetSequence = config[sourceRef.sequenceName];
    const int newYamlIndex =
        static_cast<int>(targetSequence.size());
    targetSequence.push_back(duplicatedNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    if (!mRuntimeCreationService.CreateActor(
            sourceRef,
            duplicatedNode,
            newYamlIndex)) {
        return false;
    }

    mRuntimeCreationService.RefreshPhysicsWorld();
    return true;
}

bool StageActorCreateService::AddStageObject(
    int currentPlanetNum,
    const std::string& modelPath,
    bool collisionEnabled,
    const StageActorPlacement* placement)
{
    if (!CanCreateActor() || modelPath.empty()) {
        return false;
    }

    if (!IsValidPlanetIndex(currentPlanetNum, "stage object")) {
        return false;
    }

    YAML::Node config;
    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return false;
    }

    EnsureSequence(config, "stageObjects");

    const int index = static_cast<int>(config["stageObjects"].size());
    YAML::Node stageObjectNode =
        mNodeFactory.CreateStageObject(
            currentPlanetNum,
            modelPath,
            collisionEnabled);
    ApplyPlacementToNode(stageObjectNode, currentPlanetNum, placement);
    config["stageObjects"].push_back(stageObjectNode);

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
        return false;
    }

    mRuntimeCreationService.CreateActor(
        StageActorType::StageObject, stageObjectNode, index);
    mRuntimeCreationService.RefreshPhysicsWorld();
    return true;
}
