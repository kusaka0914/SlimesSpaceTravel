#include "gfx/debug/stage/StageGizmoController.h"

#include "Game.h"
#include "actor/Actor.h"
#include "system/CameraSystem.h"
#include "system/PhysicsSystem.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

StageGizmoController::StageGizmoController(DebugEditorContext& context, StageSelectionController& selectionController,
                                           Callback pushUndoCallback, Callback savePlacementCallback)
    : mContext(context),
      mSelectionController(selectionController),
      mPushUndoCallback(std::move(pushUndoCallback)),
      mSavePlacementCallback(std::move(savePlacementCallback))
{
}

void StageGizmoController::Update()
{
    HandleOperationShortcuts();
    DrawGizmo();
}

void StageGizmoController::HandleOperationShortcuts()
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

    const bool ePressed = glfwGetKey(mContext.game->GetWindow(), GLFW_KEY_E) == GLFW_PRESS;
    const bool rPressed = glfwGetKey(mContext.game->GetWindow(), GLFW_KEY_R) == GLFW_PRESS;
    const bool tPressed = glfwGetKey(mContext.game->GetWindow(), GLFW_KEY_T) == GLFW_PRESS;

    const bool eTriggered = ePressed && !mEPressedPrev;
    const bool rTriggered = rPressed && !mRPressedPrev;
    const bool tTriggered = tPressed && !mTPressedPrev;

    mEPressedPrev = ePressed;
    mRPressedPrev = rPressed;
    mTPressedPrev = tPressed;

    if (eTriggered) {
        mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    }

    if (rTriggered) {
        mCurrentGizmoOperation = ImGuizmo::ROTATE;
    }

    if (tTriggered) {
        mCurrentGizmoOperation = ImGuizmo::SCALE;
    }
}

glm::mat4 StageGizmoController::CreateSelectedActorGizmoMatrix(Actor* actor) const
{
    if (!actor) {
        return glm::mat4(1.0f);
    }

    const glm::vec3 rotation = actor->GetEditorRotation();

    glm::mat4 rotateMat(1.0f);
    rotateMat = glm::rotate(rotateMat, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    rotateMat = glm::rotate(rotateMat, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    rotateMat = glm::rotate(rotateMat, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));

    return glm::translate(glm::mat4(1.0f), actor->GetPos()) * rotateMat *
           glm::scale(glm::mat4(1.0f), actor->GetScale());
}

void StageGizmoController::ApplyGizmoMatrixToActor(Actor* actor, const glm::mat4& matrix, ImGuizmo::OPERATION operation)
{
    if (!actor) {
        return;
    }

    float translation[3];
    float rotationDeg[3];
    float scale[3];

    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(matrix), translation, rotationDeg, scale);

    if (operation == ImGuizmo::TRANSLATE) {
        actor->SetPos(glm::vec3(translation[0], translation[1], translation[2]));
        return;
    }

    if (operation == ImGuizmo::SCALE) {
        actor->SetScale(glm::vec3(scale[0], scale[1], scale[2]));
        return;
    }

    if (operation == ImGuizmo::ROTATE) {
        actor->SetEditorRotation(glm::radians(glm::vec3(rotationDeg[0], rotationDeg[1], rotationDeg[2])));
        actor->SetFacingYaw(glm::radians(rotationDeg[1]));
        return;
    }
}

void StageGizmoController::DrawGizmo()
{
    if (!mContext.game || !mContext.game->GetWindow() || !mContext.game->GetCameraSystem()) {
        return;
    }

    if (mSelectionController.GetSelectedKeys().empty()) {
        return;
    }

    std::vector<glm::mat4> views = mContext.game->GetCameraSystem()->GetViews();
    if (views.empty()) {
        return;
    }

    int windowWidth = 0;
    int windowHeight = 0;
    glfwGetWindowSize(mContext.game->GetWindow(), &windowWidth, &windowHeight);

    if (windowWidth <= 0 || windowHeight <= 0) {
        return;
    }

    const glm::mat4 view = views[0];
    const float aspect = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
    const glm::mat4 projection = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);

    ImGuizmo::BeginFrame();
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGuizmo::SetRect(viewport->Pos.x, viewport->Pos.y, viewport->Size.x, viewport->Size.y);

    const int selectedCount = mSelectionController.GetSelectedActorCount();
    if (selectedCount <= 0) {
        return;
    }

    if (selectedCount > 1) {
        if (mCurrentGizmoOperation != ImGuizmo::TRANSLATE) {
            return;
        }

        const glm::vec3 center = mSelectionController.CalculateSelectedActorsCenter();

        glm::mat4 gizmoMatrix = glm::translate(glm::mat4(1.0f), center);

        ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), ImGuizmo::TRANSLATE, ImGuizmo::WORLD,
                             glm::value_ptr(gizmoMatrix));

        if (ImGuizmo::IsUsing()) {
            const glm::vec3 newGizmoPos = glm::vec3(gizmoMatrix[3]);

            if (!mIsUsingTransformGizmo) {
                if (mPushUndoCallback) {
                    mPushUndoCallback();
                }

                mPreviousGizmoMatrix = glm::translate(glm::mat4(1.0f), center);
                mIsUsingTransformGizmo = true;
            }

            const glm::vec3 previousPos = glm::vec3(mPreviousGizmoMatrix[3]);
            const glm::vec3 delta = newGizmoPos - previousPos;

            if (glm::length(delta) > 1e-6f) {
                mSelectionController.MoveSelectedActorsByDelta(delta);
                mPreviousGizmoMatrix = gizmoMatrix;
            }
        } else {
            if (mIsUsingTransformGizmo) {
                mIsUsingTransformGizmo = false;

                if (mSavePlacementCallback) {
                    mSavePlacementCallback();
                }

                if (mContext.game->GetPhysicsSystem()) {
                    mContext.game->GetPhysicsSystem()->Initialize();
                }
            }
        }

        return;
    }

    Actor* selectedActor = mSelectionController.GetSingleSelectedActor();
    if (!selectedActor) {
        return;
    }

    if (!mIsUsingTransformGizmo) {
        mEditingGizmoMatrix = CreateSelectedActorGizmoMatrix(selectedActor);
    }

    ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), mCurrentGizmoOperation, ImGuizmo::WORLD,
                         glm::value_ptr(mEditingGizmoMatrix));

    if (ImGuizmo::IsUsing()) {
        if (!mIsUsingTransformGizmo) {
            if (mPushUndoCallback) {
                mPushUndoCallback();
            }

            mIsUsingTransformGizmo = true;
        }

        ApplyGizmoMatrixToActor(selectedActor, mEditingGizmoMatrix, mCurrentGizmoOperation);
    } else {
        if (mIsUsingTransformGizmo) {
            mIsUsingTransformGizmo = false;

            if (mSavePlacementCallback) {
                mSavePlacementCallback();
            }

            if (mContext.game->GetPhysicsSystem()) {
                mContext.game->GetPhysicsSystem()->Initialize();
            }
        }
    }
}