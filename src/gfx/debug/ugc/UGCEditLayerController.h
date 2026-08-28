#pragma once

#include "gfx/debug/DebugEditorContext.h"

class StageAddActorPanel;
class StageEditCommandController;
class StageSelectionController;
class UGCEditorToolState;
class UGCEditorTutorial;
class UGCSelectionDragState;

class UGCEditLayerController {
public:
    UGCEditLayerController(
        DebugEditorContext& context,
        StageAddActorPanel& stageAddActorPanel,
        StageSelectionController& selectionController,
        StageEditCommandController& editCommandController,
        UGCEditorTutorial& editorTutorial,
        UGCEditorToolState& toolState,
        UGCSelectionDragState& dragState);

    void ChangeLayer(int layerDelta);
    void ChangeEditLayer(int layerDelta);
    void SyncEditLayerToPickedActor();

private:
    DebugEditorContext& mContext;
    StageAddActorPanel& mStageAddActorPanel;
    StageSelectionController& mSelectionController;
    StageEditCommandController& mEditCommandController;
    UGCEditorTutorial& mEditorTutorial;
    UGCEditorToolState& mToolState;
    UGCSelectionDragState& mDragState;
};
