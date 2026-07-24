#pragma once

#include <glm/glm.hpp>

#include <algorithm>
#include <string>
#include <vector>

enum class SequenceClipType {
    ActorMove,
    ActorVisibility,
    PlayerControl,
    Camera,
};

enum class SequenceEasing {
    Linear,
    EaseIn,
    EaseOut,
    EaseInOut,
};

struct SequenceActorRef {
    std::string group;
    int index = -1;

    bool IsValid() const { return !group.empty() && index >= 0; }
};

struct SequenceClip {
    SequenceClipType type = SequenceClipType::ActorMove;
    float startTime = 0.0f;
    float duration = 1.0f;
    SequenceActorRef actor;

    glm::vec3 fromPosition{0.0f};
    glm::vec3 toPosition{0.0f};
    SequenceEasing easing = SequenceEasing::EaseInOut;

    bool visible = true;
    bool playerControlEnabled = true;
    std::string cameraSequenceId;
};

struct GameplaySequence {
    std::string id;
    bool loop = false;
    std::vector<SequenceClip> clips;

    float CalculateDuration() const
    {
        float duration = 0.0f;
        for (const SequenceClip& clip : clips) {
            const float clipDuration =
                clip.type == SequenceClipType::ActorMove ? clip.duration : 0.0f;
            duration = std::max(duration, clip.startTime + clipDuration);
        }
        return duration;
    }
};
