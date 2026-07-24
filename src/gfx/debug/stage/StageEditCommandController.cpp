#include "gfx/debug/stage/StageEditCommandController.h"

#include "Game.h"
#include "Stage.h"
#include "gfx/debug/stage/StageActorQuery.h"
#include "gfx/debug/stage/StageYamlRepository.h"

#include "imgui.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>
#include <unordered_map>

namespace {
bool IsPrimaryShortcutModifierPressed(GLFWwindow* window)
{
    if (!window) {
        return false;
    }

#if defined(__APPLE__)
    return glfwGetKey(window, GLFW_KEY_LEFT_SUPER) == GLFW_PRESS ||
           glfwGetKey(window, GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS;
#else
    return glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
           glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
#endif
}
}

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

    const bool commandPressed =
        IsPrimaryShortcutModifierPressed(mContext.game->GetWindow());

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

    const bool commandPressed =
        IsPrimaryShortcutModifierPressed(mContext.game->GetWindow());

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

    std::string yamlText;

    if (!StageYamlRepository::ReadCurrentStageText(mContext, yamlText)) {
        return;
    }

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

    const std::string yamlText = mUndoStack.back();

    if (!StageYamlRepository::WriteCurrentStageTextAtomically(mContext, yamlText)) {
        return false;
    }

    mUndoStack.pop_back();

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

    YAML::Node config;

    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
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
            StageYamlRepository::RemoveSequenceElement(config, sequenceName, index);
        }
    }

    if (!StageYamlRepository::SaveCurrentStage(mContext, config)) {
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

    YAML::Node stageYaml;

    if (!StageYamlRepository::LoadCurrentStage(mContext, stageYaml)) {
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

    if (!StageYamlRepository::SaveCurrentStage(mContext, stageYaml)) {
        return false;
    }

    mSelectionController.SetSelectedKeys(newSelectedKeys);

    mRequestOpenPlacement = true;

    mContext.game->ReloadCurrentStage();

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
