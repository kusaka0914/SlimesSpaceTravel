#include "gfx/debug/stage/StageGizmoController.h"

#include "Game.h"
#include "actor/Actor.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/StageObject.h"
#include "component/PlatformMovementComponent.h"
#include "system/CameraSystem.h"
#include "system/PhysicsSystem.h"
#include "system/StageActorPlanetBindingService.h"
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

glm::mat4 StageGizmoController::CreateSelectedActorGizmoMatrix(
    Actor* actor, ImGuizmo::OPERATION operation) const
{
    if (!actor) {
        return glm::mat4(1.0f);
    }

    glm::mat4 orientation = glm::mat4_cast(actor->GetOrientation());
    if (operation == ImGuizmo::SCALE &&
        mContext.game && mContext.game->GetMathUtils()) {
        orientation = mContext.game->GetMathUtils()->CreateOrient(actor);
    }

    return glm::translate(glm::mat4(1.0f), actor->GetPos()) * orientation *
           glm::scale(glm::mat4(1.0f), actor->GetScale());
}

void StageGizmoController::SetTranslationSpace(
    TranslationSpace translationSpace)
{
    if (mIsUsingTransformGizmo) {
        return;
    }

    mTranslationSpace = translationSpace;
}

bool StageGizmoController::UsesPlanetSurfaceTranslation(Actor* actor) const
{
    if (!actor ||
        mCurrentGizmoOperation != ImGuizmo::TRANSLATE ||
        mTranslationSpace != TranslationSpace::PlanetSurface) {
        return false;
    }

    return actor->GetCurrentPlanet() != nullptr;
}

bool StageGizmoController::RequiresSphereSurfaceProjection(
    Actor* actor) const
{
    if (!UsesPlanetSurfaceTranslation(actor)) {
        return false;
    }

    Planet* planet = actor->GetCurrentPlanet();
    return planet &&
           planet->GetPlanetShape() == Planet::PlanetShape::Sphere &&
           glm::length(actor->GetPos() - planet->GetPos()) > 1e-6f;
}

glm::mat4 StageGizmoController::CreatePlanetSurfaceTranslationMatrix(
    Actor* actor) const
{
    if (!actor || !actor->GetCurrentPlanet()) {
        return CreateSelectedActorGizmoMatrix(actor, ImGuizmo::TRANSLATE);
    }

    Planet* planet = actor->GetCurrentPlanet();
    glm::vec3 up = actor->GetUpVec();
    if (planet->GetPlanetShape() == Planet::PlanetShape::Sphere) {
        const glm::vec3 radial = actor->GetPos() - planet->GetPos();
        if (glm::length(radial) > 1e-6f) {
            up = glm::normalize(radial);
        }
    }

    if (glm::length(up) < 1e-6f) {
        up = glm::vec3(0.0f, 1.0f, 0.0f);
    } else {
        up = glm::normalize(up);
    }

    glm::vec3 forward = actor->GetForwardVec();
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
    forward = glm::normalize(glm::cross(left, up));

    glm::mat4 surfaceOrientation(1.0f);
    surfaceOrientation[0] = glm::vec4(left, 0.0f);
    surfaceOrientation[1] = glm::vec4(up, 0.0f);
    surfaceOrientation[2] = glm::vec4(forward, 0.0f);

    return glm::translate(glm::mat4(1.0f), actor->GetPos()) *
           surfaceOrientation;
}

