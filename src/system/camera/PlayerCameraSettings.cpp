#include "system/camera/PlayerCameraSettings.h"

#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <utility>

#include <glm/common.hpp>

namespace {
float ReadFloat(const YAML::Node& node, const char* key, float fallback)
{
    const YAML::Node value = node[key];
    return value ? value.as<float>() : fallback;
}
} // namespace

void PlayerCameraSettings::Normalize()
{
    distance = glm::clamp(distance, 0.5f, 50.0f);
    pitchDegrees = glm::clamp(pitchDegrees, -89.0f, 89.0f);
    targetHeight = glm::clamp(targetHeight, -10.0f, 20.0f);
    fieldOfViewDegrees = glm::clamp(fieldOfViewDegrees, 10.0f, 120.0f);
    splitScreenFieldOfViewDegrees = glm::clamp(splitScreenFieldOfViewDegrees, 10.0f, 120.0f);
    yawSensitivity = glm::clamp(yawSensitivity, 0.0f, 20.0f);
    upSmoothingSpeed = glm::clamp(upSmoothingSpeed, 0.0f, 50.0f);
    targetSmoothingSpeed = glm::clamp(targetSmoothingSpeed, 0.0f, 50.0f);
    attackTargetSmoothingSpeed = glm::clamp(attackTargetSmoothingSpeed, 0.0f, 50.0f);

    talkDistance = glm::clamp(talkDistance, 0.5f, 50.0f);
    talkPitchDegrees = glm::clamp(talkPitchDegrees, -89.0f, 89.0f);
    talkTargetHeight = glm::clamp(talkTargetHeight, -10.0f, 20.0f);
    talkFieldOfViewDegrees = glm::clamp(talkFieldOfViewDegrees, 10.0f, 120.0f);
    talkTransitionInDuration = glm::clamp(talkTransitionInDuration, 0.0f, 10.0f);
    talkTransitionOutDuration = glm::clamp(talkTransitionOutDuration, 0.0f, 10.0f);
}

PlayerCameraSettingsRepository::PlayerCameraSettingsRepository(std::string filePath)
    : mFilePath(std::move(filePath))
{
}

bool PlayerCameraSettingsRepository::Load(PlayerCameraSettings& settings) const
{
    PlayerCameraSettings loadedSettings;

    try {
        if (!std::filesystem::exists(mFilePath)) {
            settings = loadedSettings;
            return true;
        }

        const YAML::Node root = YAML::LoadFile(mFilePath);
        const YAML::Node cameraNode = root["playerCamera"];

        if (!cameraNode || !cameraNode.IsMap()) {
            std::cerr << "Player camera settings must contain a 'playerCamera' map\n";
            return false;
        }

        loadedSettings.distance = ReadFloat(cameraNode, "distance", loadedSettings.distance);
        loadedSettings.pitchDegrees = ReadFloat(cameraNode, "pitchDegrees", loadedSettings.pitchDegrees);
        loadedSettings.targetHeight = ReadFloat(cameraNode, "targetHeight", loadedSettings.targetHeight);
        loadedSettings.fieldOfViewDegrees =
            ReadFloat(cameraNode, "fieldOfView", loadedSettings.fieldOfViewDegrees);
        loadedSettings.splitScreenFieldOfViewDegrees =
            ReadFloat(cameraNode, "splitScreenFieldOfView", loadedSettings.splitScreenFieldOfViewDegrees);
        loadedSettings.yawSensitivity =
            ReadFloat(cameraNode, "yawSensitivity", loadedSettings.yawSensitivity);
        loadedSettings.upSmoothingSpeed =
            ReadFloat(cameraNode, "upSmoothingSpeed", loadedSettings.upSmoothingSpeed);
        loadedSettings.targetSmoothingSpeed =
            ReadFloat(cameraNode, "targetSmoothingSpeed", loadedSettings.targetSmoothingSpeed);
        loadedSettings.attackTargetSmoothingSpeed =
            ReadFloat(cameraNode, "attackTargetSmoothingSpeed", loadedSettings.attackTargetSmoothingSpeed);

        const YAML::Node talkNode = cameraNode["talk"];
        if (talkNode && talkNode.IsMap()) {
            loadedSettings.talkDistance = ReadFloat(talkNode, "distance", loadedSettings.talkDistance);
            loadedSettings.talkPitchDegrees =
                ReadFloat(talkNode, "pitchDegrees", loadedSettings.talkPitchDegrees);
            loadedSettings.talkTargetHeight =
                ReadFloat(talkNode, "targetHeight", loadedSettings.talkTargetHeight);
            loadedSettings.talkFieldOfViewDegrees =
                ReadFloat(talkNode, "fieldOfView", loadedSettings.talkFieldOfViewDegrees);
            loadedSettings.talkTransitionInDuration =
                ReadFloat(talkNode, "transitionInDuration", loadedSettings.talkTransitionInDuration);
            loadedSettings.talkTransitionOutDuration =
                ReadFloat(talkNode, "transitionOutDuration", loadedSettings.talkTransitionOutDuration);
        }

        loadedSettings.Normalize();
        settings = loadedSettings;
        return true;
    } catch (const std::exception& exception) {
        std::cerr << "Failed to load player camera settings: " << exception.what() << '\n';
        return false;
    }
}

