#pragma once

#include "editor/assets/EditorModelThumbnailRenderer.h"
#include "editor/core/DebugEditorContext.h"
#include "editor/ugc/UGCDebugOverlayRenderer.h"
#include "editor/ugc/UGCEditorChromeRenderer.h"
#include "editor/ugc/UGCEditorElementRegistrar.h"
#include "editor/ugc/UGCEditorInteractionController.h"
#include "editor/ugc/UGCEditorMenuState.h"
#include "editor/ugc/UGCEditorToolState.h"
#include "editor/ugc/UGCEditorTutorial.h"
#include "editor/ugc/UGCEditorViewController.h"
#include "editor/ugc/UGCPreviewRenderer.h"
#include "editor/ugc/UGCSceneOverlayRenderer.h"
#include "editor/ugc/UGCSelectionDragState.h"
#include "editor/ugc/UGCStageEditSession.h"
#include "editor/ugc/UGCSwitchConnectionController.h"
#include "editor/ugc/UGCSwitchConnectionState.h"
#include "editor/ugc/UGCTutorialOverlayRenderer.h"
#include "editor/ugc/UGCWorkPanel.h"

#include <functional>
#include <memory>
#include <string>

class StageActorYamlWriter;
class StageAddActorPanel;
class StageEditCommandController;
class StageSelectionController;

// UGCの編集状態と描画モジュールを所有し、入力・描画・作品切り替えを統括する。
// 外部から渡されるcontextと編集用コントローラは所有しない。
class UGCEditorRenderer {
public:
    UGCEditorRenderer(DebugEditorContext& context, StageAddActorPanel& stageAddActorPanel,
                      StageActorYamlWriter& stageActorYamlWriter, StageSelectionController& selectionController,
                      StageEditCommandController& editCommandController, std::function<bool()> isAdjustingUGCUI);

    // ゲーム画面と編集UIを描画し、そのフレームの編集入力を処理する。画面サイズはピクセル単位。
    void Draw(unsigned int gameViewTexture, int gameViewWidth, int gameViewHeight);
    void DrawWorkBrowser();

    // 作業中のステージを完成チェック済みとして保存する。保存に失敗した場合はfalse。
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
    void ReloadSelectedWork();

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
    UGCEditorViewController mViewController;
    UGCStageEditSession mStageSession;
    UGCEditorInteractionController mInteractionController;
    UGCPreviewRenderer mPreviewRenderer;
    UGCTutorialOverlayRenderer mTutorialOverlayRenderer;
    UGCSceneOverlayRenderer mSceneOverlayRenderer;
    UGCWorkPanel mWorkPanel;
    std::unique_ptr<EditorModelThumbnailRenderer> mModelThumbnailRenderer;
    UGCDebugOverlayRenderer mDebugOverlayRenderer;
    UGCEditorChromeRenderer mChromeRenderer;
};
