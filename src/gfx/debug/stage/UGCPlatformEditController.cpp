#include "gfx/debug/stage/UGCPlatformEditController.h"

#include "Game.h"
#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/stage/StageActorPlacementResolver.h"
#include "gfx/debug/stage/StageSelectionController.h"
#include "gfx/debug/stage/UGCPlatformCellService.h"
#include "gfx/debug/stage/UGCPlatformGrid.h"
#include "imgui.h"

#include <cmath>
#include <utility>

UGCPlatformEditController::UGCPlatformEditController(
    DebugEditorContext& context,
    UGCPlatformCellService& platformCellService,
    StageActorPlacementResolver& placementResolver)
    : mContext(context),
      mPlatformCellService(platformCellService),
      mPlacementResolver(placementResolver)
{
}

void UGCPlatformEditController::SetSelectionController(
    StageSelectionController* selectionController)
{
    mSelectionController = selectionController;
}

void UGCPlatformEditController::SetPushUndoCallback(
    std::function<void()> pushUndoCallback)
{
    mPushUndoCallback = std::move(pushUndoCallback);
}

bool UGCPlatformEditController::TryEraseCell()
{
    if (!mSelectionController ||
        !mContext.game ||
        !mContext.game->GetCurrentStage() ||
        !mContext.game->GetPhysicsSystem()) {
        return false;
    }

    glm::vec3 rayFrom;
    glm::vec3 rayTo;
    if (!mSelectionController->TryCreateMouseRay(rayFrom, rayTo)) {
        return false;
    }

    const bool isSelectedMovingPlatformDestination =
        mSelectionController->IsMovingPlatformDestinationSelected();
    if (isSelectedMovingPlatformDestination &&
        !mMovingDestinationEraseRegion) {
        const std::optional<StageActorRef>& destinationRef =
            mSelectionController->GetPickedActorRef();
        UGCGeneratedPlatformRegion sourceRegion;
        if (!destinationRef ||
            !mPlatformCellService.ResolveMovingPlatformRegion(
                *destinationRef,
                sourceRegion)) {
            return false;
        }
        mMovingDestinationEraseRegion = sourceRegion;

        const glm::vec3 destinationCenter = mSelectionController
            ->CalculateSelectedMovingPlatformDestinationsCenter();
        const float gridSize = sourceRegion.gridSize;
        mMovingDestinationEraseLayer = static_cast<int>(std::round(
            destinationCenter.y / gridSize));
    }
    const bool isMovingPlatformDestination =
        mMovingDestinationEraseRegion.has_value();
    int eraseLayer = mMovingDestinationEraseLayer
        .value_or(mGridLayer);

    StageActorPlacement placement;
    if (!mPlacementResolver.TryResolveUGCBuildPlanePlacement(
            rayFrom, rayTo, eraseLayer, placement)) {
        return false;
    }

    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        mLastErasedCell.reset();
        return false;
    }

    if (isMovingPlatformDestination) {
        const float gridSize = mMovingDestinationEraseRegion->gridSize;
        const glm::ivec3 erasedDestinationCell =
            UGCPlatformGrid::CalculateGridPosition(
                placement.worldPosition, gridSize);
        if (mLastErasedCell &&
            *mLastErasedCell == erasedDestinationCell) {
            return false;
        }
        if (mPushUndoCallback) {
            mPushUndoCallback();
        }
        const bool removed = mPlatformCellService
            .RemoveMovingPlatformDestinationCell(
                *mMovingDestinationEraseRegion,
                placement.worldPosition);
        if (removed) {
            mLastErasedCell = erasedDestinationCell;
            mSelectionController->Clear();
        }
        return removed;
    }

    const float gridSize = mContext.game->GetUGCGridSize();
    if (!mPlatformCellService.ResolveLayerAtGridPosition(
            0,
            placement.worldPosition,
            gridSize,
            mGridLayer,
            eraseLayer)) {
        return false;
    }

    const glm::ivec3 erasedCell(
        static_cast<int>(std::floor(placement.worldPosition.x / gridSize)),
        eraseLayer,
        static_cast<int>(std::floor(placement.worldPosition.z / gridSize)));
    if (mLastErasedCell && *mLastErasedCell == erasedCell) {
        return false;
    }

    if (mPushUndoCallback) {
        mPushUndoCallback();
    }
    const bool removed = mPlatformCellService.RemoveCellAtGridPosition(
        0,
        placement.worldPosition,
        gridSize,
        eraseLayer);
    if (removed) {
        mLastErasedCell = erasedCell;
        mSelectionController->Clear();
    }
    return removed;
}

void UGCPlatformEditController::EndEraseGesture()
{
    mLastErasedCell.reset();
    mMovingDestinationEraseRegion.reset();
    mMovingDestinationEraseLayer.reset();
}

bool UGCPlatformEditController::TryTranslateCells(
    const StageActorRef& actorRef,
    const glm::vec3& worldDelta)
{
    return mPlatformCellService.TranslateCells(actorRef, worldDelta);
}

bool UGCPlatformEditController::TryTranslateCells(
    const std::vector<StageActorRef>& actorRefs,
    const glm::vec3& worldDelta)
{
    return mPlatformCellService.TranslateCells(actorRefs, worldDelta);
}

bool UGCPlatformEditController::TryTranslateMovingPlatformDestinations(
    const std::vector<StageActorRef>& actorRefs,
    const glm::vec3& worldDelta)
{
    return mPlatformCellService.TranslateMovingPlatformDestinations(
        actorRefs, worldDelta);
}

bool UGCPlatformEditController::TrySaveMovingPlatformDestinationTranslation(
    const std::vector<StageActorRef>& actorRefs,
    const glm::vec3& worldDelta)
{
    return mPlatformCellService.SaveMovingPlatformDestinationTranslation(
        actorRefs, worldDelta);
}
