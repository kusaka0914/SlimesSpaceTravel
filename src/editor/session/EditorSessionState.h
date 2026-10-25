#pragma once

#include "system/camera/CinematicCameraTypes.h"

#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>

// 再起動後に復元する編集対象・画面・カメラ・Player姿勢・選択キーの値。Actorのポインタは保存しない。
struct EditorSessionState {
    static constexpr int CurrentFormatVersion = 1;

    int stageNumber = 0;
    std::string stageYamlPath;

    int activeSectionIndex = 0;
    int sequenceEditorMenuIndex = 0;
    int stageEditorMenuIndex = 3;
    bool isEditorShowing = true;
    bool isSceneView = false;

    float rightPanelWidth = 0.0f;
    float assetBrowserHeight = 0.0f;

    CameraPose sceneCameraPose;
    bool hasPlayerDebugPose = false;
    glm::vec3 playerPosition{0.0f};
    glm::vec3 playerUp{0.0f, 1.0f, 0.0f};
    glm::quat playerOrientation{1.0f, 0.0f, 0.0f, 0.0f};
    int playerPlanetIndex = -1;
    std::vector<std::string> selectedActorKeys;
};
