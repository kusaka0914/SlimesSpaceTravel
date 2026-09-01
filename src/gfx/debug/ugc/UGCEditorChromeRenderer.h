#pragma once

#include "gfx/debug/DebugEditorContext.h"

#include <string>

class EditorModelThumbnailRenderer;
class StageAddActorPanel;
class UGCEditorInteractionController;
class UGCEditorMenuState;
class UGCEditorToolState;
class UGCEditorTutorial;
class UGCTutorialOverlayRenderer;
class UGCWorkPanel;
struct ImGuiViewport;

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
        UGCWorkPanel& workPanel,
        EditorModelThumbnailRenderer* modelThumbnailRenderer);

    bool DrawControls();

private:
    void DrawSaveShortcut(const ImGuiViewport& viewport);

    DebugEditorContext& mContext;
    StageAddActorPanel& mStageAddActorPanel;
    UGCEditorInteractionController& mInteractionController;
    UGCEditorTutorial& mEditorTutorial;
    UGCTutorialOverlayRenderer& mTutorialOverlayRenderer;
    UGCEditorToolState& mToolState;
    UGCEditorMenuState& mMenuState;
    UGCWorkPanel& mWorkPanel;
    EditorModelThumbnailRenderer* mModelThumbnailRenderer = nullptr;
    std::string mSaveFeedbackMessage;
    float mSaveFeedbackRemainingSeconds = 0.0f;
    bool mWasLastSaveSuccessful = false;
};
