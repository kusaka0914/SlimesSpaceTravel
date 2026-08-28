#include <GL/glew.h>

#include "gfx/debug/ugc/UGCPreviewController.h"

#include "actor/Actor.h"
#include "system/MeshLoadSystem.h"

UGCPreviewController::UGCPreviewController(Game* actorOwner)
    : mActorOwner(actorOwner)
{
}

UGCPreviewController::~UGCPreviewController() = default;

void UGCPreviewController::SetRenderSize(int width, int height)
{
    mPreviewState.SetRequestedRenderSize(width, height);
}

void UGCPreviewController::SetMeshLoadSystem(
    MeshLoadSystem& meshLoadSystem)
{
    mMeshLoadSystem = &meshLoadSystem;
}

int UGCPreviewController::GetRenderWidth() const
{
    return mPreviewState.GetRequestedRenderWidth();
}

int UGCPreviewController::GetRenderHeight() const
{
    return mPreviewState.GetRequestedRenderHeight();
}

void UGCPreviewController::AdjustYaw(float yawDeltaRadians)
{
    mPreviewState.AdjustYawRadians(yawDeltaRadians);
}

float UGCPreviewController::GetYaw() const
{
    return mPreviewState.GetYawRadians();
}

void UGCPreviewController::ToggleVerticalView()
{
    mPreviewState.ToggleVerticalView();
}

bool UGCPreviewController::IsViewedFromBelow() const
{
    return mPreviewState.IsViewedFromBelow();
}

float UGCPreviewController::UpdateFocusY(float deltaTime)
{
    return mPreviewState.UpdateFocusY(mGridSize, deltaTime);
}

float UGCPreviewController::GetFocusY() const
{
    return mPreviewState.GetFocusY();
}

void UGCPreviewController::SetEditLayer(int gridLayer)
{
    mPreviewState.SetEditLayer(gridLayer);
}

int UGCPreviewController::GetEditLayer() const
{
    return mPreviewState.GetEditLayer();
}

void UGCPreviewController::SetGridSize(float gridSize)
{
    mGridSize = gridSize > 0.01f ? gridSize : 0.01f;
}

float UGCPreviewController::GetGridSize() const
{
    return mGridSize;
}

void UGCPreviewController::SetOrthographicHalfHeight(float halfHeight)
{
    mOrthographicHalfHeight = halfHeight > 0.1f ? halfHeight : 0.1f;
}

float UGCPreviewController::GetOrthographicHalfHeight() const
{
    return mOrthographicHalfHeight;
}

void UGCPreviewController::SetPlatformPlacementPosition(
    const std::optional<glm::vec3>& position)
{
    mPlatformPlacementPosition = position;
}

const std::optional<glm::vec3>&
UGCPreviewController::GetPlatformPlacementPosition() const
{
    return mPlatformPlacementPosition;
}

void UGCPreviewController::SetMovingPlatformPath(
    const std::optional<glm::vec3>& startPosition,
    const std::optional<glm::vec3>& destinationPosition)
{
    mMovingPlatformPathStart = startPosition;
    mMovingPlatformPathDestination = destinationPosition;
}

const std::optional<glm::vec3>&
UGCPreviewController::GetMovingPlatformPathStart() const
{
    return mMovingPlatformPathStart;
}

const std::optional<glm::vec3>&
UGCPreviewController::GetMovingPlatformPathDestination() const
{
    return mMovingPlatformPathDestination;
}

void UGCPreviewController::SetPlacementModel(
    const std::optional<glm::vec3>& position,
    const std::string& modelPath,
    const glm::vec3& scale,
    const std::string& textureOverridePath)
{
    mPlacementModelPositions.clear();
    if (!position || modelPath.empty()) {
        mPlacementModel.reset();
        return;
    }

    if (!mPlacementModel || mPlacementModel->GetModelPath() != modelPath) {
        mPlacementModel = std::make_unique<Actor>(mActorOwner);
        mPlacementModel->SetModelPath(modelPath);
        if (mMeshLoadSystem) {
            mMeshLoadSystem->SetActorMesh(mPlacementModel.get());
        }
    }

    mPlacementModel->SetPos(*position);
    mPlacementModel->SetScale(scale);
    mPlacementModel->SetTextureOverridePath(textureOverridePath);
    mPlacementModel->SetIsActive(true);
}

void UGCPreviewController::SetPlacementModelPositions(
    const std::vector<glm::vec3>& positions,
    const std::string& modelPath,
    const glm::vec3& scale,
    const std::string& textureOverridePath)
{
    if (positions.empty()) {
        SetPlacementModel(std::nullopt, {}, scale, textureOverridePath);
        return;
    }

    SetPlacementModel(
        positions.front(), modelPath, scale, textureOverridePath);
    mPlacementModelPositions = positions;
}

Actor* UGCPreviewController::GetPlacementModel() const
{
    return mPlacementModel.get();
}

const std::vector<glm::vec3>&
UGCPreviewController::GetPlacementModelPositions() const
{
    return mPlacementModelPositions;
}
