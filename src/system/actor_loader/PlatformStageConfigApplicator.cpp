#include "system/actor_loader/PlatformStageConfigApplicator.h"

#include "actor/Planet.h"
#include "actor/Platform.h"
#include "component/PlatformBehaviorComponents.h"
#include "component/PlatformMovementComponent.h"

namespace {

void ApplyMovementConfig(
    Platform& platform,
    const PlatformMovementStageConfig& config)
{
    PlatformMovementComponent* movement =
        platform.AddMovementComponent();

    glm::vec3 baseLocalPos(0.0f);
    if (platform.GetCurrentPlanet()) {
        baseLocalPos =
            platform.GetPos() - platform.GetCurrentPlanet()->GetPos();
    }
    if (config.startLocalPos) {
        baseLocalPos = *config.startLocalPos;
    }

    movement->SetBaseLocalPos(baseLocalPos);
    movement->SetMoveOffset(config.moveOffset);
    if (config.endLocalPos) {
        movement->SetDestinationLocalPos(*config.endLocalPos);
    }
    movement->SetMoveDuration(config.moveDurationSeconds);
    movement->SetMoveOnPlayer(config.shouldMoveOnPlayer);
    movement->SetReturnDelay(config.returnDelaySeconds);
    movement->SetEndpointWaitDurationSeconds(
        config.endpointWaitSeconds);
}

void ApplyBehaviorConfig(
    Platform& platform,
    const PlatformBehaviorStageConfig& config)
{
    if (config.fadeOnStand) {
        PlatformFadeOnStandComponent* component =
            platform.AddFadeOnStandComponent();
        component->SetFadeOutDuration(
            config.fadeOnStand->fadeOutDurationSeconds);
        component->SetReappearDelay(
            config.fadeOnStand->reappearDelaySeconds);
    }

    if (config.jumpToggleInitiallyVisible) {
        platform.AddJumpToggleComponent()->SetInitiallyVisible(
            *config.jumpToggleInitiallyVisible);
    }

    if (config.intervalToggle) {
        PlatformIntervalToggleComponent* component =
            platform.AddIntervalToggleComponent();
        component->SetInitiallyVisible(
            config.intervalToggle->isInitiallyVisible);
        component->SetInterval(
            config.intervalToggle->intervalSeconds);
        component->SetWarningDuration(
            config.intervalToggle->warningDurationSeconds);
        component->SetBlinkInterval(
            config.intervalToggle->blinkIntervalSeconds);
    }

    if (config.directionalMovementSpeed) {
        platform.AddDirectionalMovementComponent()->SetSpeed(
            *config.directionalMovementSpeed);
    }

    if (config.rotation) {
        PlatformRotationComponent* component =
            platform.AddRotationComponent();
        component->SetLocalAxis(config.rotation->localAxis);
        component->SetDegreesPerSecond(
            config.rotation->degreesPerSecond);
    }

    if (config.conveyor) {
        PlatformConveyorComponent* component =
            platform.AddConveyorComponent();
        component->SetLocalDirection(
            config.conveyor->localDirection);
        component->SetSpeed(config.conveyor->speed);
    }

    if (config.hasAdhesion) {
        platform.AddAdhesionComponent();
    }

    if (config.pressureSwitch) {
        PlatformPressureSwitchComponent* component =
            platform.AddPressureSwitchComponent();
        component->SetShouldRemainOnAfterPressed(
            config.pressureSwitch->shouldRemainOnAfterPressed);
        component->SetInactiveOpacity(
            config.pressureSwitch->inactiveOpacity);
        component->SetTargetPlatformIds(
            config.pressureSwitch->targetPlatformIds);
        component->SetTargetEnemyRefs(
            config.pressureSwitch->targetEnemyRefs);
        component->SetHideTargets(
            config.pressureSwitch->hideTargets);
    }

    if (config.hasEnemyClearUnlock) {
        platform.AddEnemyClearUnlockComponent();
    }

    if (config.latchedGroupSwitch) {
        PlatformLatchedGroupSwitchComponent* component =
            platform.AddLatchedGroupSwitchComponent();
        component->SetGroupId(config.latchedGroupSwitch->groupId);
        component->SetRevealTargets(
            config.latchedGroupSwitch->revealTargets);
        component->SetHideTargets(
            config.latchedGroupSwitch->hideTargets);
    }
}

}

void ApplyPlatformStageConfig(
    Platform& platform,
    const PlatformStageConfig& config)
{
    if (config.movement) {
        ApplyMovementConfig(platform, *config.movement);
    }
    ApplyBehaviorConfig(platform, config.behavior);
}
