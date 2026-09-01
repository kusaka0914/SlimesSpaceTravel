#include "TestSupport.h"

#include "system/actor_loader/PlatformStageConfig.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace {

void ModernPlatformWithoutMovementComponentDoesNotCreateMovementConfig()
{
    const YAML::Node platformNode = YAML::Load(R"(
components:
  rotation: {}
)");

    const PlatformStageConfig config =
        ParsePlatformStageConfig(platformNode);

    ExpectFalse(config.movement.has_value(), "movement config");
    ExpectTrue(config.behavior.rotation.has_value(), "rotation config");
}

void MovementComponentUsesDocumentedDefaultsAndLegacyWaitField()
{
    const YAML::Node platformNode = YAML::Load(R"(
components:
  movement:
    destinationWaitSeconds: 2.5
)");

    const PlatformStageConfig config =
        ParsePlatformStageConfig(platformNode);
    const PlatformMovementStageConfig& movement = *config.movement;

    ExpectNear(4.0f, movement.moveOffset.x, 0.0001f, "default move offset x");
    ExpectNear(3.0f, movement.moveDurationSeconds, 0.0001f, "default move duration");
    ExpectNear(1.0f, movement.returnDelaySeconds, 0.0001f, "default return delay");
    ExpectNear(2.5f, movement.endpointWaitSeconds, 0.0001f, "legacy endpoint wait");
    ExpectFalse(movement.shouldMoveOnPlayer, "default move-on-player");
}

void LegacyMovingPlatformReadsMovementFieldsFromPlatformNode()
{
    const YAML::Node platformNode = YAML::Load(R"(
startLocalPos: [1.0, 2.0, 3.0]
endLocalPos: [4.0, 5.0, 6.0]
moveDuration: 7.0
moveOnPlayer: true
returnDelay: 8.0
endpointWaitSeconds: 9.0
)");

    const PlatformStageConfig config =
        ParsePlatformStageConfig(platformNode, true);
    const PlatformMovementStageConfig& movement = *config.movement;

    ExpectNear(1.0f, movement.startLocalPos->x, 0.0001f, "start x");
    ExpectNear(6.0f, movement.endLocalPos->z, 0.0001f, "end z");
    ExpectNear(7.0f, movement.moveDurationSeconds, 0.0001f, "move duration");
    ExpectTrue(movement.shouldMoveOnPlayer, "move-on-player");
    ExpectNear(8.0f, movement.returnDelaySeconds, 0.0001f, "return delay");
    ExpectNear(9.0f, movement.endpointWaitSeconds, 0.0001f, "endpoint wait");
}

void BehaviorComponentsParseValuesAndIgnoreInvalidActorTargets()
{
    const YAML::Node platformNode = YAML::Load(R"(
components:
  fadeOnStand:
    fadeOutDuration: 1.25
    reappearDelay: 2.5
  jumpToggle:
    initiallyVisible: false
  intervalToggle:
    interval: 4.0
    warningDuration: 0.75
    blinkInterval: 0.2
    initiallyVisible: false
  directionalMovement:
    speed: 3.5
  rotation:
    axis: [1.0, 0.0, 0.0]
    degreesPerSecond: 90.0
  conveyor:
    direction: [0.0, 1.0, 0.0]
    speed: 6.0
  adhesion: {}
  enemyClearUnlock: {}
  pressureSwitch:
    remainsOnAfterPressed: true
    inactiveOpacity: 0.4
    targets: [first, second]
    enemyTargets:
      - { sequence: enemies, index: 3 }
      - { sequence: enemies }
  latchedGroupSwitch:
    groupId: group-a
    targets:
      - { sequence: platforms, index: 4, platformId: target-id }
)");

    const PlatformBehaviorStageConfig& behavior =
        ParsePlatformStageConfig(platformNode).behavior;

    ExpectNear(1.25f, behavior.fadeOnStand->fadeOutDurationSeconds, 0.0001f, "fade duration");
    ExpectFalse(*behavior.jumpToggleInitiallyVisible, "jump initial visibility");
    ExpectNear(4.0f, behavior.intervalToggle->intervalSeconds, 0.0001f, "toggle interval");
    ExpectNear(3.5f, *behavior.directionalMovementSpeed, 0.0001f, "movement speed");
    ExpectNear(1.0f, behavior.rotation->localAxis.x, 0.0001f, "rotation axis x");
    ExpectNear(90.0f, behavior.rotation->degreesPerSecond, 0.0001f, "rotation speed");
    ExpectNear(1.0f, behavior.conveyor->localDirection.y, 0.0001f, "conveyor direction y");
    ExpectNear(6.0f, behavior.conveyor->speed, 0.0001f, "conveyor speed");
    ExpectTrue(behavior.hasAdhesion, "adhesion component");
    ExpectTrue(behavior.hasEnemyClearUnlock, "enemy-clear component");
    ExpectEqual(
        std::size_t(2),
        behavior.pressureSwitch->targetPlatformIds.size(),
        "platform target count");
    ExpectEqual(
        std::size_t(1),
        behavior.pressureSwitch->targetEnemyRefs.size(),
        "valid enemy target count");
    ExpectTrue(
        behavior.pressureSwitch->shouldRemainOnAfterPressed,
        "latched pressure switch");
    ExpectNear(
        0.4f,
        behavior.pressureSwitch->inactiveOpacity,
        0.0001f,
        "inactive opacity");
    ExpectEqual(
        std::string("group-a"),
        behavior.latchedGroupSwitch->groupId,
        "switch group");
    ExpectEqual(
        std::string("target-id"),
        behavior.latchedGroupSwitch->revealTargets[0].platformId,
        "reveal target id");
}

}

void RegisterPlatformStageConfigTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.emplace_back(
        "PlatformStageConfig.ModernPlatformWithoutMovementComponentDoesNotCreateMovementConfig",
        ModernPlatformWithoutMovementComponentDoesNotCreateMovementConfig);
    tests.emplace_back(
        "PlatformStageConfig.MovementComponentUsesDocumentedDefaultsAndLegacyWaitField",
        MovementComponentUsesDocumentedDefaultsAndLegacyWaitField);
    tests.emplace_back(
        "PlatformStageConfig.LegacyMovingPlatformReadsMovementFieldsFromPlatformNode",
        LegacyMovingPlatformReadsMovementFieldsFromPlatformNode);
    tests.emplace_back(
        "PlatformStageConfig.BehaviorComponentsParseValuesAndIgnoreInvalidActorTargets",
        BehaviorComponentsParseValuesAndIgnoreInvalidActorTargets);
}
