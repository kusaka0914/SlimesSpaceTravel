#include "gfx/debug/session/EditorSessionState.h"

#include <algorithm>
#include <exception>
#include <fstream>
#include <utility>
#include <yaml-cpp/yaml.h>

namespace {
void WriteVector3(YAML::Emitter& emitter, const glm::vec3& vector)
{
    emitter << YAML::Flow << YAML::BeginSeq
            << vector.x << vector.y << vector.z
            << YAML::EndSeq;
}

bool ReadVector3(const YAML::Node& node, glm::vec3& outVector)
{
    if (!node || !node.IsSequence() || node.size() != 3) {
        return false;
    }

    outVector.x = node[0].as<float>();
    outVector.y = node[1].as<float>();
    outVector.z = node[2].as<float>();
    return true;
}
}

bool EditorSessionRepository::Save(
    const std::filesystem::path& filePath,
    const EditorSessionState& sessionState,
    std::string& outErrorMessage)
{
    outErrorMessage.clear();

    try {
        std::error_code directoryError;
        const std::filesystem::path parentDirectory = filePath.parent_path();
        if (!parentDirectory.empty()) {
            std::filesystem::create_directories(parentDirectory, directoryError);
        }
        if (directoryError) {
            outErrorMessage = "Failed to create the editor session directory: " +
                              directoryError.message();
            return false;
        }

        YAML::Emitter emitter;
        emitter << YAML::BeginMap;
        emitter << YAML::Key << "formatVersion"
                << YAML::Value << EditorSessionState::CurrentFormatVersion;

        emitter << YAML::Key << "stage" << YAML::Value << YAML::BeginMap;
        emitter << YAML::Key << "number" << YAML::Value << sessionState.stageNumber;
        emitter << YAML::Key << "yamlPath" << YAML::Value << sessionState.stageYamlPath;
        emitter << YAML::EndMap;

        emitter << YAML::Key << "editor" << YAML::Value << YAML::BeginMap;
        emitter << YAML::Key << "activeSection" << YAML::Value << sessionState.activeSectionIndex;
        emitter << YAML::Key << "sequenceMenu" << YAML::Value << sessionState.sequenceEditorMenuIndex;
        emitter << YAML::Key << "stageMenu" << YAML::Value << sessionState.stageEditorMenuIndex;
        emitter << YAML::Key << "editorShowing" << YAML::Value << sessionState.isEditorShowing;
        emitter << YAML::Key << "sceneView" << YAML::Value << sessionState.isSceneView;
        emitter << YAML::Key << "rightPanelWidth" << YAML::Value << sessionState.rightPanelWidth;
        emitter << YAML::Key << "assetBrowserHeight" << YAML::Value << sessionState.assetBrowserHeight;
        emitter << YAML::EndMap;

        emitter << YAML::Key << "sceneCamera" << YAML::Value << YAML::BeginMap;
        emitter << YAML::Key << "position" << YAML::Value;
        WriteVector3(emitter, sessionState.sceneCameraPose.position);
        emitter << YAML::Key << "target" << YAML::Value;
        WriteVector3(emitter, sessionState.sceneCameraPose.target);
        emitter << YAML::Key << "up" << YAML::Value;
        WriteVector3(emitter, sessionState.sceneCameraPose.up);
        emitter << YAML::Key << "fieldOfViewDegrees"
                << YAML::Value << sessionState.sceneCameraPose.fieldOfViewDegrees;
        emitter << YAML::EndMap;

        emitter << YAML::Key << "playerDebugPose" << YAML::Value << YAML::BeginMap;
        emitter << YAML::Key << "available" << YAML::Value << sessionState.hasPlayerDebugPose;
        emitter << YAML::Key << "position" << YAML::Value;
        WriteVector3(emitter, sessionState.playerPosition);
        emitter << YAML::Key << "up" << YAML::Value;
        WriteVector3(emitter, sessionState.playerUp);
        emitter << YAML::Key << "orientation" << YAML::Value
                << YAML::Flow << YAML::BeginSeq
                << sessionState.playerOrientation.w
                << sessionState.playerOrientation.x
                << sessionState.playerOrientation.y
                << sessionState.playerOrientation.z
                << YAML::EndSeq;
        emitter << YAML::Key << "planetIndex" << YAML::Value << sessionState.playerPlanetIndex;
        emitter << YAML::EndMap;

        std::vector<std::string> sortedActorKeys = sessionState.selectedActorKeys;
        std::sort(sortedActorKeys.begin(), sortedActorKeys.end());
        emitter << YAML::Key << "selectedActors" << YAML::Value << YAML::BeginSeq;
        for (const std::string& actorKey : sortedActorKeys) {
            emitter << actorKey;
        }
        emitter << YAML::EndSeq;
        emitter << YAML::EndMap;

        std::ofstream output(filePath, std::ios::binary | std::ios::trunc);
        if (!output) {
            outErrorMessage = "Failed to open the editor session file for writing: " +
                              filePath.string();
            return false;
        }

        output << emitter.c_str();
        if (!output.good()) {
            outErrorMessage = "Failed while writing the editor session file: " +
                              filePath.string();
            return false;
        }
    } catch (const std::exception& exception) {
        outErrorMessage = "Failed to save the editor session: " +
                          std::string(exception.what());
        return false;
    }

    return true;
}

