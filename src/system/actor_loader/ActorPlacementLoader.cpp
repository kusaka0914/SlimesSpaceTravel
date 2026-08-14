#include "system/actor_loader/ActorPlacementLoader.h"

#include "actor/Actor.h"
#include "actor/Planet.h"

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace {
bool HasVec3(const YAML::Node& node, const char* key)
{
    return node[key] && node[key].IsSequence() && node[key].size() >= 3;
}

bool HasVec2(const YAML::Node& node, const char* key)
{
    return node[key] && node[key].IsSequence() && node[key].size() >= 2;
}

bool HasQuat(const YAML::Node& node, const char* key)
{
    return node[key] && node[key].IsSequence() && node[key].size() >= 4;
}

glm::vec3 ReadVec3(const YAML::Node& node, const char* key, const glm::vec3& fallback)
{
    if (!HasVec3(node, key)) {
        return fallback;
    }

    return glm::vec3(node[key][0].as<float>(), node[key][1].as<float>(), node[key][2].as<float>());
}
} // namespace

glm::vec3 ActorPlacementLoader::CalculatePos(const YAML::Node& node, const Planet& currentPlanet) const
{
    if (HasVec3(node, "pos")) {
        const glm::vec3 localPos = ReadVec3(node, "pos", glm::vec3(0.0f));
        return currentPlanet.GetPos() + localPos;
    }

    const float theta = node["theta"] ? node["theta"].as<float>() : 0.0f;
    const float phi = node["phi"] ? node["phi"].as<float>() : 0.0f;
    const float height = node["height"] ? node["height"].as<float>() : 0.0f;

    glm::vec3 dir(std::cos(phi) * std::cos(theta), std::sin(phi), std::cos(phi) * std::sin(theta));

    const float len = glm::length(dir);
    if (len < 1e-6f) {
        dir = glm::vec3(1.0f, 0.0f, 0.0f);
    } else {
        dir /= len;
    }

    return currentPlanet.GetPos() + (currentPlanet.GetRadius() + height) * dir;
}

void ActorPlacementLoader::ApplyPlacementFromStageNode(Actor* actor, const YAML::Node& node, Planet* currentPlanet,
                                                       int stageYamlIndex, float defaultHeight) const
{
    if (!actor || !currentPlanet) {
        return;
    }

    actor->SetStageYamlIndex(stageYamlIndex);

    const bool isDebugDisabled =
        node["debugDisabled"]
            ? node["debugDisabled"].as<bool>()
            : false;
    actor->SetIsDebugDisabled(isDebugDisabled);

    const int visibleIfStageCleared =
        node["visibleIfStageCleared"]
            ? node["visibleIfStageCleared"].as<int>()
            : -1;
    actor->SetVisibleIfStageCleared(visibleIfStageCleared);
    const int hiddenIfStageCleared =
        node["hiddenIfStageCleared"]
            ? node["hiddenIfStageCleared"].as<int>()
            : -1;
    actor->SetHiddenIfStageCleared(hiddenIfStageCleared);
    actor->SetHiddenWhenRocketAppears(
        node["hiddenWhenRocketAppears"]
            ? node["hiddenWhenRocketAppears"].as<bool>()
            : false);

    const bool shouldAffectGravityDirection =
        node["affectsGravityDirection"]
            ? node["affectsGravityDirection"].as<bool>()
            : true;
    actor->SetShouldAffectGravityDirection(
        shouldAffectGravityDirection);
    actor->SetShouldReactToOverheadGravityRay(
        node["reactsToOverheadGravityRay"]
            ? node["reactsToOverheadGravityRay"].as<bool>()
            : false);

    const float theta = node["theta"] ? node["theta"].as<float>() : 0.0f;
    const float phi = node["phi"] ? node["phi"].as<float>() : 0.0f;
    const float height = node["height"] ? node["height"].as<float>() : defaultHeight;

    actor->SetSphericalPlacement(theta, phi, height);

    if (HasVec3(node, "pos")) {
        actor->SetPos(CalculatePos(node, *currentPlanet));
    } else {
        actor->SetPos(currentPlanet->CalculateSurfacePos(theta, phi, height));
    }
}

void ActorPlacementLoader::ApplyRotationFromStageNode(Actor* actor, const YAML::Node& node) const
{
    if (!actor) {
        return;
    }

    glm::vec3 editorRotation(0.0f);

    if (node["facingYaw"]) {
        editorRotation.y = node["facingYaw"].as<float>();
    }

    if (HasVec3(node, "rotation")) {
        editorRotation = ReadVec3(node, "rotation", editorRotation);
    }

    actor->SetEditorRotation(editorRotation);
    actor->SetFacingYaw(editorRotation.y);

    if (HasVec3(node, "upVec")) {
        const glm::vec3 upVec = ReadVec3(node, "upVec", glm::vec3(0.0f, 1.0f, 0.0f));

        if (glm::length(upVec) > 1e-6f) {
            actor->SetUpVec(glm::normalize(upVec));
        }
    }

    if (HasQuat(node, "rotationQuat")) {
        const glm::quat orientation(node["rotationQuat"][0].as<float>(), node["rotationQuat"][1].as<float>(),
                                    node["rotationQuat"][2].as<float>(), node["rotationQuat"][3].as<float>());
        if (glm::length(orientation) > 1e-6f) {
            actor->SetOrientation(glm::normalize(orientation));
        }
    }
}

void ActorPlacementLoader::ApplyScaleFromStageNode(Actor* actor, const YAML::Node& node) const
{
    if (!actor) {
        return;
    }

    if (HasVec3(node, "scale")) {
        const glm::vec3 currentScale = actor->GetScale();

        const float scaleX = node["scale"][0] ? node["scale"][0].as<float>() : currentScale.x;
        const float scaleY = node["scale"][1] ? node["scale"][1].as<float>() : currentScale.y;
        const float scaleZ = node["scale"][2] ? node["scale"][2].as<float>() : currentScale.z;

        actor->SetScale(glm::vec3(scaleX, scaleY, scaleZ));
    }

    if (HasVec2(node, "textureTiling")) {
        const glm::vec2 currentTiling = actor->GetTextureTiling();
        const float tilingX =
            node["textureTiling"][0] ? node["textureTiling"][0].as<float>() : currentTiling.x;
        const float tilingY =
            node["textureTiling"][1] ? node["textureTiling"][1].as<float>() : currentTiling.y;

        actor->SetTextureTiling(glm::max(glm::vec2(tilingX, tilingY), glm::vec2(0.01f)));
    }

    if (node["textureOverride"]) {
        actor->SetTextureOverridePath(node["textureOverride"].as<std::string>());
    }
}
