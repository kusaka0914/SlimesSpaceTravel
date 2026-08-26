#include "gfx/debug/stage/UGCPlacementPresetController.h"

#include "Game.h"
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
            "platform.obj", glm::vec3(1.0f));
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
            "enemy.obj", glm::vec3(0.25f));
        return true;
    case UGCPresetKind::EllipsePlanet:
        mPlacementController.CancelPlacement();
        if (mPushUndoCallback) {
            mPushUndoCallback();
        }
        return mCreateService.AddEllipsePlanet("planet.obj");
    case UGCPresetKind::PressureSwitch:
        mPlacementController.BeginPlacement(
            "スイッチ",
            0,
            [this](int planetIndex, const StageActorPlacement& placement) {
                if (mPushUndoCallback) {
                    mPushUndoCallback();
                }
                const StageActorPlacement worldUpPlacement =
                    CreateWorldUpPlacement(placement);
                return mCreateService.AddPressureSwitchPlatform(
                    planetIndex,
                    "platform.obj",
                    glm::vec3(0.75f, 0.2f, 0.75f),
                    &worldUpPlacement);
            });
        mPlacementController.SetPlacementPreviewModel(
            "platform.obj", glm::vec3(0.75f, 0.2f, 0.75f));
        return true;
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
            "star.obj", glm::vec3(0.3f));
        return true;
    case UGCPresetKind::MovingPlatform: {
        if (!mPlatformCellService.RefreshGeneratedPlatforms()) {
            return false;
        }
        if (mSelectionController) {
            mSelectionController->Clear();
        }
        auto startPlacement =
            std::make_shared<std::optional<StageActorPlacement>>();
        mPlacementController.BeginPlacement(
            "移動足場：出発点",
            0,
            [this, startPlacement](
                int planetIndex,
                const StageActorPlacement& placement) {
                if (!*startPlacement) {
                    *startPlacement = CreateWorldUpPlacement(placement);
                    mPlacementController.SetPlacementPrompt(
                        "移動足場：到着点",
                        "到着点をクリックしてください");
                    return true;
                }
                if (mPushUndoCallback) {
                    mPushUndoCallback();
                }
                const float gridSize = mContext.game->GetUGCGridSize();
                const glm::ivec3 startCell =
                    UGCPlatformGrid::CalculateGridPosition(
                        (*startPlacement)->worldPosition, gridSize);
                const glm::ivec3 endCell =
                    UGCPlatformGrid::CalculateGridPosition(
                        placement.worldPosition, gridSize);
                const bool created = mPlatformCellService.AddCell(
                    planetIndex,
                    **startPlacement,
                    gridSize,
                    mPlatformFootprintSideLength,
                    "moving",
                    endCell - startCell);
                if (created) {
                    *startPlacement = std::nullopt;
                    mPlacementController.SetPlacementDisplayName(
                        "移動足場：出発点");
                    if (mSelectionController) {
                        mSelectionController->Clear();
                    }
                }
                return created;
            });
        mPlacementController.SetPlacementPreviewModel(
            "platform.obj",
            UGCPlatformGrid::CalculateFootprintPreviewScale(
                mContext.game->GetUGCGridSize(),
                mPlatformFootprintSideLength));
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
            "platform.obj",
            UGCPlatformGrid::CalculateFootprintPreviewScale(
                mContext.game->GetUGCGridSize(),
                mPlatformFootprintSideLength));
        return true;
    }
    case UGCPresetKind::TwoPlayerSwitch: {
        auto firstPlacement =
            std::make_shared<std::optional<StageActorPlacement>>();
        mPlacementController.BeginPlacement(
            "2人用スイッチ：1つ目",
            0,
            [this, firstPlacement](
                int planetIndex,
                const StageActorPlacement& placement) {
                if (!*firstPlacement) {
                    *firstPlacement = CreateWorldUpPlacement(placement);
                    mPlacementController.SetPlacementPrompt(
                        "2人用スイッチ：2つ目",
                        "もう1つのスイッチをクリックしてください");
                    return true;
                }
                if (mPushUndoCallback) {
                    mPushUndoCallback();
                }
                const bool created =
                    mCreateService.AddTwoPlayerSwitchPair(
                        planetIndex,
                        **firstPlacement,
                        CreateWorldUpPlacement(placement));
                if (created) {
                    *firstPlacement = std::nullopt;
                }
                return created;
            });
        mPlacementController.SetPlacementPreviewModel(
            "platform.obj", glm::vec3(0.75f, 0.2f, 0.75f));
        return true;
    }
    }

    return false;
}
