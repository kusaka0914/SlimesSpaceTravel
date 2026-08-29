#pragma once

#include "gfx/debug/DebugEditorContext.h"

#include <functional>

class EditorModelThumbnailRenderer;
class StageAddActorPanel;
class UGCEditorInteractionController;
class UGCEditorMenuState;
class UGCEditorToolState;
class UGCPreviewRenderer;
class UGCSceneOverlayRenderer;
class UGCWorkPanel;

class UGCDebugOverlayRenderer {
public:
    UGCDebugOverlayRenderer(
        DebugEditorContext& context,
        StageAddActorPanel& stageAddActorPanel,
        UGCEditorInteractionController& interactionController,
        UGCPreviewRenderer& previewRenderer,
        UGCSceneOverlayRenderer& sceneOverlayRenderer,
        UGCEditorToolState& toolState,
        UGCEditorMenuState& menuState,
        UGCWorkPanel& workPanel,
        EditorModelThumbnailRenderer* modelThumbnailRenderer,
        std::function<bool()> isAdjustingUGCUI);

    void Draw();

private:
    DebugEditorContext& mContext;
    StageAddActorPanel& mStageAddActorPanel;
    UGCEditorInteractionController& mInteractionController;
    UGCPreviewRenderer& mPreviewRenderer;
    UGCSceneOverlayRenderer& mSceneOverlayRenderer;
    UGCEditorToolState& mToolState;
    UGCEditorMenuState& mMenuState;
    UGCWorkPanel& mWorkPanel;
    EditorModelThumbnailRenderer* mModelThumbnailRenderer = nullptr;
    std::function<bool()> mIsAdjustingUGCUI;
};
