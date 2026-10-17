#pragma once

#include "component/PlatformRevealTarget.h"

#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

struct PlatformMovementStageConfig {
    std::optional<glm::vec3> startLocalPos;
    glm::vec3 moveOffset{4.0f, 0.0f, 0.0f};
    std::optional<glm::vec3> endLocalPos;
    float moveDurationSeconds = 3.0f;
    float returnDelaySeconds = 1.0f;
    float endpointWaitSeconds = 0.0f;
    bool shouldMoveOnPlayer = false;
};

struct PlatformFadeOnStandStageConfig {
    float fadeOutDurationSeconds = 1.0f;
    float reappearDelaySeconds = 2.0f;
};

struct PlatformIntervalToggleStageConfig {
    float intervalSeconds = 3.0f;
    float warningDurationSeconds = 1.0f;
    float blinkIntervalSeconds = 0.15f;
    bool isInitiallyVisible = true;
};

struct PlatformRotationStageConfig {
    glm::vec3 localAxis{0.0f, 1.0f, 0.0f};
    float degreesPerSecond = 45.0f;
};

struct PlatformConveyorStageConfig {
    glm::vec3 localDirection{0.0f, 0.0f, 1.0f};
    float speed = 2.0f;
};

struct PlatformPressureSwitchStageConfig {
    std::vector<std::string> targetPlatformIds;
    std::vector<PlatformRevealTarget> targetEnemyRefs;
    std::vector<PlatformRevealTarget> hideTargets;
    float inactiveOpacity = 0.2f;
    bool shouldRemainOnAfterPressed = false;
};

struct PlatformLatchedGroupSwitchStageConfig {
    std::string groupId;
    std::vector<PlatformRevealTarget> revealTargets;
    std::vector<PlatformRevealTarget> hideTargets;
};

struct PlatformBehaviorStageConfig {
    std::optional<PlatformFadeOnStandStageConfig> fadeOnStand;
    std::optional<bool> jumpToggleInitiallyVisible;
    std::optional<PlatformIntervalToggleStageConfig> intervalToggle;
    std::optional<float> directionalMovementSpeed;
    std::optional<PlatformRotationStageConfig> rotation;
    std::optional<PlatformConveyorStageConfig> conveyor;
    std::optional<PlatformPressureSwitchStageConfig> pressureSwitch;
    std::optional<PlatformLatchedGroupSwitchStageConfig> latchedGroupSwitch;
    bool hasAdhesion = false;
    bool hasEnemyClearUnlock = false;
};

// 床の移動・振る舞い設定。optionalが空なら対応するComponentは追加しない。
struct PlatformStageConfig {
    std::optional<PlatformMovementStageConfig> movement;
    PlatformBehaviorStageConfig behavior;
};

// ステージの床設定を解析する。指定時は旧形式の移動フィールドも受け付ける。YAML変換例外は伝播する。
PlatformStageConfig ParsePlatformStageConfig(const YAML::Node& platformNode,
                                             bool shouldUseLegacyMovementFields = false);
