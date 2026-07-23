#pragma once

#include <string>
#include <unordered_map>

enum class PlayerAnimationPlaybackMode {
    BaseLoop,
    OneShot,
};

struct PlayerAnimationDefinition {
    std::string clipName;
    PlayerAnimationPlaybackMode playbackMode = PlayerAnimationPlaybackMode::OneShot;
};

using PlayerAnimationDefinitions = std::unordered_map<std::string, PlayerAnimationDefinition>;
