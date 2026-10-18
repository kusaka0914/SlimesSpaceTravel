#include "system/camera/PlayerCameraSettings.h"

#include "utils/YamlRead.h"

#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <utility>

#include <glm/common.hpp>

void PlayerCameraSettings::Normalize()
{
    distance = glm::clamp(distance, 0.5f, 50.0f);
    pitchDegrees = glm::clamp(pitchDegrees, -89.0f, 89.0f);
    targetHeight = glm::clamp(targetHeight, -10.0f, 20.0f);
    splitScreenTargetHeight = glm::clamp(splitScreenTargetHeight, -10.0f, 20.0f);
    fieldOfViewDegrees = glm::clamp(fieldOfViewDegrees, 10.0f, 120.0f);
    splitScreenFieldOfViewDegrees = glm::clamp(splitScreenFieldOfViewDegrees, 10.0f, 120.0f);
    yawSensitivity = glm::clamp(yawSensitivity, 0.0f, 20.0f);
    pitchSensitivityDegrees = glm::clamp(pitchSensitivityDegrees, 0.0f, 360.0f);
    minPitchDegrees = glm::clamp(minPitchDegrees, -89.0f, 89.0f);
    maxPitchDegrees = glm::clamp(maxPitchDegrees, minPitchDegrees, 0.0f);
    pitchDegrees = glm::clamp(pitchDegrees, minPitchDegrees, maxPitchDegrees);
    upSmoothingSpeed = glm::clamp(upSmoothingSpeed, 0.0f, 50.0f);
    targetSmoothingSpeed = glm::clamp(targetSmoothingSpeed, 0.0f, 50.0f);
    attackTargetSmoothingSpeed = glm::clamp(attackTargetSmoothingSpeed, 0.0f, 50.0f);
    airSlamDistanceMultiplier = glm::clamp(airSlamDistanceMultiplier, 1.0f, 3.0f);
    airSlamPitchDegrees = glm::clamp(airSlamPitchDegrees, -89.0f, 0.0f);
    airSlamDistanceSmoothingSpeed = glm::clamp(airSlamDistanceSmoothingSpeed, 0.0f, 50.0f);
    airSlamReturnDelaySeconds = glm::clamp(airSlamReturnDelaySeconds, 0.0f, 10.0f);
    autoFollowDelaySeconds = glm::clamp(autoFollowDelaySeconds, 0.0f, 5.0f);
    autoFollowRotationDurationSeconds = glm::clamp(autoFollowRotationDurationSeconds, 0.05f, 5.0f);
    autoFollowMinimumLateralInput = glm::clamp(autoFollowMinimumLateralInput, 0.0f, 1.0f);
    autoFollowMaximumBackwardInput = glm::clamp(autoFollowMaximumBackwardInput, 0.0f, 1.0f);
    autoFollowDelayAfterManualInputSeconds = glm::clamp(autoFollowDelayAfterManualInputSeconds, 0.0f, 5.0f);
    backwardFacingLookAheadDistance = glm::clamp(backwardFacingLookAheadDistance, 0.0f, 50.0f);
    backwardFacingFramingActivationDelaySeconds = glm::clamp(backwardFacingFramingActivationDelaySeconds, 0.0f, 5.0f);
    backwardFacingFramingMinimumBackwardInput = glm::clamp(backwardFacingFramingMinimumBackwardInput, 0.0f, 1.0f);
    surfaceTraversalAutoAlignAngleDegrees = glm::clamp(surfaceTraversalAutoAlignAngleDegrees, 0.0f, 180.0f);
    surfaceTraversalAutoAlignMinimumMovementInput =
        glm::clamp(surfaceTraversalAutoAlignMinimumMovementInput, 0.0f, 1.0f);
    backwardFacingFramingStartAngleDegrees = glm::clamp(backwardFacingFramingStartAngleDegrees, 0.0f, 180.0f);
    backwardFacingFramingEndAngleDegrees =
        glm::clamp(backwardFacingFramingEndAngleDegrees, backwardFacingFramingStartAngleDegrees, 180.0f);
    backwardFacingFramingSmoothingSpeed = glm::clamp(backwardFacingFramingSmoothingSpeed, 0.0f, 50.0f);

    talkDistance = glm::clamp(talkDistance, 0.5f, 50.0f);
    talkPitchDegrees = glm::clamp(talkPitchDegrees, -89.0f, 89.0f);
    talkTargetHeight = glm::clamp(talkTargetHeight, -10.0f, 20.0f);
    talkFieldOfViewDegrees = glm::clamp(talkFieldOfViewDegrees, 10.0f, 120.0f);
    talkTransitionInDuration = glm::clamp(talkTransitionInDuration, 0.0f, 10.0f);
    talkTransitionOutDuration = glm::clamp(talkTransitionOutDuration, 0.0f, 10.0f);

    bossDefeatDistance = glm::clamp(bossDefeatDistance, 0.5f, 50.0f);
    bossDefeatCameraHeight = glm::clamp(bossDefeatCameraHeight, -20.0f, 20.0f);
    bossDefeatTargetHeight = glm::clamp(bossDefeatTargetHeight, -20.0f, 20.0f);
    bossDefeatFieldOfViewDegrees = glm::clamp(bossDefeatFieldOfViewDegrees, 10.0f, 120.0f);
    bossDefeatStarDistance = glm::clamp(bossDefeatStarDistance, 0.5f, 50.0f);
    bossDefeatStarCameraHeight = glm::clamp(bossDefeatStarCameraHeight, -20.0f, 20.0f);
    bossDefeatStarTargetHeight = glm::clamp(bossDefeatStarTargetHeight, -20.0f, 20.0f);

    boatRideDistance = glm::clamp(boatRideDistance, 0.5f, 100.0f);
    boatRideCameraHeight = glm::clamp(boatRideCameraHeight, -50.0f, 50.0f);
    boatRideTargetHeight = glm::clamp(boatRideTargetHeight, -50.0f, 50.0f);
    boatRideFieldOfViewDegrees = glm::clamp(boatRideFieldOfViewDegrees, 10.0f, 120.0f);
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

        loadedSettings.distance = YamlRead::ReadOr<float>(cameraNode, "distance", loadedSettings.distance);
        loadedSettings.pitchDegrees = YamlRead::ReadOr<float>(cameraNode, "pitchDegrees", loadedSettings.pitchDegrees);
        loadedSettings.targetHeight = YamlRead::ReadOr<float>(cameraNode, "targetHeight", loadedSettings.targetHeight);
        loadedSettings.splitScreenTargetHeight =
            YamlRead::ReadOr<float>(cameraNode, "splitScreenTargetHeight", loadedSettings.splitScreenTargetHeight);
        loadedSettings.fieldOfViewDegrees =
            YamlRead::ReadOr<float>(cameraNode, "fieldOfView", loadedSettings.fieldOfViewDegrees);
        loadedSettings.splitScreenFieldOfViewDegrees =
            YamlRead::ReadOr<float>(cameraNode, "splitScreenFieldOfView", loadedSettings.splitScreenFieldOfViewDegrees);
        loadedSettings.yawSensitivity =
            YamlRead::ReadOr<float>(cameraNode, "yawSensitivity", loadedSettings.yawSensitivity);
        loadedSettings.pitchSensitivityDegrees =
            YamlRead::ReadOr<float>(cameraNode, "pitchSensitivityDegrees", loadedSettings.pitchSensitivityDegrees);
        loadedSettings.minPitchDegrees =
            YamlRead::ReadOr<float>(cameraNode, "minPitchDegrees", loadedSettings.minPitchDegrees);
        loadedSettings.maxPitchDegrees =
            YamlRead::ReadOr<float>(cameraNode, "maxPitchDegrees", loadedSettings.maxPitchDegrees);
        loadedSettings.upSmoothingSpeed =
            YamlRead::ReadOr<float>(cameraNode, "upSmoothingSpeed", loadedSettings.upSmoothingSpeed);
        loadedSettings.targetSmoothingSpeed =
            YamlRead::ReadOr<float>(cameraNode, "targetSmoothingSpeed", loadedSettings.targetSmoothingSpeed);
        loadedSettings.attackTargetSmoothingSpeed = YamlRead::ReadOr<float>(cameraNode, "attackTargetSmoothingSpeed",
                                                                            loadedSettings.attackTargetSmoothingSpeed);
        loadedSettings.airSlamDistanceMultiplier =
            YamlRead::ReadOr<float>(cameraNode, "airSlamDistanceMultiplier", loadedSettings.airSlamDistanceMultiplier);
        loadedSettings.airSlamPitchDegrees =
            YamlRead::ReadOr<float>(cameraNode, "airSlamPitchDegrees", loadedSettings.airSlamPitchDegrees);
        loadedSettings.airSlamDistanceSmoothingSpeed = YamlRead::ReadOr<float>(
            cameraNode, "airSlamDistanceSmoothingSpeed", loadedSettings.airSlamDistanceSmoothingSpeed);
        loadedSettings.airSlamReturnDelaySeconds =
            YamlRead::ReadOr<float>(cameraNode, "airSlamReturnDelaySeconds", loadedSettings.airSlamReturnDelaySeconds);
        loadedSettings.autoFollowDelaySeconds =
            YamlRead::ReadOr<float>(cameraNode, "autoFollowDelaySeconds", loadedSettings.autoFollowDelaySeconds);
        loadedSettings.autoFollowRotationDurationSeconds = YamlRead::ReadOr<float>(
            cameraNode, "autoFollowRotationDurationSeconds", loadedSettings.autoFollowRotationDurationSeconds);
        loadedSettings.autoFollowMinimumLateralInput = YamlRead::ReadOr<float>(
            cameraNode, "autoFollowMinimumLateralInput", loadedSettings.autoFollowMinimumLateralInput);
        loadedSettings.autoFollowMaximumBackwardInput = YamlRead::ReadOr<float>(
            cameraNode, "autoFollowMaximumBackwardInput", loadedSettings.autoFollowMaximumBackwardInput);
        loadedSettings.autoFollowDelayAfterManualInputSeconds =
            YamlRead::ReadOr<float>(cameraNode, "autoFollowDelayAfterManualInputSeconds",
                                    loadedSettings.autoFollowDelayAfterManualInputSeconds);
        loadedSettings.backwardFacingLookAheadDistance = YamlRead::ReadOr<float>(
            cameraNode, "backwardFacingLookAheadDistance", loadedSettings.backwardFacingLookAheadDistance);
        loadedSettings.backwardFacingFramingActivationDelaySeconds =
            YamlRead::ReadOr<float>(cameraNode, "backwardFacingFramingActivationDelaySeconds",
                                    loadedSettings.backwardFacingFramingActivationDelaySeconds);
        loadedSettings.backwardFacingFramingMinimumBackwardInput =
            YamlRead::ReadOr<float>(cameraNode, "backwardFacingFramingMinimumBackwardInput",
                                    loadedSettings.backwardFacingFramingMinimumBackwardInput);
        loadedSettings.surfaceTraversalAutoAlignAngleDegrees = YamlRead::ReadOr<float>(
            cameraNode, "surfaceTraversalAutoAlignAngleDegrees", loadedSettings.surfaceTraversalAutoAlignAngleDegrees);
        loadedSettings.surfaceTraversalAutoAlignMinimumMovementInput =
            YamlRead::ReadOr<float>(cameraNode, "surfaceTraversalAutoAlignMinimumMovementInput",
                                    loadedSettings.surfaceTraversalAutoAlignMinimumMovementInput);
        loadedSettings.backwardFacingFramingStartAngleDegrees =
            YamlRead::ReadOr<float>(cameraNode, "backwardFacingFramingStartAngleDegrees",
                                    loadedSettings.backwardFacingFramingStartAngleDegrees);
        loadedSettings.backwardFacingFramingEndAngleDegrees = YamlRead::ReadOr<float>(
            cameraNode, "backwardFacingFramingEndAngleDegrees", loadedSettings.backwardFacingFramingEndAngleDegrees);
        loadedSettings.backwardFacingFramingSmoothingSpeed = YamlRead::ReadOr<float>(
            cameraNode, "backwardFacingFramingSmoothingSpeed", loadedSettings.backwardFacingFramingSmoothingSpeed);

        const YAML::Node talkNode = cameraNode["talk"];
        if (talkNode && talkNode.IsMap()) {
            loadedSettings.talkDistance = YamlRead::ReadOr<float>(talkNode, "distance", loadedSettings.talkDistance);
            loadedSettings.talkPitchDegrees =
                YamlRead::ReadOr<float>(talkNode, "pitchDegrees", loadedSettings.talkPitchDegrees);
            loadedSettings.talkTargetHeight =
                YamlRead::ReadOr<float>(talkNode, "targetHeight", loadedSettings.talkTargetHeight);
            loadedSettings.talkFieldOfViewDegrees =
                YamlRead::ReadOr<float>(talkNode, "fieldOfView", loadedSettings.talkFieldOfViewDegrees);
            loadedSettings.talkTransitionInDuration =
                YamlRead::ReadOr<float>(talkNode, "transitionInDuration", loadedSettings.talkTransitionInDuration);
            loadedSettings.talkTransitionOutDuration =
                YamlRead::ReadOr<float>(talkNode, "transitionOutDuration", loadedSettings.talkTransitionOutDuration);
        }

        const YAML::Node bossDefeatNode = cameraNode["bossDefeat"];
        if (bossDefeatNode && bossDefeatNode.IsMap()) {
            loadedSettings.bossDefeatDistance =
                YamlRead::ReadOr<float>(bossDefeatNode, "distance", loadedSettings.bossDefeatDistance);
            loadedSettings.bossDefeatCameraHeight =
                YamlRead::ReadOr<float>(bossDefeatNode, "cameraHeight", loadedSettings.bossDefeatCameraHeight);
            loadedSettings.bossDefeatTargetHeight =
                YamlRead::ReadOr<float>(bossDefeatNode, "targetHeight", loadedSettings.bossDefeatTargetHeight);
            loadedSettings.bossDefeatFieldOfViewDegrees =
                YamlRead::ReadOr<float>(bossDefeatNode, "fieldOfView", loadedSettings.bossDefeatFieldOfViewDegrees);
            loadedSettings.bossDefeatStarDistance =
                YamlRead::ReadOr<float>(bossDefeatNode, "starDistance", loadedSettings.bossDefeatStarDistance);
            loadedSettings.bossDefeatStarCameraHeight =
                YamlRead::ReadOr<float>(bossDefeatNode, "starCameraHeight", loadedSettings.bossDefeatStarCameraHeight);
            loadedSettings.bossDefeatStarTargetHeight =
                YamlRead::ReadOr<float>(bossDefeatNode, "starTargetHeight", loadedSettings.bossDefeatStarTargetHeight);
        }

        const YAML::Node boatRideNode = cameraNode["boatRide"];
        if (boatRideNode && boatRideNode.IsMap()) {
            loadedSettings.boatRideDistance =
                YamlRead::ReadOr<float>(boatRideNode, "distance", loadedSettings.boatRideDistance);
            loadedSettings.boatRideCameraHeight =
                YamlRead::ReadOr<float>(boatRideNode, "cameraHeight", loadedSettings.boatRideCameraHeight);
            loadedSettings.boatRideTargetHeight =
                YamlRead::ReadOr<float>(boatRideNode, "targetHeight", loadedSettings.boatRideTargetHeight);
            loadedSettings.boatRideFieldOfViewDegrees =
                YamlRead::ReadOr<float>(boatRideNode, "fieldOfView", loadedSettings.boatRideFieldOfViewDegrees);
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
        emitter << YAML::Key << "splitScreenTargetHeight" << YAML::Value << normalizedSettings.splitScreenTargetHeight;
        emitter << YAML::Key << "fieldOfView" << YAML::Value << normalizedSettings.fieldOfViewDegrees;
        emitter << YAML::Key << "splitScreenFieldOfView" << YAML::Value
                << normalizedSettings.splitScreenFieldOfViewDegrees;
        emitter << YAML::Key << "yawSensitivity" << YAML::Value << normalizedSettings.yawSensitivity;
        emitter << YAML::Key << "pitchSensitivityDegrees" << YAML::Value << normalizedSettings.pitchSensitivityDegrees;
        emitter << YAML::Key << "minPitchDegrees" << YAML::Value << normalizedSettings.minPitchDegrees;
        emitter << YAML::Key << "maxPitchDegrees" << YAML::Value << normalizedSettings.maxPitchDegrees;
        emitter << YAML::Key << "upSmoothingSpeed" << YAML::Value << normalizedSettings.upSmoothingSpeed;
        emitter << YAML::Key << "targetSmoothingSpeed" << YAML::Value << normalizedSettings.targetSmoothingSpeed;
        emitter << YAML::Key << "attackTargetSmoothingSpeed" << YAML::Value
                << normalizedSettings.attackTargetSmoothingSpeed;
        emitter << YAML::Key << "airSlamDistanceMultiplier" << YAML::Value
                << normalizedSettings.airSlamDistanceMultiplier;
        emitter << YAML::Key << "airSlamPitchDegrees" << YAML::Value << normalizedSettings.airSlamPitchDegrees;
        emitter << YAML::Key << "airSlamDistanceSmoothingSpeed" << YAML::Value
                << normalizedSettings.airSlamDistanceSmoothingSpeed;
        emitter << YAML::Key << "airSlamReturnDelaySeconds" << YAML::Value
                << normalizedSettings.airSlamReturnDelaySeconds;
        emitter << YAML::Key << "autoFollowDelaySeconds" << YAML::Value << normalizedSettings.autoFollowDelaySeconds;
        emitter << YAML::Key << "autoFollowRotationDurationSeconds" << YAML::Value
                << normalizedSettings.autoFollowRotationDurationSeconds;
        emitter << YAML::Key << "autoFollowMinimumLateralInput" << YAML::Value
                << normalizedSettings.autoFollowMinimumLateralInput;
        emitter << YAML::Key << "autoFollowMaximumBackwardInput" << YAML::Value
                << normalizedSettings.autoFollowMaximumBackwardInput;
        emitter << YAML::Key << "autoFollowDelayAfterManualInputSeconds" << YAML::Value
                << normalizedSettings.autoFollowDelayAfterManualInputSeconds;
        emitter << YAML::Key << "backwardFacingLookAheadDistance" << YAML::Value
                << normalizedSettings.backwardFacingLookAheadDistance;
        emitter << YAML::Key << "backwardFacingFramingActivationDelaySeconds" << YAML::Value
                << normalizedSettings.backwardFacingFramingActivationDelaySeconds;
        emitter << YAML::Key << "backwardFacingFramingMinimumBackwardInput" << YAML::Value
                << normalizedSettings.backwardFacingFramingMinimumBackwardInput;
        emitter << YAML::Key << "surfaceTraversalAutoAlignAngleDegrees" << YAML::Value
                << normalizedSettings.surfaceTraversalAutoAlignAngleDegrees;
        emitter << YAML::Key << "surfaceTraversalAutoAlignMinimumMovementInput" << YAML::Value
                << normalizedSettings.surfaceTraversalAutoAlignMinimumMovementInput;
        emitter << YAML::Key << "backwardFacingFramingStartAngleDegrees" << YAML::Value
                << normalizedSettings.backwardFacingFramingStartAngleDegrees;
        emitter << YAML::Key << "backwardFacingFramingEndAngleDegrees" << YAML::Value
                << normalizedSettings.backwardFacingFramingEndAngleDegrees;
        emitter << YAML::Key << "backwardFacingFramingSmoothingSpeed" << YAML::Value
                << normalizedSettings.backwardFacingFramingSmoothingSpeed;

        emitter << YAML::Key << "talk" << YAML::Value << YAML::BeginMap;
        emitter << YAML::Key << "distance" << YAML::Value << normalizedSettings.talkDistance;
        emitter << YAML::Key << "pitchDegrees" << YAML::Value << normalizedSettings.talkPitchDegrees;
        emitter << YAML::Key << "targetHeight" << YAML::Value << normalizedSettings.talkTargetHeight;
        emitter << YAML::Key << "fieldOfView" << YAML::Value << normalizedSettings.talkFieldOfViewDegrees;
        emitter << YAML::Key << "transitionInDuration" << YAML::Value << normalizedSettings.talkTransitionInDuration;
        emitter << YAML::Key << "transitionOutDuration" << YAML::Value << normalizedSettings.talkTransitionOutDuration;
        emitter << YAML::EndMap;

        emitter << YAML::Key << "bossDefeat" << YAML::Value << YAML::BeginMap;
        emitter << YAML::Key << "distance" << YAML::Value << normalizedSettings.bossDefeatDistance;
        emitter << YAML::Key << "cameraHeight" << YAML::Value << normalizedSettings.bossDefeatCameraHeight;
        emitter << YAML::Key << "targetHeight" << YAML::Value << normalizedSettings.bossDefeatTargetHeight;
        emitter << YAML::Key << "fieldOfView" << YAML::Value << normalizedSettings.bossDefeatFieldOfViewDegrees;
        emitter << YAML::Key << "starDistance" << YAML::Value << normalizedSettings.bossDefeatStarDistance;
        emitter << YAML::Key << "starCameraHeight" << YAML::Value << normalizedSettings.bossDefeatStarCameraHeight;
        emitter << YAML::Key << "starTargetHeight" << YAML::Value << normalizedSettings.bossDefeatStarTargetHeight;
        emitter << YAML::EndMap;

        emitter << YAML::Key << "boatRide" << YAML::Value << YAML::BeginMap;
        emitter << YAML::Key << "distance" << YAML::Value << normalizedSettings.boatRideDistance;
        emitter << YAML::Key << "cameraHeight" << YAML::Value << normalizedSettings.boatRideCameraHeight;
        emitter << YAML::Key << "targetHeight" << YAML::Value << normalizedSettings.boatRideTargetHeight;
        emitter << YAML::Key << "fieldOfView" << YAML::Value << normalizedSettings.boatRideFieldOfViewDegrees;
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
