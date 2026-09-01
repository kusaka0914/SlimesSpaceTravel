#include "gfx/debug/stage/StageEditShortcutHandler.h"

#include "Game.h"
#include "Stage.h"
#include "gfx/debug/stage/StageActorQuery.h"
#include "gfx/debug/stage/StageEditCommandController.h"
#include "gfx/debug/stage/StageSelectionController.h"
#include "imgui.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <string>
#include <unordered_set>
#include <vector>

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

StageEditShortcutHandler::StageEditShortcutHandler(
    DebugEditorContext& context,
    StageSelectionController& selectionController,
    StageEditCommandController& editCommandController)
    : mContext(context),
      mSelectionController(selectionController),
      mEditCommandController(editCommandController)
{
}

void StageEditShortcutHandler::Update()
{
    HandleDeleteShortcut();
    HandleUndoRedoShortcut();
    HandleDuplicateShortcut();
}

void StageEditShortcutHandler::HandleDeleteShortcut()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }
    if (mSelectionController.GetSelectedKeys().empty()) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput || ImGui::IsAnyItemActive()) {
        return;
    }

    const bool isBackspacePressed =
        ImGui::IsKeyPressed(ImGuiKey_Backspace, false);
    const bool isDeletePressed =
        ImGui::IsKeyPressed(ImGuiKey_Delete, false);
    if (!isBackspacePressed && !isDeletePressed) {
        return;
    }

    const std::unordered_set<std::string>& selectedKeys =
        mSelectionController.GetSelectedKeys();
    const std::vector<StageActorRef> targets =
        StageActorQuery::CollectAllTargets(
            mContext.game->GetCurrentStage(),
            true);
    std::vector<int> selectedPlanetIndices;
    for (const StageActorRef& target : targets) {
        if (target.type == StageActorType::Planet &&
            selectedKeys.contains(StageActorQuery::MakeKey(target))) {
            selectedPlanetIndices.push_back(target.yamlIndex);
        }
    }

    if (!selectedPlanetIndices.empty() &&
        selectedPlanetIndices.size() == selectedKeys.size()) {
        const std::size_t planetCount =
            mContext.game->GetCurrentStage()->GetPlanets().size();
        if (selectedPlanetIndices.size() >= planetCount) {
            return;
        }

        std::sort(
            selectedPlanetIndices.rbegin(),
            selectedPlanetIndices.rend());
        for (int planetIndex : selectedPlanetIndices) {
            mEditCommandController.DeletePlanet(planetIndex);
        }
        return;
    }

    mEditCommandController.DeleteSelectedKeys(selectedKeys);
}

void StageEditShortcutHandler::HandleUndoRedoShortcut()
{
    if (!mContext.game || !mContext.game->GetWindow()) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput || ImGui::IsAnyItemActive()) {
        return;
    }

    GLFWwindow* window = mContext.game->GetWindow();
    const bool isCommandPressed =
        IsPrimaryShortcutModifierPressed(window);
    const bool isZPressed =
        glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS;
    const bool isYPressed =
        glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS;
    const bool isShiftPressed =
        glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;

    const bool isZTriggered = isZPressed && !mWasZPressed;
    const bool isYTriggered = isYPressed && !mWasYPressed;
    mWasZPressed = isZPressed;
    mWasYPressed = isYPressed;

    if (!isCommandPressed) {
        return;
    }

    const bool isRedoTriggered =
        isYTriggered || (isZTriggered && isShiftPressed);
    if (isRedoTriggered) {
        mEditCommandController.RestoreRedo();
        return;
    }
    if (isZTriggered) {
        mEditCommandController.RestoreUndo();
    }
}

void StageEditShortcutHandler::HandleDuplicateShortcut()
{
    if (!mContext.game || !mContext.game->GetWindow()) {
        return;
    }
    if (mSelectionController.GetSelectedKeys().empty()) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput || ImGui::IsAnyItemActive()) {
        return;
    }

    GLFWwindow* window = mContext.game->GetWindow();
    const bool isCommandPressed =
        IsPrimaryShortcutModifierPressed(window);
    const bool isDPressed =
        glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
    const bool isDTriggered = isDPressed && !mWasDPressed;
    mWasDPressed = isDPressed;

    if (isCommandPressed && isDTriggered) {
        mEditCommandController.DuplicateSelectedKeys(
            mSelectionController.GetSelectedKeys());
    }
}
