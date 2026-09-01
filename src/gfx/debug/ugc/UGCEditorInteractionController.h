#pragma once

#include "gfx/debug/DebugEditorContext.h"

#include <glm/glm.hpp>

class StageActorYamlWriter;
class StageAddActorPanel;
class StageEditCommandController;
class StageSelectionController;
class UGCEditorViewController;
class UGCEditorToolState;
class UGCEditorTutorial;
class UGCSelectionDragState;
class UGCSwitchConnectionState;

class UGCEditorInteractionController {
public:
    UGCEditorInteractionController(
        DebugEditorContext& context,
        StageAddActorPanel& stageAddActorPanel,
        StageActorYamlWriter& stageActorYamlWriter,
        StageSelectionController& selectionController,
        StageEditCommandController& editCommandController,
        UGCEditorTutorial& editorTutorial,
        UGCEditorToolState& toolState,
        UGCSelectionDragState& dragState,
        UGCSwitchConnectionState& connectionState,
        UGCEditorViewController& viewController);

    void HandleUndo();
    void HandleRedo();
    void ToggleEraser();
    void ActivateSelectionMode();
    void AdjustZoom(float distanceMultiplier);
    void ChangeLayer(int layerDelta);
    void MoveSelectionOnGrid(int gridX, int gridZ);
    void UpdateSceneInteraction();
    void ChangeEditLayer(int layerDelta);
    void ToggleVerticalView();
    void SetFixedView(const glm::vec3& viewDirection);
    const glm::vec3& GetViewDirection() const;

private:
    bool TryIntersectDragPlane(
        const glm::vec3& rayFrom,
        const glm::vec3& rayTo,
        glm::vec3& outIntersection) const;
    void UpdateSelectionDrag();
    void SyncEditLayerToPickedActor();

    DebugEditorContext& mContext;
    StageAddActorPanel& mStageAddActorPanel;
    StageActorYamlWriter& mStageActorYamlWriter;
    StageSelectionController& mSelectionController;
    StageEditCommandController& mEditCommandController;
    UGCEditorTutorial& mEditorTutorial;
    UGCEditorToolState& mToolState;
    UGCSelectionDragState& mDragState;
    UGCSwitchConnectionState& mConnectionState;
    UGCEditorViewController& mViewController;
};
