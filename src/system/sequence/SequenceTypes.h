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

// playersはPlayer配列、それ以外はステージYAMLの種別と0始まりの配置インデックスで識別する。
struct SequenceActorRef {
    std::string group;
    int index = -1;

    bool IsValid() const { return !group.empty() && index >= 0; }
};

// 時刻・継続時間は秒単位、移動位置はワールド座標。移動以外のクリップは開始時刻のイベントとして扱う。
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
    std::string displayName;
    bool loop = false;
    std::vector<SequenceClip> clips;

    float CalculateDuration() const
    {
        float duration = 0.0f;
        for (const SequenceClip& clip : clips) {
            // 移動以外は開始時刻で発火するため、durationを演出全体の長さへ加算しない。
            const float clipDuration = clip.type == SequenceClipType::ActorMove ? clip.duration : 0.0f;
            duration = std::max(duration, clip.startTime + clipDuration);
        }
        return duration;
    }
};
