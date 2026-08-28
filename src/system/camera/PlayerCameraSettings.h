#pragma once

#include <string>

struct PlayerCameraSettings {
    float distance = 8.0f;
    float pitchDegrees = -57.29578f;
    float targetHeight = 1.5f;
    float splitScreenTargetHeight = 1.5f;
    float fieldOfViewDegrees = 60.0f;
    float splitScreenFieldOfViewDegrees = 55.0f;
    float yawSensitivity = 2.5f;
    float pitchSensitivityDegrees = 90.0f;
    float minPitchDegrees = -80.0f;
    float maxPitchDegrees = 0.0f;
    float upSmoothingSpeed = 8.0f;
    float targetSmoothingSpeed = 10.0f;
    float attackTargetSmoothingSpeed = 6.0f;

    float talkDistance = 4.5f;
    float talkPitchDegrees = -30.0f;
    float talkTargetHeight = 1.25f;
    float talkFieldOfViewDegrees = 55.0f;
    float talkTransitionInDuration = 0.6f;
    float talkTransitionOutDuration = 0.45f;

    float bossDefeatDistance = 6.0f;
    float bossDefeatCameraHeight = 1.5f;
    float bossDefeatTargetHeight = 0.5f;
    float bossDefeatFieldOfViewDegrees = 45.0f;
    float bossDefeatStarDistance = 6.0f;
    float bossDefeatStarCameraHeight = 1.5f;
    float bossDefeatStarTargetHeight = 0.35f;

    float boatRideDistance = 9.0f;
    float boatRideCameraHeight = 3.0f;
    float boatRideTargetHeight = 1.0f;
    float boatRideFieldOfViewDegrees = 55.0f;

    void Normalize();
};

class PlayerCameraSettingsRepository {
public:
    explicit PlayerCameraSettingsRepository(std::string filePath);

    bool Load(PlayerCameraSettings& settings) const;
    bool Save(const PlayerCameraSettings& settings) const;

private:
    std::string mFilePath;
};
