#pragma once

#include "system/camera/CinematicCameraTypes.h"
#include <string>
#include <yaml-cpp/yaml.h>

// 距離はワールド単位、角度とFOVは度。Follow/Targetの方向・注視オフセットはreferenceUpを基準に保持する。
struct CameraZoneViewSettings {
    enum class Mode { Follow, Fixed, Target };
    Mode mode = Mode::Follow;
    bool fixedTracksPlayer = true;
    glm::vec3 referenceUp{0, 1, 0};
    glm::vec3 backDirection{0, 0, 1};
    glm::vec3 lookOffset{0, 1.5f, 0};
    float distance = 8.0f;
    float pitchDegrees = 20.0f;
    float fieldOfViewDegrees = 60.0f;
    float targetWeight = 0.35f;
    float minimumDistance = 5.0f;
    float maximumDistance = 40.0f;
    float framingMargin = 1.2f;
    // 対象はStage YAMLの参照で解決し、削除・非表示時はfallbackTargetを注視する。
    std::string targetSequence;
    int targetIndex = -1;
    glm::vec3 fallbackTarget{0};
    CameraPose fixedPose{};

    void Normalize();
    void Load(const YAML::Node& node);
    void Save(YAML::Node node) const;
};

// 既存の単方向設定を行き用として維持し、双方向ON時だけ帰り用の構図を使う。
struct CameraZoneSettings : CameraZoneViewSettings {
    int priority = 0;
    bool enabled = true;
    bool holdWhileAirborne = true;
    float enterSeconds = 0.7f;
    float exitSeconds = 0.7f;
    float exitMargin = 0.4f;
    bool allowManualRotation = false;
    float manualReturnDelay = 1.0f;
    bool bidirectional = false;
    glm::vec3 travelDirection{0, 0, -1};
    float directionSwitchDelay = 0.4f;
    float directionBlendSeconds = 0.7f;
    bool allowAirborneDirectionSwitch = false;
    bool defaultReturnCamera = false;
    CameraZoneViewSettings returnCamera;

    void Normalize();
    void Load(const YAML::Node& node);
    void Save(YAML::Node node) const;
};
