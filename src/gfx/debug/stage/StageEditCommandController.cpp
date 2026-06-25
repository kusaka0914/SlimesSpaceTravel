#include "gfx/debug/stage/StageEditCommandController.h"

#include "Game.h"
#include "Stage.h"
#include "gfx/debug/stage/StageActorQuery.h"

#include "imgui.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <unordered_map>

StageEditCommandController::StageEditCommandController(DebugEditorContext& context,
                                                       StageSelectionController& selectionController)
    : mContext(context),
      mSelectionController(selectionController)
{
}

void StageEditCommandController::UpdateShortcuts()
{
    HandleDeleteShortcut();
    HandleUndoShortcut();
    HandleDuplicateShortcut();
}

bool StageEditCommandController::ConsumeRequestOpenPlacement()
{
    const bool result = mRequestOpenPlacement;
    mRequestOpenPlacement = false;
    return result;
}

void StageEditCommandController::HandleDeleteShortcut()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    if (mSelectionController.GetSelectedKeys().empty()) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();

    if (io.WantTextInput) {
        return;
    }

    if (ImGui::IsAnyItemActive()) {
        return;
    }

    const bool backspacePressed = ImGui::IsKeyPressed(ImGuiKey_Backspace, false);
    const bool deletePressed = ImGui::IsKeyPressed(ImGuiKey_Delete, false);

    if (!backspacePressed && !deletePressed) {
        return;
    }

    DeleteSelectedKeys(mSelectionController.GetSelectedKeys());
}

void StageEditCommandController::HandleUndoShortcut()
{
    if (!mContext.game || !mContext.game->GetWindow()) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();

    if (io.WantTextInput) {
        return;
    }

    if (ImGui::IsAnyItemActive()) {
        return;
    }

    const bool commandPressed = glfwGetKey(mContext.game->GetWindow(), GLFW_KEY_LEFT_SUPER) == GLFW_PRESS ||
                                glfwGetKey(mContext.game->GetWindow(), GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS;

    const bool zPressed = glfwGetKey(mContext.game->GetWindow(), GLFW_KEY_Z) == GLFW_PRESS;

    const bool zTriggered = zPressed && !mZPressedPrev;
    mZPressedPrev = zPressed;

    if (!commandPressed || !zTriggered) {
        return;
    }

    RestoreUndo();
}

void StageEditCommandController::HandleDuplicateShortcut()
{
    if (!mContext.game || !mContext.game->GetWindow()) {
        return;
    }

    if (mSelectionController.GetSelectedKeys().empty()) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();

    if (io.WantTextInput) {
        return;
    }

    if (ImGui::IsAnyItemActive()) {
        return;
    }

    const bool commandPressed = glfwGetKey(mContext.game->GetWindow(), GLFW_KEY_LEFT_SUPER) == GLFW_PRESS ||
                                glfwGetKey(mContext.game->GetWindow(), GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS;

    const bool dPressed = glfwGetKey(mContext.game->GetWindow(), GLFW_KEY_D) == GLFW_PRESS;

    const bool dTriggered = dPressed && !mDPressedPrev;
    mDPressedPrev = dPressed;

    if (!commandPressed || !dTriggered) {
        return;
    }

    DuplicateSelectedKeys(mSelectionController.GetSelectedKeys());
}

void StageEditCommandController::PushUndo()
{
    if (!mContext.game) {
        return;
    }

    const std::string filePath = mContext.game->GetCurrentStageYamlPath();

    std::ifstream ifs(filePath);
    if (!ifs) {
        std::cerr << "Failed to open stage yaml for undo: " << filePath << std::endl;
        return;
    }

    std::string yamlText((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

    try {
        YAML::Load(yamlText);
    } catch (const YAML::Exception& e) {
        std::cerr << "Skip pushing invalid undo yaml: " << e.what() << std::endl;
        return;
    }

    mUndoStack.push_back(yamlText);

    constexpr std::size_t maxUndoCount = 20;
    if (mUndoStack.size() > maxUndoCount) {
        mUndoStack.erase(mUndoStack.begin());
    }
}

bool StageEditCommandController::RestoreUndo()
{
    if (!mContext.game || mUndoStack.empty()) {
        return false;
    }

    const std::string filePath = mContext.game->GetCurrentStageYamlPath();
    const std::string tempPath = filePath + ".tmp";

    const std::string yamlText = mUndoStack.back();
    mUndoStack.pop_back();

    try {
        YAML::Load(yamlText);
    } catch (const YAML::Exception& e) {
        std::cerr << "Undo yaml is invalid. Restore cancelled: " << e.what() << std::endl;
        return false;
    }

    {
        std::ofstream ofs(tempPath, std::ios::out | std::ios::trunc);
        if (!ofs) {
            std::cerr << "Failed to open temp undo yaml: " << tempPath << std::endl;
            return false;
        }

        ofs << yamlText;
        ofs.close();

        if (!ofs) {
            std::cerr << "Failed to write temp undo yaml completely: " << tempPath << std::endl;
            return false;
        }
    }

    std::filesystem::rename(tempPath, filePath);

    mSelectionController.Clear();

    mContext.game->ReloadCurrentStage();

    return true;
}

bool StageEditCommandController::DeleteSelectedKeys(const std::unordered_set<std::string>& selectedKeys)
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return false;
    }

    if (selectedKeys.empty()) {
        return false;
    }

    const std::string filePath = mContext.game->GetCurrentStageYamlPath();

    YAML::Node config;

    try {
        config = YAML::LoadFile(filePath);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load stage yaml: " << filePath << std::endl;
        std::cerr << e.what() << std::endl;
        return false;
    }

    std::vector<StageActorRef> targets = StageActorQuery::CollectAllTargets(mContext.game->GetCurrentStage());

    std::unordered_map<std::string, std::vector<int>> deleteIndicesBySequence;

    for (const StageActorRef& target : targets) {
        const std::string key = StageActorQuery::MakeKey(target);

        if (!selectedKeys.contains(key)) {
            continue;
        }

        deleteIndicesBySequence[target.sequenceName].push_back(target.yamlIndex);
    }

    if (deleteIndicesBySequence.empty()) {
        return false;
    }

    PushUndo();

    for (auto& pair : deleteIndicesBySequence) {
        const std::string sequenceName = pair.first;
        std::vector<int>& indices = pair.second;

        std::sort(indices.begin(), indices.end());
        indices.erase(std::unique(indices.begin(), indices.end()), indices.end());

        std::sort(indices.rbegin(), indices.rend());

        for (int index : indices) {
            RemoveYamlSequenceElement(config, sequenceName, index);
        }
    }

    if (!SaveYamlFile(filePath, config)) {
        return false;
    }

    mSelectionController.Clear();

    mContext.game->ReloadCurrentStage();

    return true;
}

bool StageEditCommandController::DuplicateSelectedKeys(const std::unordered_set<std::string>& selectedKeys)
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return false;
    }

    if (selectedKeys.empty()) {
        return false;
    }

    const std::string filePath = mContext.game->GetCurrentStageYamlPath();

    YAML::Node stageYaml;

    try {
        stageYaml = YAML::LoadFile(filePath);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load stage yaml for duplicate: " << e.what() << std::endl;
        return false;
    }

    std::vector<StageActorRef> targets = StageActorQuery::CollectAllTargets(mContext.game->GetCurrentStage());

    std::unordered_set<std::string> newSelectedKeys;

    bool duplicated = false;

    const glm::vec3 duplicateOffset(1.0f, 0.0f, 0.0f);

    for (const StageActorRef& target : targets) {
        const std::string key = StageActorQuery::MakeKey(target);

        if (!selectedKeys.contains(key)) {
            continue;
        }

        YAML::Node sequence = stageYaml[target.sequenceName];

        if (!sequence || !sequence.IsSequence()) {
            std::cerr << "Duplicate skipped. Sequence not found: " << target.sequenceName << std::endl;
            continue;
        }

        if (target.yamlIndex < 0 || target.yamlIndex >= static_cast<int>(sequence.size())) {
            std::cerr << "Duplicate skipped. Invalid yamlIndex: " << target.yamlIndex << std::endl;
            continue;
        }

        YAML::Node sourceNode = sequence[target.yamlIndex];
        YAML::Node duplicatedNode = YAML::Clone(sourceNode);

        OffsetDuplicatedActorNode(duplicatedNode, duplicateOffset);

        const int newYamlIndex = static_cast<int>(sequence.size());

        sequence.push_back(duplicatedNode);

        newSelectedKeys.insert(target.sequenceName + ":" + std::to_string(newYamlIndex));

        duplicated = true;
    }

    if (!duplicated) {
        return false;
    }

    PushUndo();

    if (!SaveYamlFile(filePath, stageYaml)) {
        return false;
    }

    mSelectionController.SetSelectedKeys(newSelectedKeys);

    mRequestOpenPlacement = true;

    mContext.game->ReloadCurrentStage();

    return true;
}

