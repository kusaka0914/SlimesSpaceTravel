#pragma once

#include "system/camera/CinematicCameraTypes.h"

#include <filesystem>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>

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

class EditorSessionRepository {
public:
    static bool Save(
        const std::filesystem::path& filePath,
        const EditorSessionState& sessionState,
        std::string& outErrorMessage);

    static bool Load(
        const std::filesystem::path& filePath,
        EditorSessionState& outSessionState,
        std::string& outErrorMessage);
};
