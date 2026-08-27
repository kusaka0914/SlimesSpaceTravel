#include "gfx/debug/stage/UGCPresetVisuals.h"

namespace {

const UGCPresetVisual ellipsePlanetVisual{
    "惑星", "planet.obj", glm::vec3(4.0f, 1.0f, 4.0f), ""};
const UGCPresetVisual normalPlatformVisual{
    "通常足場", "platform.obj", glm::vec3(1.0f, 0.2f, 1.0f),
    "textures/roadTex.png"};
const UGCPresetVisual movingPlatformVisual{
    "移動足場", "platform.obj", glm::vec3(1.0f, 0.2f, 1.0f),
    "textures/roadTex.png"};
const UGCPresetVisual fadingPlatformVisual{
    "消える足場", "platform.obj", glm::vec3(1.0f, 0.2f, 1.0f),
    "textures/glass.png"};
const UGCPresetVisual adhesivePlatformVisual{
    "くっつき足場", "platform.obj", glm::vec3(1.0f, 0.2f, 1.0f),
    "textures/slimePlatform.png"};
const UGCPresetVisual pressureSwitchVisual{
    "スイッチ", "platform.obj", glm::vec3(0.75f, 0.2f, 0.75f),
    "textures/platform_switch_hold_off_red_platform_uv.png"};
const UGCPresetVisual twoPlayerSwitchVisual{
    "2人用スイッチ", "platform.obj", glm::vec3(0.75f, 0.2f, 0.75f),
    "textures/platform_switch_two_player_off_red_platform_uv.png"};
const UGCPresetVisual normalEnemyVisual{
    "敵", "enemy.obj", glm::vec3(0.25f), ""};
const UGCPresetVisual goalStarVisual{
    "ゴール", "star.obj", glm::vec3(0.3f), ""};

}

const UGCPresetVisual& GetUGCPresetVisual(UGCPresetKind presetKind)
{
    switch (presetKind) {
    case UGCPresetKind::EllipsePlanet:
        return ellipsePlanetVisual;
    case UGCPresetKind::NormalPlatform:
        return normalPlatformVisual;
    case UGCPresetKind::MovingPlatform:
        return movingPlatformVisual;
    case UGCPresetKind::FadingPlatform:
        return fadingPlatformVisual;
    case UGCPresetKind::AdhesivePlatform:
        return adhesivePlatformVisual;
    case UGCPresetKind::PressureSwitch:
        return pressureSwitchVisual;
    case UGCPresetKind::TwoPlayerSwitch:
        return twoPlayerSwitchVisual;
    case UGCPresetKind::NormalEnemy:
        return normalEnemyVisual;
    case UGCPresetKind::GoalStar:
        return goalStarVisual;
    }

    return normalPlatformVisual;
}

const char* GetUGCPlatformTextureOverridePath(
    const std::string& platformBehavior)
{
    if (platformBehavior == "fading") {
        return fadingPlatformVisual.initialTextureOverridePath;
    }
    if (platformBehavior == "adhesive") {
        return adhesivePlatformVisual.initialTextureOverridePath;
    }
    return normalPlatformVisual.initialTextureOverridePath;
}
