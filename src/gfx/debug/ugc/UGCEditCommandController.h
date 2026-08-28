#pragma once

#include "gfx/debug/DebugEditorContext.h"

class StageAddActorPanel;
class StageEditCommandController;
class StageSelectionController;
class UGCEditorToolState;
class UGCEditorTutorial;
class UGCSelectionDragState;

class UGCEditCommandController {
public:
    UGCEditCommandController(
        DebugEditorContext& context,
        StageAddActorPanel& stageAddActorPanel,
        StageSelectionController& selectionController,
        StageEditCommandController& editCommandController,
        UGCEditorTutorial& editorTutorial,
        UGCEditorToolState& toolState,
        UGCSelectionDragState& dragState);

    void HandleUndo();
    void HandleRedo();
    void ToggleEraser();
    void ActivateSelectionMode();
    void MoveSelectionOnGrid(int gridX, int gridZ);

private:
    DebugEditorContext& mContext;
    StageAddActorPanel& mStageAddActorPanel;
    StageSelectionController& mSelectionController;
    StageEditCommandController& mEditCommandController;
    UGCEditorTutorial& mEditorTutorial;
    UGCEditorToolState& mToolState;
    UGCSelectionDragState& mDragState;
};
