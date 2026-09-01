#include "TestSupport.h"

#include "gfx/debug/stage/UGCPresetVisuals.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

void PlatformPresetVisualsUseTheirAssignedTextures()
{
    const UGCPresetVisual& normalPlatform =
        GetUGCPresetVisual(UGCPresetKind::NormalPlatform);
    const UGCPresetVisual& movingPlatform =
        GetUGCPresetVisual(UGCPresetKind::MovingPlatform);
    const UGCPresetVisual& fadingPlatform =
        GetUGCPresetVisual(UGCPresetKind::FadingPlatform);
    const UGCPresetVisual& adhesivePlatform =
        GetUGCPresetVisual(UGCPresetKind::AdhesivePlatform);

    ExpectEqual(
        std::string("textures/roadTex.png"),
        std::string(normalPlatform.initialTextureOverridePath),
        "normal platform texture");
    ExpectEqual(
        std::string("textures/roadTex.png"),
        std::string(movingPlatform.initialTextureOverridePath),
        "moving platform texture");
    ExpectEqual(
        std::string("textures/glass.png"),
        std::string(fadingPlatform.initialTextureOverridePath),
        "fading platform texture");
    ExpectEqual(
        std::string("textures/slimePlatform.png"),
        std::string(adhesivePlatform.initialTextureOverridePath),
        "adhesive platform texture");
}

void PlanetAndSwitchPresetVisualsUsePlacementScales()
{
    const UGCPresetVisual& ellipsePlanet =
        GetUGCPresetVisual(UGCPresetKind::EllipsePlanet);
    const UGCPresetVisual& pressureSwitch =
        GetUGCPresetVisual(UGCPresetKind::PressureSwitch);
    const UGCPresetVisual& twoPlayerSwitch =
        GetUGCPresetVisual(UGCPresetKind::TwoPlayerSwitch);

    ExpectTrue(
        ellipsePlanet.thumbnailScale == glm::vec3(4.0f, 1.0f, 4.0f),
        "ellipse planet scale");
    ExpectTrue(
        pressureSwitch.thumbnailScale == glm::vec3(0.75f, 0.2f, 0.75f),
        "pressure switch scale");
    ExpectTrue(
        twoPlayerSwitch.thumbnailScale == glm::vec3(0.75f, 0.2f, 0.75f),
        "two-player switch scale");
    ExpectEqual(
        std::string("textures/platform_switch_hold_off_red_platform_uv.png"),
        std::string(pressureSwitch.initialTextureOverridePath),
        "pressure switch texture");
    ExpectEqual(
        std::string("textures/platform_switch_two_player_off_red_platform_uv.png"),
        std::string(twoPlayerSwitch.initialTextureOverridePath),
        "two-player switch texture");
}

}

void RegisterUGCPresetVisualsTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.emplace_back(
        "Platform preset visuals use their assigned textures",
        PlatformPresetVisualsUseTheirAssignedTextures);
    tests.emplace_back(
        "Planet and switch preset visuals use placement scales",
        PlanetAndSwitchPresetVisualsUsePlacementScales);
}
