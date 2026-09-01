#pragma once

#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/ugc/UGCEditorToolState.h"
#include "gfx/debug/ugc/UGCSwitchConnectionState.h"

class StageAddActorPanel;
class StageSelectionController;
class UGCEditorInteractionController;
class UGCSwitchConnectionController;

class UGCSceneOverlayRenderer {
public:
    UGCSceneOverlayRenderer(
        DebugEditorContext& context,
        StageAddActorPanel& stageAddActorPanel,
        StageSelectionController& selectionController,
        UGCEditorInteractionController& interactionController,
        UGCSwitchConnectionController& connectionController,
        UGCEditorToolState& toolState,
        UGCSwitchConnectionState& connectionState);

    void DrawBackgroundGuides();
    void DrawSelectionOverlays();

private:
    void DrawSwitchConnectionLines();
    void DrawUnconnectedSwitchWarnings();
    void DrawPlacementPreview();
    void DrawLayerControls();
    void DrawTransformControls();
    void DrawGridOverlay();
    void DrawStackBadges();

    DebugEditorContext& mContext;
    StageAddActorPanel& mStageAddActorPanel;
    StageSelectionController& mSelectionController;
    UGCEditorInteractionController& mInteractionController;
    UGCSwitchConnectionController& mConnectionController;
    UGCEditorToolState& mToolState;
    UGCSwitchConnectionState& mConnectionState;
};
