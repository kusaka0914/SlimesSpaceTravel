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

// プリセットの表示設定を返す。未知の種類には通常足場の設定を返す。
const UGCPresetVisual& GetUGCPresetVisual(UGCPresetKind presetKind);
const char* GetUGCPlatformTextureOverridePath(const std::string& platformBehavior);
