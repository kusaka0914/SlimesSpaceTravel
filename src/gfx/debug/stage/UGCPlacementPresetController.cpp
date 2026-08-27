#include "gfx/debug/stage/UGCPlacementPresetController.h"

#include "Game.h"
#include "actor/Platform.h"
#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/stage/StageActorCreateService.h"
#include "gfx/debug/stage/StageActorPlacementController.h"
#include "gfx/debug/stage/StageSelectionController.h"
#include "gfx/debug/stage/UGCPlatformCellService.h"
#include "gfx/debug/stage/UGCPlatformGrid.h"

#include <memory>
#include <optional>
#include <utility>

namespace {

StageActorPlacement CreateWorldUpPlacement(
    const StageActorPlacement& surfacePlacement)
{
    StageActorPlacement worldUpPlacement = surfacePlacement;
    worldUpPlacement.surfaceNormal = glm::vec3(0.0f, 1.0f, 0.0f);
    return worldUpPlacement;
}

}

UGCPlacementPresetController::UGCPlacementPresetController(
    DebugEditorContext& context,
    StageActorCreateService& actorCreateService,
    UGCPlatformCellService& platformCellService,
    StageActorPlacementController& placementController)
    : mContext(context),
      mCreateService(actorCreateService),
      mPlatformCellService(platformCellService),
      mPlacementController(placementController)
{
}

void UGCPlacementPresetController::SetSelectionController(
    StageSelectionController* selectionController)
{
    mSelectionController = selectionController;
}

void UGCPlacementPresetController::SetPushUndoCallback(
    std::function<void()> pushUndoCallback)
{
    mPushUndoCallback = std::move(pushUndoCallback);
}

