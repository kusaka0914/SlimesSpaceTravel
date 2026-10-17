#pragma once

#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/stage/StageSelectionController.h"

#include "ImGuizmo.h"
#include "imgui.h"

#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Actor;

// 選択Actorのギズモ操作を管理し、操作開始時のUndo記録と終了時の配置保存を通知する。
class StageGizmoController {
public:
    using Callback = std::function<void()>;

    enum class TranslationSpace {
        PlanetSurface,
        World,
    };

    StageGizmoController(DebugEditorContext& context, StageSelectionController& selectionController,
                         Callback pushUndoCallback, Callback savePlacementCallback);

    // 毎フレーム呼ぶ。操作中は実行中Actorを変更し、操作完了時に保存コールバックを呼ぶ。
    void Update();
    void SetTranslationSpace(TranslationSpace translationSpace);
    TranslationSpace GetTranslationSpace() const { return mTranslationSpace; }
    bool IsUsingTransformGizmo() const { return mIsUsingTransformGizmo; }

private:
    void HandleOperationShortcuts();
    void DrawGizmo();

    glm::mat4 CreateSelectedActorGizmoMatrix(Actor* actor, ImGuizmo::OPERATION operation) const;
    glm::mat4 CreatePlanetSurfaceTranslationMatrix(Actor* actor) const;
    bool UsesPlanetSurfaceTranslation(Actor* actor) const;
    bool RequiresSphereSurfaceProjection(Actor* actor) const;
    void ApplyPlanetSurfaceTranslation(Actor* actor, const glm::vec3& rawWorldPos);
    void ApplyGizmoMatrixToActor(Actor* actor, const glm::mat4& matrix, ImGuizmo::OPERATION operation);

    DebugEditorContext& mContext;
    StageSelectionController& mSelectionController;

    Callback mPushUndoCallback;
    Callback mSavePlacementCallback;

    ImGuizmo::OPERATION mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    TranslationSpace mTranslationSpace = TranslationSpace::PlanetSurface;

    bool mWasEPressed = false;
    bool mWasRPressed = false;
    bool mWasTPressed = false;

    bool mIsUsingTransformGizmo = false;

    glm::mat4 mPreviousGizmoMatrix = glm::mat4(1.0f);
    glm::mat4 mEditingGizmoMatrix = glm::mat4(1.0f);
};
