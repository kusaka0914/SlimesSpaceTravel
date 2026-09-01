#pragma once

#include <glm/glm.hpp>

#include <string>

enum class UGCPresetKind {
    EllipsePlanet,
    NormalPlatform,
    MovingPlatform,
    FadingPlatform,
    AdhesivePlatform,
    PressureSwitch,
    TwoPlayerSwitch,
    NormalEnemy,
    GoalStar,
};

struct UGCPresetVisual {
    const char* displayName;
    const char* modelPath;
    glm::vec3 thumbnailScale;
    const char* initialTextureOverridePath;
};

const UGCPresetVisual& GetUGCPresetVisual(UGCPresetKind presetKind);
const char* GetUGCPlatformTextureOverridePath(
    const std::string& platformBehavior);
