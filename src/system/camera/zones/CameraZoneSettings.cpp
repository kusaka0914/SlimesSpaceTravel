#include "system/camera/zones/CameraZoneSettings.h"

#include <algorithm>
#include <cmath>

namespace {
glm::vec3 ReadVector(const YAML::Node& root, const char* name, glm::vec3 fallback)
{
    const auto node = root[name];
    if (!node || !node.IsSequence() || node.size() != 3) {
        return fallback;
    }
    glm::vec3 value(node[0].as<float>(), node[1].as<float>(), node[2].as<float>());
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z) ? value : fallback;
}
void WriteVector(YAML::Node root, const char* name, const glm::vec3& value)
{
    YAML::Node node(YAML::NodeType::Sequence);
    for (int axis = 0; axis < 3; ++axis) {
        node.push_back(value[axis]);
    }
    root[name] = node;
}
float ClampFinite(float value, float minimum, float maximum, float fallback)
{
    return std::isfinite(value) ? std::clamp(value, minimum, maximum) : fallback;
}
} // namespace

void CameraZoneViewSettings::Normalize()
{
    distance = ClampFinite(distance, 0.5f, 200.0f, 8.0f);
    pitchDegrees = ClampFinite(pitchDegrees, -85, 85, 20);
    fieldOfViewDegrees = ClampFinite(fieldOfViewDegrees, 15, 110, 60);
    targetWeight = ClampFinite(targetWeight, 0, 1, 0.35f);
    minimumDistance = ClampFinite(minimumDistance, 0.5f, 200, 5);
    maximumDistance = ClampFinite(maximumDistance, minimumDistance, 200, 40);
    framingMargin = ClampFinite(framingMargin, 1, 3, 1.2f);
    const auto finiteVector = [](glm::vec3 value) {
        return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
    };
    if (!finiteVector(referenceUp)) {
        referenceUp = {0, 1, 0};
    }
    if (!finiteVector(backDirection)) {
        backDirection = {0, 0, 1};
    }
    if (!finiteVector(lookOffset)) {
        lookOffset = {0, 1.5f, 0};
    }
    if (!finiteVector(fallbackTarget)) {
        fallbackTarget = {0, 0, 0};
    }
    if (!finiteVector(fixedPose.position)) {
        fixedPose.position = {0, 0, 8};
    }
    if (!finiteVector(fixedPose.target)) {
        fixedPose.target = {0, 0, 0};
    }
    if (glm::length(fixedPose.position - fixedPose.target) < 0.01f) {
        fixedPose.target = fixedPose.position + glm::vec3(0, 0, -1);
    }
    if (!finiteVector(fixedPose.up) || glm::dot(fixedPose.up, fixedPose.up) < 0.000001f) {
        fixedPose.up = {0, 1, 0};
    }
    if (glm::dot(referenceUp, referenceUp) < 0.000001f) {
        referenceUp = {0, 1, 0};
    }
    referenceUp = glm::normalize(referenceUp);
    backDirection -= referenceUp * glm::dot(backDirection, referenceUp);
    if (glm::dot(backDirection, backDirection) < 0.000001f) {
        backDirection =
            glm::cross(referenceUp, std::abs(referenceUp.x) < 0.9f ? glm::vec3(1, 0, 0) : glm::vec3(0, 0, 1));
    }
    backDirection = glm::normalize(backDirection);
}

void CameraZoneViewSettings::Load(const YAML::Node& node)
{
    if (!node || !node.IsMap()) {
        return;
    }
    const auto name = node["mode"].as<std::string>("Follow");
    mode = name == "Fixed" ? Mode::Fixed : name == "Target" ? Mode::Target : Mode::Follow;
    fixedTracksPlayer = node["fixedTracksPlayer"].as<decltype(fixedTracksPlayer)>(fixedTracksPlayer);
    distance = node["distance"].as<decltype(distance)>(distance);
    pitchDegrees = node["pitchDegrees"].as<decltype(pitchDegrees)>(pitchDegrees);
    fieldOfViewDegrees = node["fieldOfViewDegrees"].as<decltype(fieldOfViewDegrees)>(fieldOfViewDegrees);
    targetWeight = node["targetWeight"].as<decltype(targetWeight)>(targetWeight);
    minimumDistance = node["minimumDistance"].as<decltype(minimumDistance)>(minimumDistance);
    maximumDistance = node["maximumDistance"].as<decltype(maximumDistance)>(maximumDistance);
    framingMargin = node["framingMargin"].as<decltype(framingMargin)>(framingMargin);
    targetSequence = node["targetSequence"].as<decltype(targetSequence)>(targetSequence);
    targetIndex = node["targetIndex"].as<decltype(targetIndex)>(targetIndex);
    referenceUp = ReadVector(node, "referenceUp", referenceUp);
    backDirection = ReadVector(node, "backDirection", backDirection);
    lookOffset = ReadVector(node, "lookOffset", lookOffset);
    fallbackTarget = ReadVector(node, "fallbackTarget", fallbackTarget);
    fixedPose.position = ReadVector(node, "fixedPosition", fixedPose.position);
    fixedPose.target = ReadVector(node, "fixedTarget", fixedPose.target);
    fixedPose.up = ReadVector(node, "fixedUp", fixedPose.up);
    fixedPose.fieldOfViewDegrees = fieldOfViewDegrees;
    Normalize();
}