void StageGizmoController::ApplyPlanetSurfaceTranslation(
    Actor* actor, const glm::vec3& rawWorldPos)
{
    if (!actor) {
        return;
    }

    Planet* planet = actor->GetCurrentPlanet();
    if (!RequiresSphereSurfaceProjection(actor)) {
        actor->SetPos(rawWorldPos);

        if (Platform* platform = dynamic_cast<Platform*>(actor);
            platform && platform->GetMovementComponent() &&
            platform->GetCurrentPlanet()) {
            platform->GetMovementComponent()->SetEditorPreviewLocalPos(
                rawWorldPos - platform->GetCurrentPlanet()->GetPos());
        }
        return;
    }

    const glm::vec3 currentPos = actor->GetPos();
    const glm::vec3 currentRadial =
        currentPos - planet->GetPos();
    const float currentRadius = glm::length(currentRadial);
    if (currentRadius < 1e-6f) {
        actor->SetPos(rawWorldPos);
        return;
    }

    const glm::vec3 currentNormal = currentRadial / currentRadius;
    const glm::vec3 rawDelta = rawWorldPos - currentPos;
    const float radialDelta = glm::dot(rawDelta, currentNormal);
    const glm::vec3 tangentDelta =
        rawDelta - currentNormal * radialDelta;

    glm::vec3 newDirection = currentNormal;
    const float tangentDistance = glm::length(tangentDelta);
    if (tangentDistance > 1e-6f) {
        const glm::vec3 tangentDirection =
            tangentDelta / tangentDistance;
        glm::vec3 rotationAxis =
            glm::cross(currentNormal, tangentDirection);
        if (glm::length(rotationAxis) > 1e-6f) {
            rotationAxis = glm::normalize(rotationAxis);
            const float surfaceAngle =
                tangentDistance / currentRadius;
            newDirection =
                glm::normalize(
                    glm::angleAxis(surfaceAngle, rotationAxis) *
                    currentNormal);
        }
    }

    const float newRadius =
        std::max(0.01f, currentRadius + radialDelta);
    const glm::vec3 newPos =
        planet->GetPos() + newDirection * newRadius;
    actor->SetPos(newPos);

    const float theta = std::atan2(newDirection.z, newDirection.x);
    const float phi = std::asin(glm::clamp(newDirection.y, -1.0f, 1.0f));
    actor->SetSphericalPlacement(
        theta,
        phi,
        newRadius - planet->GetRadius());

    if (Platform* platform = dynamic_cast<Platform*>(actor);
        platform && platform->GetMovementComponent()) {
        platform->GetMovementComponent()->SetEditorPreviewLocalPos(
            newPos - planet->GetPos());
    }
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

        if (mContext.game && mContext.game->GetMathUtils()) {
            const glm::vec3 left = glm::normalize(glm::cross(up, forward));
            forward = glm::normalize(glm::cross(left, up));

            glm::mat3 orientationMatrix(1.0f);
            orientationMatrix[0] = left;
            orientationMatrix[1] = up;
            orientationMatrix[2] = forward;

            const glm::quat orientation = glm::normalize(glm::quat_cast(orientationMatrix));
            actor->SetOrientation(orientation);
            actor->SetEditorRotation(
                mContext.game->GetMathUtils()->CalculateActorEditorRotationFromOrientation(actor, orientation));
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

        if (Planet* planet = dynamic_cast<Planet*>(actor)) {
            const glm::vec3 previousCenter = planet->GetPos();
            planet->SetPos(worldPos);
            StageActorPlanetBindingService::TranslateActorsBoundToPlanet(
                planet,
                worldPos - previousCenter);
            return;
        }

        actor->SetPos(worldPos);

        if (Platform* platform = dynamic_cast<Platform*>(actor);
            platform && platform->GetMovementComponent() &&
            platform->GetCurrentPlanet()) {
            platform->GetMovementComponent()->SetEditorPreviewLocalPos(
                worldPos - platform->GetCurrentPlanet()->GetPos());
        }
        return;
    }

    if (operation == ImGuizmo::SCALE) {
        const glm::vec3 previousScale = actor->GetScale();
        const glm::vec3 actorScale = glm::max(
            glm::vec3(scale[0], scale[1], scale[2]),
            glm::vec3(0.1f));

        if (Planet* planet = dynamic_cast<Planet*>(actor)) {
            planet->SetScale(actorScale);
            planet->SetRadius(actorScale.x);
            planet->SetTextureTiling(
                glm::vec2(
                    std::max(
                        1.0f,
                        std::sqrt(std::abs(actorScale.x * actorScale.z))),
                    std::max(1.0f, std::abs(actorScale.y))));
            StageActorPlanetBindingService::
                ReprojectSurfaceActorsAfterPlanetScale(planet);
            return;
        }

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

    float viewportX = 0.0f;
    float viewportY = 0.0f;
    float viewportWidth = 0.0f;
    float viewportHeight = 0.0f;
    if (mContext.gameViewport.IsValid()) {
        viewportX = mContext.gameViewport.x;
        viewportY = mContext.gameViewport.y;
        viewportWidth = mContext.gameViewport.width;
        viewportHeight = mContext.gameViewport.height;
    } else {
        int windowWidth = 0;
        int windowHeight = 0;
        glfwGetWindowSize(
            mContext.game->GetWindow(),
            &windowWidth,
            &windowHeight);
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        viewportX = viewport->Pos.x;
        viewportY = viewport->Pos.y;
        viewportWidth = static_cast<float>(windowWidth);
        viewportHeight = static_cast<float>(windowHeight);
    }

    if (viewportWidth <= 0.0f || viewportHeight <= 0.0f) {
        return;
    }

    const glm::mat4 view = views[0];
    const float aspect = viewportWidth / viewportHeight;
    const float fieldOfViewDegrees =
        mContext.game->GetCameraSystem()->GetFieldOfViewDegrees();
    const glm::mat4 projection = glm::perspective(
        glm::radians(fieldOfViewDegrees),
        aspect,
        0.1f,
        100.0f);

    ImGuizmo::BeginFrame();
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());

    ImGuizmo::SetRect(
        viewportX,
        viewportY,
        viewportWidth,
        viewportHeight);

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

    // 惑星移動時には所属アクターも追従させる必要がある。現状はその処理を持つ
    // 惑星設定パネルからのみ変形し、汎用ギズモによる不完全な変更を防ぐ。
    Planet* selectedPlanet = dynamic_cast<Planet*>(selectedActor);
    if (selectedPlanet && mCurrentGizmoOperation == ImGuizmo::ROTATE) {
        return;
    }

    if (!mIsUsingTransformGizmo) {
        mEditingGizmoMatrix =
            UsesPlanetSurfaceTranslation(selectedActor)
                ? CreatePlanetSurfaceTranslationMatrix(selectedActor)
                : CreateSelectedActorGizmoMatrix(
                      selectedActor, mCurrentGizmoOperation);
    }

    const bool usesPlanetSurfaceTranslation =
        UsesPlanetSurfaceTranslation(selectedActor);
    const ImGuizmo::MODE gizmoMode =
        mCurrentGizmoOperation == ImGuizmo::ROTATE ||
                usesPlanetSurfaceTranslation
            ? ImGuizmo::LOCAL
            : ImGuizmo::WORLD;
    ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), mCurrentGizmoOperation, gizmoMode,
                         glm::value_ptr(mEditingGizmoMatrix));

    if (ImGuizmo::IsUsing()) {
        if (!mIsUsingTransformGizmo) {
            if (mPushUndoCallback) {
                mPushUndoCallback();
            }

            mIsUsingTransformGizmo = true;
        }

        if (usesPlanetSurfaceTranslation) {
            ApplyPlanetSurfaceTranslation(
                selectedActor, glm::vec3(mEditingGizmoMatrix[3]));
            mEditingGizmoMatrix[3] =
                glm::vec4(selectedActor->GetPos(), 1.0f);
        } else {
            ApplyGizmoMatrixToActor(
                selectedActor,
                mEditingGizmoMatrix,
                mCurrentGizmoOperation);
        }

        if (!selectedPlanet) {
            StageActorPlanetBindingService::RefreshNearestPlanetBinding(
                mContext.game->GetCurrentStage(),
                selectedActor);
            if (mCurrentGizmoOperation == ImGuizmo::TRANSLATE) {
                selectedActor->CaptureEditorAuthoredPosition();
            } else if (mCurrentGizmoOperation == ImGuizmo::ROTATE) {
                selectedActor->CaptureEditorAuthoredRotation();
            } else if (mCurrentGizmoOperation == ImGuizmo::SCALE) {
                selectedActor->CaptureEditorAuthoredScale();
            }
        } else if (mCurrentGizmoOperation == ImGuizmo::TRANSLATE) {
            selectedPlanet->CaptureEditorAuthoredPosition();
        } else if (mCurrentGizmoOperation == ImGuizmo::SCALE) {
            selectedPlanet->CaptureEditorAuthoredScale();
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
}