bool UGCPlacementPresetController::ActivatePreset(UGCPresetKind presetKind)
{
    const UGCPresetVisual& presetVisual =
        GetUGCPresetVisual(presetKind);

    switch (presetKind) {
    case UGCPresetKind::NormalPlatform:
        if (!mPlatformCellService.RefreshGeneratedPlatforms()) {
            return false;
        }
        if (mSelectionController) {
            mSelectionController->Clear();
        }
        mPlacementController.BeginPlacement(
            "通常足場",
            0,
            [this](int planetIndex, const StageActorPlacement& placement) {
                const StageActorPlacement worldUpPlacement =
                    CreateWorldUpPlacement(placement);
                const float gridSize = mContext.game->GetUGCGridSize();
                const bool added = mPlatformCellService.AddCell(
                    planetIndex,
                    worldUpPlacement,
                    gridSize,
                    mPlatformFootprintSideLength);
                if (added && mSelectionController) {
                    mSelectionController->Clear();
                }
                return added;
            },
            true,
            true,
            false,
            true);
        mPlacementController.SetPlacementPreviewModel(
            presetVisual.modelPath,
            UGCPlatformGrid::CalculateFootprintPreviewScale(
                mContext.game->GetUGCGridSize(),
                mPlatformFootprintSideLength),
            presetVisual.initialTextureOverridePath);
        return true;
    case UGCPresetKind::NormalEnemy:
        mPlacementController.BeginPlacement(
            "通常敵",
            0,
            [this](int planetIndex, const StageActorPlacement& placement) {
                if (mPushUndoCallback) {
                    mPushUndoCallback();
                }
                const StageActorPlacement worldUpPlacement =
                    CreateWorldUpPlacement(placement);
                return mCreateService.AddEnemy(
                    "normal", planetIndex, &worldUpPlacement);
            });
        mPlacementController.SetPlacementPreviewModel(
            presetVisual.modelPath,
            presetVisual.thumbnailScale,
            presetVisual.initialTextureOverridePath);
        return true;
    case UGCPresetKind::EllipsePlanet:
        mPlacementController.BeginPlacement(
            "惑星",
            0,
            [this, modelPath = presetVisual.modelPath](
                int,
                const StageActorPlacement& placement) {
                if (mPushUndoCallback) {
                    mPushUndoCallback();
                }
                return mCreateService.AddEllipsePlanetAtPosition(
                    modelPath,
                    placement.worldPosition);
            });
        mPlacementController.SetPlacementPreviewModel(
            presetVisual.modelPath,
            presetVisual.thumbnailScale,
            presetVisual.initialTextureOverridePath);
        return true;
    case UGCPresetKind::PressureSwitch:
        {
        const glm::vec3 switchScale = presetVisual.thumbnailScale;
        const std::string switchTexturePath =
            presetVisual.initialTextureOverridePath;
        mPlacementController.BeginPlacement(
            "スイッチ：配置位置",
            0,
            [this, switchScale, switchTexturePath](
                int planetIndex,
                const StageActorPlacement& placement) {
                const StageActorPlacement worldUpPlacement =
                    CreateWorldUpPlacement(placement);
                mPlacementController.BeginTargetPlatformSelection(
                    "スイッチ：表示する足場",
                    worldUpPlacement.worldPosition,
                    [this,
                     planetIndex,
                     worldUpPlacement,
                     switchScale,
                     switchTexturePath](Platform* targetPlatform) {
                        const std::string& targetPlatformId =
                            targetPlatform->GetPlatformId();
                        if (targetPlatformId.empty()) {
                            return false;
                        }

                        if (mPushUndoCallback) {
                            mPushUndoCallback();
                        }
                        return mCreateService.AddPressureSwitchPlatform(
                                   planetIndex,
                                   "platform.obj",
                                   switchScale,
                                   switchTexturePath,
                                   targetPlatformId,
                                   &worldUpPlacement)
                            .has_value();
                    },
                    [this,
                     planetIndex,
                     worldUpPlacement,
                     switchScale,
                     switchTexturePath]() {
                        if (mPushUndoCallback) {
                            mPushUndoCallback();
                        }
                        return mCreateService.AddPressureSwitchPlatform(
                                   planetIndex,
                                   "platform.obj",
                                   switchScale,
                                   switchTexturePath,
                                   "",
                                   &worldUpPlacement)
                            .has_value();
                    },
                    [this]() {
                        ActivatePreset(UGCPresetKind::PressureSwitch);
                    });
                return true;
            });
        mPlacementController.SetPlacementPreviewModel(
            presetVisual.modelPath,
            presetVisual.thumbnailScale,
            presetVisual.initialTextureOverridePath);
        return true;
        }
    case UGCPresetKind::GoalStar:
        mPlacementController.BeginPlacement(
            "ゴールの星",
            0,
            [this](int planetIndex, const StageActorPlacement& placement) {
                if (mPushUndoCallback) {
                    mPushUndoCallback();
                }
                return mCreateService.AddStar(planetIndex, &placement);
            });
        mPlacementController.SetPlacementPreviewModel(
            presetVisual.modelPath,
            presetVisual.thumbnailScale,
            presetVisual.initialTextureOverridePath);
        return true;
    case UGCPresetKind::MovingPlatform: {
        if (!mPlatformCellService.RefreshGeneratedPlatforms()) {
            return false;
        }
        if (mSelectionController) {
            mSelectionController->Clear();
        }
        mPlacementController.BeginMovingPlatformStrokePlacement(
            [this](
                int planetIndex,
                const std::vector<glm::ivec3>& startCells,
                const glm::ivec3& destinationOffset) {
                return mPlatformCellService.AddCells(
                    planetIndex,
                    startCells,
                    mContext.game->GetUGCGridSize(),
                    mPlatformFootprintSideLength,
                    "moving",
                    destinationOffset);
            });
        mPlacementController.SetPlacementPreviewModel(
            presetVisual.modelPath,
            UGCPlatformGrid::CalculateFootprintPreviewScale(
                mContext.game->GetUGCGridSize(),
                mPlatformFootprintSideLength),
            presetVisual.initialTextureOverridePath);
        return true;
    }
    case UGCPresetKind::FadingPlatform:
    case UGCPresetKind::AdhesivePlatform: {
        if (!mPlatformCellService.RefreshGeneratedPlatforms()) {
            return false;
        }
        if (mSelectionController) {
            mSelectionController->Clear();
        }
        const bool isFading =
            presetKind == UGCPresetKind::FadingPlatform;
        mPlacementController.BeginPlacement(
            isFading ? "消える足場" : "くっつき足場",
            0,
            [this, isFading](
                int planetIndex,
                const StageActorPlacement& placement) {
                const float gridSize = mContext.game->GetUGCGridSize();
                const bool added = mPlatformCellService.AddCell(
                    planetIndex,
                    CreateWorldUpPlacement(placement),
                    gridSize,
                    mPlatformFootprintSideLength,
                    isFading ? "fading" : "adhesive");
                if (added && mSelectionController) {
                    mSelectionController->Clear();
                }
                return added;
            },
            true,
            true,
            false,
            true);
        mPlacementController.SetPlacementPreviewModel(
            presetVisual.modelPath,
            UGCPlatformGrid::CalculateFootprintPreviewScale(
                mContext.game->GetUGCGridSize(),
                mPlatformFootprintSideLength),
            presetVisual.initialTextureOverridePath);
        return true;
    }
    case UGCPresetKind::TwoPlayerSwitch: {
        const glm::vec3 switchScale = presetVisual.thumbnailScale;
        const std::string switchTexturePath =
            presetVisual.initialTextureOverridePath;
        auto firstPlacement =
            std::make_shared<std::optional<StageActorPlacement>>();
        mPlacementController.BeginPlacement(
            "2人用スイッチ：1つ目",
            0,
            [this, firstPlacement, switchScale, switchTexturePath](
                int planetIndex,
                const StageActorPlacement& placement) {
                if (!*firstPlacement) {
                    *firstPlacement = CreateWorldUpPlacement(placement);
                    mPlacementController.SetFixedPlacementPreviewPositions(
                        {(*firstPlacement)->worldPosition});
                    mPlacementController.SetPlacementPrompt(
                        "2人用スイッチ：2つ目",
                        "もう1つのスイッチをクリックしてください");
                    return true;
                }
                const StageActorPlacement secondPlacement =
                    CreateWorldUpPlacement(placement);
                mPlacementController.SetFixedPlacementPreviewPositions(
                    {(*firstPlacement)->worldPosition,
                     secondPlacement.worldPosition});
                mPlacementController.BeginTargetPlatformSelection(
                    "2人用スイッチ：表示する足場",
                    (*firstPlacement)->worldPosition,
                    [this,
                     planetIndex,
                     firstPlacement,
                     secondPlacement,
                     switchScale,
                     switchTexturePath](Platform* targetPlatform) {
                        const std::string& targetPlatformId =
                            targetPlatform->GetPlatformId();
                        if (targetPlatformId.empty()) {
                            return false;
                        }
                        if (mPushUndoCallback) {
                            mPushUndoCallback();
                        }
                        return mCreateService.AddTwoPlayerSwitchPair(
                            planetIndex,
                            **firstPlacement,
                            secondPlacement,
                            switchScale,
                            switchTexturePath,
                            targetPlatformId);
                    },
                    [this,
                     planetIndex,
                     firstPlacement,
                     secondPlacement,
                     switchScale,
                     switchTexturePath]() {
                        if (mPushUndoCallback) {
                            mPushUndoCallback();
                        }
                        return mCreateService.AddTwoPlayerSwitchPair(
                            planetIndex,
                            **firstPlacement,
                            secondPlacement,
                            switchScale,
                            switchTexturePath,
                            "");
                    },
                    [this]() {
                        ActivatePreset(UGCPresetKind::TwoPlayerSwitch);
                    });
                return true;
            });
        mPlacementController.SetPlacementPreviewModel(
            presetVisual.modelPath,
            presetVisual.thumbnailScale,
            presetVisual.initialTextureOverridePath);
        return true;
    }
    }

    return false;
}
