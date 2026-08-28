#pragma once

#include "gfx/debug/DebugEditorContext.h"

class EditorModelThumbnailRenderer;
class StageAddActorPanel;
class UGCEditorInteractionController;
class UGCEditorMenuState;
class UGCEditorToolState;
class UGCEditorTutorial;
class UGCTutorialOverlayRenderer;
class UGCWorkFlowController;

class UGCEditorChromeRenderer {
public:
    UGCEditorChromeRenderer(
        DebugEditorContext& context,
        StageAddActorPanel& stageAddActorPanel,
        UGCEditorInteractionController& interactionController,
        UGCEditorTutorial& editorTutorial,
        UGCTutorialOverlayRenderer& tutorialOverlayRenderer,
        UGCEditorToolState& toolState,
        UGCEditorMenuState& menuState,
        UGCWorkFlowController& workFlowController,
        EditorModelThumbnailRenderer* modelThumbnailRenderer);

    bool DrawControls();

private:
    DebugEditorContext& mContext;
    StageAddActorPanel& mStageAddActorPanel;
    UGCEditorInteractionController& mInteractionController;
    UGCEditorTutorial& mEditorTutorial;
    UGCTutorialOverlayRenderer& mTutorialOverlayRenderer;
    UGCEditorToolState& mToolState;
    UGCEditorMenuState& mMenuState;
    UGCWorkFlowController& mWorkFlowController;
    EditorModelThumbnailRenderer* mModelThumbnailRenderer = nullptr;
};