void CameraZoneViewSettings::Save(YAML::Node node) const
{
    auto value = *this;
    value.Normalize();
    node["mode"] = value.mode == Mode::Fixed ? "Fixed" : value.mode == Mode::Target ? "Target" : "Follow";
    node["fixedTracksPlayer"] = value.fixedTracksPlayer;
    node["distance"] = value.distance;
    node["pitchDegrees"] = value.pitchDegrees;
    node["fieldOfViewDegrees"] = value.fieldOfViewDegrees;
    node["targetWeight"] = value.targetWeight;
    node["minimumDistance"] = value.minimumDistance;
    node["maximumDistance"] = value.maximumDistance;
    node["framingMargin"] = value.framingMargin;
    node["targetSequence"] = value.targetSequence;
    node["targetIndex"] = value.targetIndex;
    WriteVector(node, "referenceUp", value.referenceUp);
    WriteVector(node, "backDirection", value.backDirection);
    WriteVector(node, "lookOffset", value.lookOffset);
    WriteVector(node, "fallbackTarget", value.fallbackTarget);
    if (mode == Mode::Fixed) {
        WriteVector(node, "fixedPosition", value.fixedPose.position);
        WriteVector(node, "fixedTarget", value.fixedPose.target);
        WriteVector(node, "fixedUp", value.fixedPose.up);
    } else {
        node.remove("fixedPosition");
        node.remove("fixedTarget");
        node.remove("fixedUp");
    }
}

void CameraZoneSettings::Normalize()
{
    CameraZoneViewSettings::Normalize();
    returnCamera.Normalize();
    enterSeconds = ClampFinite(enterSeconds, 0, 10, 0.7f);
    exitSeconds = ClampFinite(exitSeconds, 0, 10, 0.7f);
    exitMargin = ClampFinite(exitMargin, 0, 10, 0.4f);
    manualReturnDelay = ClampFinite(manualReturnDelay, 0, 10, 1.0f);
    directionSwitchDelay = ClampFinite(directionSwitchDelay, 0, 10, 0.4f);
    directionBlendSeconds = ClampFinite(directionBlendSeconds, 0, 10, 0.7f);
    if (!std::isfinite(travelDirection.x) || !std::isfinite(travelDirection.y) || !std::isfinite(travelDirection.z) ||
        glm::dot(travelDirection, travelDirection) < 0.000001f) {
        travelDirection = {0, 0, -1};
    }
    travelDirection = glm::normalize(travelDirection);
}

void CameraZoneSettings::Load(const YAML::Node& node)
{
    CameraZoneViewSettings::Load(node);
    if (!node || !node.IsMap()) {
        return;
    }
    priority = node["priority"].as<decltype(priority)>(priority);
    enabled = node["enabled"].as<decltype(enabled)>(enabled);
    holdWhileAirborne = node["holdWhileAirborne"].as<decltype(holdWhileAirborne)>(holdWhileAirborne);
    enterSeconds = node["enterSeconds"].as<decltype(enterSeconds)>(enterSeconds);
    exitSeconds = node["exitSeconds"].as<decltype(exitSeconds)>(exitSeconds);
    exitMargin = node["exitMargin"].as<decltype(exitMargin)>(exitMargin);
    allowManualRotation = node["allowManualRotation"].as<bool>(false);
    manualReturnDelay = node["manualReturnDelay"].as<decltype(manualReturnDelay)>(manualReturnDelay);
    bidirectional = node["bidirectional"].as<decltype(bidirectional)>(bidirectional);
    directionSwitchDelay = node["directionSwitchDelay"].as<decltype(directionSwitchDelay)>(directionSwitchDelay);
    directionBlendSeconds = node["directionBlendSeconds"].as<decltype(directionBlendSeconds)>(directionBlendSeconds);
    allowAirborneDirectionSwitch =
        node["allowAirborneDirectionSwitch"].as<decltype(allowAirborneDirectionSwitch)>(allowAirborneDirectionSwitch);
    defaultReturnCamera = node["defaultReturnCamera"].as<decltype(defaultReturnCamera)>(defaultReturnCamera);
    travelDirection = ReadVector(node, "travelDirection", travelDirection);
    returnCamera = static_cast<const CameraZoneViewSettings&>(*this);
    returnCamera.backDirection = -backDirection;
    returnCamera.Load(node["returnCamera"]);
    Normalize();
}

void CameraZoneSettings::Save(YAML::Node node) const
{
    auto value = *this;
    value.Normalize();
    value.CameraZoneViewSettings::Save(node);
    node["priority"] = value.priority;
    node["enabled"] = value.enabled;
    node["holdWhileAirborne"] = value.holdWhileAirborne;
    node["enterSeconds"] = value.enterSeconds;
    node["exitSeconds"] = value.exitSeconds;
    node["exitMargin"] = value.exitMargin;
    node["allowManualRotation"] = value.allowManualRotation;
    node["manualReturnDelay"] = value.manualReturnDelay;
    node["bidirectional"] = value.bidirectional;
    node["directionSwitchDelay"] = value.directionSwitchDelay;
    node["directionBlendSeconds"] = value.directionBlendSeconds;
    node["allowAirborneDirectionSwitch"] = value.allowAirborneDirectionSwitch;
    node["defaultReturnCamera"] = value.defaultReturnCamera;
    WriteVector(node, "travelDirection", value.travelDirection);
    value.returnCamera.Save(node["returnCamera"]);
}
