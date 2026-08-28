#pragma once

#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/assets/EditorModelThumbnailRenderer.h"
#include "gfx/debug/stage/StageEditorTypes.h"
#include "gfx/debug/ugc/UGCEditorTutorial.h"
#include "gfx/debug/ugc/UGCEditorElementRegistrar.h"
#include "gfx/debug/ugc/UGCEditorChromeRenderer.h"
#include "gfx/debug/ugc/UGCEditCommandController.h"
#include "gfx/debug/ugc/UGCEditLayerController.h"
#include "gfx/debug/ugc/UGCEditorMenuState.h"
#include "gfx/debug/ugc/UGCDebugOverlayRenderer.h"
#include "gfx/debug/ugc/UGCEditorInteractionController.h"
#include "gfx/debug/ugc/UGCEditorToolState.h"
#include "gfx/debug/ugc/UGCEditorViewController.h"
#include "gfx/debug/ugc/UGCPreviewRenderer.h"
#include "gfx/debug/ugc/UGCSceneOverlayRenderer.h"
#include "gfx/debug/ugc/UGCSceneInteractionController.h"
#include "gfx/debug/ugc/UGCSelectionDragController.h"
#include "gfx/debug/ugc/UGCSwitchConnectionController.h"
#include "gfx/debug/ugc/UGCSwitchConnectionState.h"
#include "gfx/debug/ugc/UGCTutorialOverlayRenderer.h"
#include "gfx/debug/ugc/UGCSelectionDragState.h"
#include "gfx/debug/ugc/UGCWorkFlowController.h"

#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class StageActorYamlWriter;
class StageAddActorPanel;
class StageEditCommandController;
class StageGizmoController;
class StagePlanetPanel;
class StageSelectionController;
struct ImDrawList;
struct ImVec2;

class UGCEditorRenderer {
public:
    UGCEditorRenderer(
        DebugEditorContext& context,
        StageAddActorPanel& stageAddActorPanel,
        StageActorYamlWriter& stageActorYamlWriter,
        StageSelectionController& selectionController,
        StageEditCommandController& editCommandController,
        std::function<bool()> isAdjustingUGCUI);

    void Draw(
        unsigned int gameViewTexture,
        int gameViewWidth,
        int gameViewHeight);
    void DrawWorkBrowser();
    bool CompleteVerification(const std::string& workFileName);
    void DrawDebugEditorOverlay();
    void RegisterUIEditorElements();

    void HandleUndo();
    void HandleRedo();
    void ToggleEraser();
    void ActivateSelectionMode();
    void OpenMenu();
    void AdjustZoom(float distanceMultiplier);
    void ChangeLayer(int layerDelta);
    void MoveSelectionOnGrid(int gridX, int gridZ);
    void HandleTutorialReturnedFromPlaytest();

private:
    bool IsWorkManagementOpen() const;
    void DrawWorkManagement();
    void StartVerification();

private:
    DebugEditorContext& mContext;
    StageAddActorPanel& mStageAddActorPanel;
    StageActorYamlWriter& mStageActorYamlWriter;
    StageSelectionController& mSelectionController;
    StageEditCommandController& mEditCommandController;
    std::function<bool()> mIsAdjustingUGCUI;

    UGCEditorToolState mToolState;
    UGCSelectionDragState mDragState;
    UGCEditorTutorial mEditorTutorial;
    UGCEditorMenuState mMenuState;
    UGCEditorElementRegistrar mElementRegistrar;
    UGCSwitchConnectionState mConnectionState;
    UGCSwitchConnectionController mSwitchConnectionController;
    UGCEditCommandController mUGCEditCommandController;
    UGCEditLayerController mEditLayerController;
    UGCSelectionDragController mSelectionDragController;
    UGCEditorViewController mViewController;
    UGCSceneInteractionController mSceneInteractionController;
    UGCEditorInteractionController mInteractionController;
    UGCPreviewRenderer mPreviewRenderer;
    UGCTutorialOverlayRenderer mTutorialOverlayRenderer;
    UGCSceneOverlayRenderer mSceneOverlayRenderer;
    UGCWorkFlowController mWorkFlowController;
    std::unique_ptr<EditorModelThumbnailRenderer> mModelThumbnailRenderer;
    UGCDebugOverlayRenderer mDebugOverlayRenderer;
    UGCEditorChromeRenderer mChromeRenderer;

};
