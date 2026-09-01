#include "TestSupport.h"

#include "system/camera/CameraShakeEffect.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

void AirStrongAttackStartsWithVerticalImpactAndReturnsToZero()
{
    CameraShakeEffect shake;

    ExpectTrue(
        shake.TryStart(CameraShakePattern::AirStrongAttackHit),
        "air strong attack shake starts");
    ExpectTrue(shake.IsActive(), "air strong attack shake is active");
    ExpectNear(0.0f, shake.GetLocalOffset().x, 0.0001f, "initial horizontal offset");
    ExpectNear(0.18f, shake.GetLocalOffset().y, 0.0001f, "initial vertical offset");

    shake.Update(0.016f);
    ExpectNear(
        0.18f,
        shake.GetLocalOffset().y,
        0.0001f,
        "initial vertical offset is preserved for the hit frame");

    shake.Update(0.16f);

    ExpectFalse(shake.IsActive(), "air strong attack shake finishes");
    ExpectNear(0.0f, shake.GetLocalOffset().x, 0.0001f, "finished horizontal offset");
    ExpectNear(0.0f, shake.GetLocalOffset().y, 0.0001f, "finished vertical offset");
}

void WeakerDamageShakeDoesNotReplaceActiveAirStrongAttackShake()
{
    CameraShakeEffect shake;
    shake.TryStart(CameraShakePattern::AirStrongAttackHit);
    const glm::vec2 airStrongOffset = shake.GetLocalOffset();

    ExpectFalse(
        shake.TryStart(CameraShakePattern::PlayerDamaged),
        "weaker damage shake is rejected");
    ExpectNear(
        airStrongOffset.x,
        shake.GetLocalOffset().x,
        0.0001f,
        "strong shake horizontal offset remains");
    ExpectNear(
        airStrongOffset.y,
        shake.GetLocalOffset().y,
        0.0001f,
        "strong shake vertical offset remains");
}

void RepeatedDamageShakeRestartsWithoutAddingOffsets()
{
    CameraShakeEffect shake;
    shake.TryStart(CameraShakePattern::PlayerDamaged);
    shake.Update(0.016f);
    shake.Update(0.10f);

    ExpectTrue(
        shake.TryStart(CameraShakePattern::PlayerDamaged),
        "same-strength damage shake restarts");
    const glm::vec2 restartedOffset = shake.GetLocalOffset();
    ExpectTrue(
        glm::length(restartedOffset) <= 0.12f,
        "restarted damage shake remains capped");
}

}

void RegisterCameraShakeEffectTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.emplace_back(
        "CameraShakeEffect.AirStrongAttackStartsWithVerticalImpactAndReturnsToZero",
        AirStrongAttackStartsWithVerticalImpactAndReturnsToZero);
    tests.emplace_back(
        "CameraShakeEffect.WeakerDamageShakeDoesNotReplaceActiveAirStrongAttackShake",
        WeakerDamageShakeDoesNotReplaceActiveAirStrongAttackShake);
    tests.emplace_back(
        "CameraShakeEffect.RepeatedDamageShakeRestartsWithoutAddingOffsets",
        RepeatedDamageShakeRestartsWithoutAddingOffsets);
}