bool PlayerCameraSettingsRepository::Save(const PlayerCameraSettings& settings) const
{
    try {
        const std::filesystem::path outputPath(mFilePath);
        if (outputPath.has_parent_path()) {
            std::filesystem::create_directories(outputPath.parent_path());
        }

        PlayerCameraSettings normalizedSettings = settings;
        normalizedSettings.Normalize();

        YAML::Emitter emitter;
        emitter << YAML::BeginMap;
        emitter << YAML::Key << "playerCamera" << YAML::Value << YAML::BeginMap;
        emitter << YAML::Key << "distance" << YAML::Value << normalizedSettings.distance;
        emitter << YAML::Key << "pitchDegrees" << YAML::Value << normalizedSettings.pitchDegrees;
        emitter << YAML::Key << "targetHeight" << YAML::Value << normalizedSettings.targetHeight;
        emitter << YAML::Key << "fieldOfView" << YAML::Value << normalizedSettings.fieldOfViewDegrees;
        emitter << YAML::Key << "splitScreenFieldOfView" << YAML::Value
                << normalizedSettings.splitScreenFieldOfViewDegrees;
        emitter << YAML::Key << "yawSensitivity" << YAML::Value << normalizedSettings.yawSensitivity;
        emitter << YAML::Key << "upSmoothingSpeed" << YAML::Value << normalizedSettings.upSmoothingSpeed;
        emitter << YAML::Key << "targetSmoothingSpeed" << YAML::Value
                << normalizedSettings.targetSmoothingSpeed;
        emitter << YAML::Key << "attackTargetSmoothingSpeed" << YAML::Value
                << normalizedSettings.attackTargetSmoothingSpeed;

        emitter << YAML::Key << "talk" << YAML::Value << YAML::BeginMap;
        emitter << YAML::Key << "distance" << YAML::Value << normalizedSettings.talkDistance;
        emitter << YAML::Key << "pitchDegrees" << YAML::Value << normalizedSettings.talkPitchDegrees;
        emitter << YAML::Key << "targetHeight" << YAML::Value << normalizedSettings.talkTargetHeight;
        emitter << YAML::Key << "fieldOfView" << YAML::Value
                << normalizedSettings.talkFieldOfViewDegrees;
        emitter << YAML::Key << "transitionInDuration" << YAML::Value
                << normalizedSettings.talkTransitionInDuration;
        emitter << YAML::Key << "transitionOutDuration" << YAML::Value
                << normalizedSettings.talkTransitionOutDuration;
        emitter << YAML::EndMap;

        emitter << YAML::EndMap;
        emitter << YAML::EndMap;

        std::ofstream output(mFilePath);
        if (!output) {
            return false;
        }

        output << emitter.c_str() << '\n';
        return static_cast<bool>(output);
    } catch (const std::exception& exception) {
        std::cerr << "Failed to save player camera settings: " << exception.what() << '\n';
        return false;
    }
}
