#include "editor/ugc/UGCEditorViewController.h"

#include "app/Game.h"
#include "world/Stage.h"
#include "actor/Planet.h"
#include "editor/ugc/UGCEditorTutorial.h"
#include "system/CameraSystem.h"

#include <algorithm>
#include <cmath>

UGCEditorViewController::UGCEditorViewController(DebugEditorContext& context, UGCEditorTutorial& editorTutorial)
    : mContext(context),
      mEditorTutorial(editorTutorial)
{
}

void UGCEditorViewController::AdjustZoom(float distanceMultiplier)
{
    AdjustViewDistance(distanceMultiplier);
    mEditorTutorial.RecordViewAdjustment();
}

void UGCEditorViewController::ToggleVerticalView()
{
    if (!mContext.game || !mContext.game->GetCameraSystem()) {
        return;
    }

    mContext.game->ToggleUGCPreviewVerticalView();
    const bool isViewedFromBelow = mContext.game->GetIsUGCPreviewViewedFromBelow();
    const glm::vec3 viewDirection = isViewedFromBelow ? glm::vec3(0.0f, -1.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);

    CameraSystem* cameraSystem = mContext.game->GetCameraSystem();
    CameraPose pose = cameraSystem->GetDebugCameraPose();
    const float currentViewDistance = glm::length(pose.position - pose.target);
    constexpr float fallbackViewDistance = 30.0f;
    const float viewDistance = currentViewDistance > 0.0001f ? currentViewDistance : fallbackViewDistance;

    pose.position = pose.target + viewDirection * viewDistance;
    pose.up = isViewedFromBelow ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 0.0f, -1.0f);
    cameraSystem->SetDebugCameraPose(pose);

    mViewDirection = viewDirection;
    mContext.game->SetIsUGCOrthographicView(true);
    mContext.game->SetFreeCameraMode(true);
}

void UGCEditorViewController::SetFixedView(const glm::vec3& viewDirection)
{
    if (!mContext.game || !mContext.game->GetCameraSystem() || !mContext.game->GetCurrentStage()) {
        return;
    }

    glm::vec3 stageCenter(0.0f);
    float viewDistance = 24.0f;
    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();
    if (!planets.empty()) {
        for (const Planet* planet : planets) {
            if (planet) {
                stageCenter += planet->GetPos();
            }
        }
        stageCenter /= static_cast<float>(planets.size());
        for (const Planet* planet : planets) {
            if (!planet) {
                continue;
            }
            viewDistance =
                std::max(viewDistance, glm::length(planet->GetPos() - stageCenter) +
                                           std::max({std::abs(planet->GetScale().x), std::abs(planet->GetScale().y),
                                                     std::abs(planet->GetScale().z)}) *
                                               3.0f);
        }
    }

    const glm::vec3 normalizedDirection = glm::normalize(viewDirection);
    const glm::vec3 absoluteDirection = glm::abs(normalizedDirection);
    const int activeAxisCount = (absoluteDirection.x > 0.001f ? 1 : 0) + (absoluteDirection.y > 0.001f ? 1 : 0) +
                                (absoluteDirection.z > 0.001f ? 1 : 0);
    const bool isAxisAlignedView = activeAxisCount == 1;
    mViewDirection = normalizedDirection;
    CameraPose pose;
    pose.position = stageCenter + normalizedDirection * viewDistance;
    pose.target = stageCenter;
    if (normalizedDirection.y > 0.9f) {
        pose.up = glm::vec3(0.0f, 0.0f, -1.0f);
    } else if (normalizedDirection.y < -0.9f) {
        pose.up = glm::vec3(0.0f, 0.0f, 1.0f);
    } else {
        pose.up = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    pose.fieldOfViewDegrees = 55.0f;
    mContext.game->SetIsUGCOrthographicView(isAxisAlignedView);
    if (isAxisAlignedView) {
        // 同じ視点距離の透視投影と表示範囲を揃え、投影方式の切り替えで大きさが跳ねるのを防ぐ。
        const float matchingPerspectiveHalfHeight =
            viewDistance * std::tan(glm::radians(pose.fieldOfViewDegrees) * 0.5f);
        mContext.game->SetUGCOrthographicHalfHeight(matchingPerspectiveHalfHeight);
    }
    mContext.game->GetCameraSystem()->SetDebugCameraPose(pose);
    mContext.game->SetFreeCameraMode(true);
}

void UGCEditorViewController::AdjustViewDistance(float distanceMultiplier)
{
    if (!mContext.game || !mContext.game->GetCameraSystem() || !mContext.game->GetCurrentStage() ||
        distanceMultiplier <= 0.0f) {
        return;
    }

    CameraSystem* cameraSystem = mContext.game->GetCameraSystem();
    if (mContext.game->GetIsUGCOrthographicView()) {
        constexpr float minimumHalfHeight = 1.0f;
        constexpr float maximumHalfHeight = 250.0f;
        const float nextHalfHeight = glm::clamp(mContext.game->GetUGCOrthographicHalfHeight() * distanceMultiplier,
                                                minimumHalfHeight, maximumHalfHeight);
        mContext.game->SetUGCOrthographicHalfHeight(nextHalfHeight);
        return;
    }

    CameraPose pose = cameraSystem->GetDebugCameraPose();

    const glm::vec3 targetToCamera = pose.position - pose.target;
    const float currentDistance = glm::length(targetToCamera);
    constexpr float minimumDistance = 3.0f;
    constexpr float maximumDistance = 250.0f;
    if (currentDistance <= 0.0001f) {
        return;
    }

    const float nextDistance = glm::clamp(currentDistance * distanceMultiplier, minimumDistance, maximumDistance);
    pose.position = pose.target + targetToCamera / currentDistance * nextDistance;
    cameraSystem->SetDebugCameraPose(pose);
    mContext.game->SetFreeCameraMode(true);
}

const glm::vec3& UGCEditorViewController::GetViewDirection() const
{
    return mViewDirection;
}