bool EditorSessionRepository::Load(
    const std::filesystem::path& filePath,
    EditorSessionState& outSessionState,
    std::string& outErrorMessage)
{
    outErrorMessage.clear();

    try {
        const YAML::Node root = YAML::LoadFile(filePath.string());
        const int formatVersion = root["formatVersion"].as<int>(0);
        if (formatVersion != EditorSessionState::CurrentFormatVersion) {
            outErrorMessage = "Unsupported editor session format version: " +
                              std::to_string(formatVersion);
            return false;
        }

        const YAML::Node stageNode = root["stage"];
        const YAML::Node editorNode = root["editor"];
        const YAML::Node cameraNode = root["sceneCamera"];
        if (!stageNode || !editorNode || !cameraNode) {
            outErrorMessage = "The editor session file is missing required sections.";
            return false;
        }

        EditorSessionState loadedState;
        loadedState.stageNumber = stageNode["number"].as<int>();
        loadedState.stageYamlPath = stageNode["yamlPath"].as<std::string>();
        loadedState.activeSectionIndex = editorNode["activeSection"].as<int>(0);
        loadedState.sequenceEditorMenuIndex = editorNode["sequenceMenu"].as<int>(0);
        loadedState.stageEditorMenuIndex = editorNode["stageMenu"].as<int>(3);
        loadedState.isEditorShowing = editorNode["editorShowing"].as<bool>(true);
        loadedState.isSceneView = editorNode["sceneView"].as<bool>(false);
        loadedState.rightPanelWidth = editorNode["rightPanelWidth"].as<float>(0.0f);
        loadedState.assetBrowserHeight = editorNode["assetBrowserHeight"].as<float>(0.0f);

        if (!ReadVector3(cameraNode["position"], loadedState.sceneCameraPose.position) ||
            !ReadVector3(cameraNode["target"], loadedState.sceneCameraPose.target) ||
            !ReadVector3(cameraNode["up"], loadedState.sceneCameraPose.up)) {
            outErrorMessage = "The editor session contains an invalid scene camera pose.";
            return false;
        }
        loadedState.sceneCameraPose.fieldOfViewDegrees =
            cameraNode["fieldOfViewDegrees"].as<float>(60.0f);

        const YAML::Node playerPoseNode = root["playerDebugPose"];
        if (playerPoseNode) {
            loadedState.hasPlayerDebugPose =
                playerPoseNode["available"].as<bool>(false);
            const YAML::Node orientationNode = playerPoseNode["orientation"];
            if (loadedState.hasPlayerDebugPose &&
                (!ReadVector3(playerPoseNode["position"], loadedState.playerPosition) ||
                 !ReadVector3(playerPoseNode["up"], loadedState.playerUp) ||
                 !orientationNode ||
                 !orientationNode.IsSequence() ||
                 orientationNode.size() != 4)) {
                outErrorMessage = "The editor session contains an invalid player debug pose.";
                return false;
            }
            if (loadedState.hasPlayerDebugPose) {
                loadedState.playerOrientation = glm::quat(
                    orientationNode[0].as<float>(),
                    orientationNode[1].as<float>(),
                    orientationNode[2].as<float>(),
                    orientationNode[3].as<float>());
                loadedState.playerPlanetIndex =
                    playerPoseNode["planetIndex"].as<int>(-1);
            }
        }

        const YAML::Node selectedActorsNode = root["selectedActors"];
        if (selectedActorsNode && selectedActorsNode.IsSequence()) {
            for (const YAML::Node& actorKeyNode : selectedActorsNode) {
                loadedState.selectedActorKeys.emplace_back(actorKeyNode.as<std::string>());
            }
        }

        outSessionState = std::move(loadedState);
    } catch (const std::exception& exception) {
        outErrorMessage = "Failed to load the editor session: " +
                          std::string(exception.what());
        return false;
    }

    return true;
}
