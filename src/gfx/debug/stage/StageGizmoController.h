#pragma once

#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/stage/StageSelectionController.h"

#include "ImGuizmo.h"
#include "imgui.h"

#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Actor;

class StageGizmoController {
public:
    using Callback = std::function<void()>;

    StageGizmoController(DebugEditorContext& context, StageSelectionController& selectionController,
                         Callback pushUndoCallback, Callback savePlacementCallback);

    void Update();

private:
    void HandleOperationShortcuts();
    void DrawGizmo();

    glm::mat4 CreateSelectedActorGizmoMatrix(Actor* actor) const;
    void ApplyGizmoMatrixToActor(Actor* actor, const glm::mat4& matrix, ImGuizmo::OPERATION operation);

private:
    DebugEditorContext& mContext;
    StageSelectionController& mSelectionController;

    Callback mPushUndoCallback;
    Callback mSavePlacementCallback;

    ImGuizmo::OPERATION mCurrentGizmoOperation = ImGuizmo::TRANSLATE;

    bool mEPressedPrev = false;
    bool mRPressedPrev = false;
    bool mTPressedPrev = false;

    bool mIsUsingTransformGizmo = false;

    glm::mat4 mPreviousGizmoMatrix = glm::mat4(1.0f);
    glm::mat4 mEditingGizmoMatrix = glm::mat4(1.0f);
};