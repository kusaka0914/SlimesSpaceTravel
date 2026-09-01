#pragma once

#include "gfx/debug/DebugPanel.h"
#include "gfx/debug/stage/StageActorCreateService.h"
#include "gfx/debug/stage/StageActorCreationForms.h"
#include "gfx/debug/stage/StageActorPlacementController.h"
#include "gfx/debug/stage/StageActorPlacementResolver.h"
#include "gfx/debug/stage/StageBoatCreationForm.h"
#include "gfx/debug/stage/StageCollectibleCreationForms.h"
#include "gfx/debug/stage/StageEditorTypes.h"
#include "gfx/debug/stage/StageWorldCreationForms.h"
#include "gfx/debug/stage/UGCPlatformCellService.h"
#include "gfx/debug/stage/UGCPlatformEditController.h"
#include "gfx/debug/stage/UGCPlacementPresetController.h"

#include <functional>
#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class StageSelectionController;

class StageAddActorPanel : public DebugPanel {
public:
    explicit StageAddActorPanel(DebugEditorContext& context);

    void Draw() override;
    void UpdatePlacement();
    void SetSelectionController(StageSelectionController* selectionController);
    void SetPushUndoCallback(std::function<void()> pushUndoCallback);
    void SetPlacementCompletedCallback(
        std::function<void()> placementCompletedCallback)
    {
        mPlacementController.SetPlacementCompletedCallback(
            std::move(placementCompletedCallback));
    }
    void SetUGCEditLayer(int gridLayer)
    {
        mPlacementController.SetUGCEditLayer(gridLayer);
        mUGCPlatformEditController.SetGridLayer(gridLayer);
    }
    void SetUGCPlatformFootprintSideLength(int sideLength)
    {
        mPlacementController.SetUGCPlatformFootprintSideLength(sideLength);
        mUGCPresetController.SetPlatformFootprintSideLength(sideLength);
    }
    bool BeginDuplicatePlacement(const StageActorRef& sourceRef);
    bool ActivateUGCPreset(UGCPresetKind presetKind);
    bool TryEraseUGCPlatformCell();
    void EndUGCEraseGesture();
    bool TryTranslateUGCPlatformCells(
        const StageActorRef& actorRef,
        const glm::vec3& worldDelta);
    bool TryTranslateUGCPlatformCells(
        const std::vector<StageActorRef>& actorRefs,
        const glm::vec3& worldDelta);
    bool TryTranslateUGCMovingPlatformDestinations(
        const std::vector<StageActorRef>& actorRefs,
        const glm::vec3& worldDelta);
    bool TrySaveUGCMovingPlatformDestinationTranslation(
        const std::vector<StageActorRef>& actorRefs,
        const glm::vec3& worldDelta);
    bool IsPlacementActive() const
    {
        return mPlacementController.IsPlacementActive();
    }
    const std::optional<glm::vec3>& GetPlacementPreviewPosition() const
    {
        return mPlacementController.GetPlacementPreviewPosition();
    }
    const std::string& GetPlacementDisplayName() const
    {
        return mPlacementController.GetPlacementDisplayName();
    }
    const std::string& GetPlacementStatus() const
    {
        return mPlacementController.GetPlacementStatus();
    }
    void CancelPlacement();

private:
    StageActorCreateService mCreateService;
    UGCPlatformCellService mUGCPlatformCellService;
    StageActorPlacementResolver mPlacementResolver;
    UGCPlatformEditController mUGCPlatformEditController;
    StageActorPlacementController mPlacementController;
    UGCPlacementPresetController mUGCPresetController;
    StageJewelItemCreationForm mJewelItemCreationForm;
    StageHazardActorCreationForm mHazardActorCreationForm;
    StageBoatArrivalPointCreationForm mBoatArrivalPointCreationForm;
    StageEnemyCreationForm mEnemyCreationForm;
    StageNPCCreationForm mNPCCreationForm;
    StageTutorialTriggerCreationForm mTutorialTriggerCreationForm;
    StageObjectCreationForm mStageObjectCreationForm;
    StagePlanetCreationForm mPlanetCreationForm;
    StagePlatformCreationForm mPlatformCreationForm;
    StageCrystalCreationForm mCrystalCreationForm;
    StageBoatPartsCreationForm mBoatPartsCreationForm;
    StageBoatCreationForm mBoatCreationForm;
    StageStarCreationForm mStarCreationForm;
};
