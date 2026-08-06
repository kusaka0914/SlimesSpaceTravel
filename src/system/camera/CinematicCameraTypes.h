#pragma once

#include <glm/glm.hpp>

#include <string>
#include <vector>

enum class CameraEasing {
    Linear,
    EaseIn,
    EaseOut,
    EaseInOut,
};

struct CameraPose {
    glm::vec3 position{0.0f};
    glm::vec3 target{0.0f, 0.0f, 1.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    float fieldOfViewDegrees = 60.0f;
};

struct CinematicCameraKeyframe {
    float time = 0.0f;
    CameraPose pose;
    CameraEasing easing = CameraEasing::EaseInOut;
};

struct CinematicSequence {
    std::string id;
    std::string displayName;
    bool loop = false;
    float endHoldDuration = 0.5f;
    std::vector<CinematicCameraKeyframe> keyframes;
};
