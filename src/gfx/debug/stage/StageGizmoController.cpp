#include "gfx/debug/stage/StageGizmoController.h"

#include "Game.h"
#include "actor/Actor.h"
#include "actor/MovingPlatform.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/StageObject.h"
#include "system/CameraSystem.h"
#include "system/PhysicsSystem.h"
#include "utils/MathUtils.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
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

    glm::vec3 up = actor->GetUpVec();
    glm::vec3 forward = actor->GetForwardVec();

    if (glm::length(up) < 1e-6f) {
        up = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    up = glm::normalize(up);

    forward -= up * glm::dot(forward, up);
    if (glm::length(forward) < 1e-6f) {
        forward = glm::vec3(0.0f, 0.0f, 1.0f);
        forward -= up * glm::dot(forward, up);
    }
    if (glm::length(forward) < 1e-6f) {
        forward = glm::vec3(1.0f, 0.0f, 0.0f);
        forward -= up * glm::dot(forward, up);
    }
    forward = glm::normalize(forward);

    const glm::vec3 left = glm::normalize(glm::cross(up, forward));

    glm::mat4 orientation(1.0f);
    orientation[0] = glm::vec4(left, 0.0f);
    orientation[1] = glm::vec4(up, 0.0f);
    orientation[2] = glm::vec4(forward, 0.0f);

    return glm::translate(glm::mat4(1.0f), actor->GetPos()) * orientation *
           glm::scale(glm::mat4(1.0f), actor->GetScale());
}

void StageGizmoController::ApplyGizmoMatrixToActor(Actor* actor, const glm::mat4& matrix, ImGuizmo::OPERATION operation)
{
    if (!actor) {
        return;
    }

    if (operation == ImGuizmo::ROTATE) {
        glm::vec3 up = glm::vec3(matrix[1]);
        glm::vec3 forward = glm::vec3(matrix[2]);

        if (glm::length(up) < 1e-6f || glm::length(forward) < 1e-6f) {
            return;
        }

        up = glm::normalize(up);
        forward -= up * glm::dot(forward, up);
        if (glm::length(forward) < 1e-6f) {
            return;
        }
        forward = glm::normalize(forward);

        actor->SetUpVec(up);

        if (mContext.game && mContext.game->GetMathUtils()) {
            const float facingYaw = mContext.game->GetMathUtils()->GetYawFromDirection(up, forward);
            actor->SetFacingYaw(facingYaw);

            glm::vec3 editorRotation = actor->GetEditorRotation();
            editorRotation.y = facingYaw;
            actor->SetEditorRotation(editorRotation);
        }
        return;
    }

    float translation[3];
    float rotationDeg[3];
    float scale[3];

    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(matrix), translation, rotationDeg, scale);

    if (operation == ImGuizmo::TRANSLATE) {
        const glm::vec3 worldPos(
            translation[0], translation[1], translation[2]);
        actor->SetPos(worldPos);

        if (MovingPlatform* movingPlatform =
                dynamic_cast<MovingPlatform*>(actor);
            movingPlatform && movingPlatform->GetMoveOnPlayer() &&
            movingPlatform->GetCurrentPlanet()) {
            movingPlatform->SetEditorPreviewLocalPos(
                worldPos - movingPlatform->GetCurrentPlanet()->GetPos());
        }
        return;
    }

    if (operation == ImGuizmo::SCALE) {
        const glm::vec3 previousScale = actor->GetScale();
        const glm::vec3 actorScale(scale[0], scale[1], scale[2]);
        actor->SetScale(actorScale);

        const bool horizontalScaleChanged =
            std::abs(actorScale.x - previousScale.x) > 0.0001f ||
            std::abs(actorScale.z - previousScale.z) > 0.0001f;
        if (horizontalScaleChanged &&
            (dynamic_cast<Platform*>(actor) || dynamic_cast<StageObject*>(actor))) {
            actor->SetTextureTiling(
                glm::vec2(
                    std::max(1.0f, std::abs(actorScale.x)),
                    std::max(1.0f, std::abs(actorScale.z))));
        }
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

    const ImGuizmo::MODE gizmoMode =
        mCurrentGizmoOperation == ImGuizmo::ROTATE ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
    ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), mCurrentGizmoOperation, gizmoMode,
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
