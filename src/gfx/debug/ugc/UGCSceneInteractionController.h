#pragma once

#include "gfx/debug/DebugEditorContext.h"

class StageAddActorPanel;
class StageEditCommandController;
class StageSelectionController;
class UGCEditLayerController;
class UGCEditorToolState;
class UGCEditorTutorial;
class UGCSelectionDragController;
class UGCSwitchConnectionState;

class UGCSceneInteractionController {
public:
    UGCSceneInteractionController(
        DebugEditorContext& context,
        StageAddActorPanel& stageAddActorPanel,
        StageSelectionController& selectionController,
        StageEditCommandController& editCommandController,
        UGCEditorTutorial& editorTutorial,
        UGCEditorToolState& toolState,
        UGCSwitchConnectionState& connectionState,
        UGCSelectionDragController& selectionDragController,
        UGCEditLayerController& editLayerController);

    void Update();

private:
    DebugEditorContext& mContext;
    StageAddActorPanel& mStageAddActorPanel;
    StageSelectionController& mSelectionController;
    StageEditCommandController& mEditCommandController;
    UGCEditorTutorial& mEditorTutorial;
    UGCEditorToolState& mToolState;
    UGCSwitchConnectionState& mConnectionState;
    UGCSelectionDragController& mSelectionDragController;
    UGCEditLayerController& mEditLayerController;
};
