#pragma once

#include <string>

struct PlayerCameraSettings {
    float distance = 8.0f;
    float pitchDegrees = -57.29578f;
    float targetHeight = 1.5f;
    float fieldOfViewDegrees = 60.0f;
    float splitScreenFieldOfViewDegrees = 45.0f;
    float yawSensitivity = 2.5f;
    float upSmoothingSpeed = 8.0f;
    float targetSmoothingSpeed = 10.0f;
    float attackTargetSmoothingSpeed = 6.0f;

    float talkDistance = 4.5f;
    float talkPitchDegrees = -30.0f;
    float talkTargetHeight = 1.25f;
    float talkFieldOfViewDegrees = 55.0f;
    float talkTransitionInDuration = 0.6f;
    float talkTransitionOutDuration = 0.45f;

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