bool StageEditCommandController::SaveYamlFile(const std::string& filePath, const YAML::Node& config)
{
    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open yaml for writing: " << filePath << std::endl;
        return false;
    }

    file << config;
    return true;
}

bool StageEditCommandController::RemoveYamlSequenceElement(YAML::Node& config, const std::string& sequenceName,
                                                           int index)
{
    if (!config[sequenceName] || !config[sequenceName].IsSequence()) {
        std::cerr << "Invalid yaml sequence: " << sequenceName << std::endl;
        return false;
    }

    YAML::Node oldSeq = config[sequenceName];

    if (index < 0 || index >= static_cast<int>(oldSeq.size())) {
        std::cerr << "Delete index out of range: " << index << std::endl;
        return false;
    }

    YAML::Node newSeq(YAML::NodeType::Sequence);

    for (int i = 0; i < static_cast<int>(oldSeq.size()); ++i) {
        if (i == index) {
            continue;
        }

        newSeq.push_back(oldSeq[i]);
    }

    config[sequenceName] = newSeq;
    return true;
}

void StageEditCommandController::OffsetDuplicatedActorNode(YAML::Node actorNode, const glm::vec3& offset) const
{
    if (!actorNode) {
        return;
    }

    if (actorNode["pos"] && actorNode["pos"].IsSequence() && actorNode["pos"].size() >= 3) {
        try {
            const float x = actorNode["pos"][0].as<float>();
            const float y = actorNode["pos"][1].as<float>();
            const float z = actorNode["pos"][2].as<float>();

            actorNode["pos"][0] = x + offset.x;
            actorNode["pos"][1] = y + offset.y;
            actorNode["pos"][2] = z + offset.z;

            return;
        } catch (const YAML::Exception& e) {
            std::cerr << "Invalid pos while duplicating. Recreate pos." << std::endl;
        }
    }

    if (actorNode["theta"]) {
        try {
            const float theta = actorNode["theta"].as<float>();
            actorNode["theta"] = theta + 0.15f;
        } catch (const YAML::Exception& e) {
            actorNode["theta"] = 0.15f;
        }
    } else {
        actorNode["theta"] = 0.15f;
    }
}