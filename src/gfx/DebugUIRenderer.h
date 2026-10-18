#pragma once

#include "gfx/debug/DebugBuildRestartPanel.h"
#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/DebugEditorLayoutController.h"
#include "gfx/debug/DebugEditorSection.h"
#include "gfx/debug/DebugEditorWorkspaceRenderer.h"
#include "gfx/debug/DebugGameViewportRenderer.h"
#include "gfx/debug/assets/EditorAssetCatalog.h"
#include "gfx/debug/panels/AssetBrowserPanel.h"
#include "gfx/debug/panels/BasicInfoDebugPanel.h"
#include "gfx/debug/panels/CameraDebugPanel.h"
#include "gfx/debug/panels/CinematicSequenceDebugPanel.h"
#include "gfx/debug/panels/EndingRollDebugPanel.h"
#include "gfx/debug/panels/ParameterDebugPanel.h"
#include "gfx/debug/panels/ParticleEffectDebugPanel.h"
#include "gfx/debug/panels/PerformanceDebugPanel.h"
#include "gfx/debug/panels/SequenceDebugPanel.h"
#include "gfx/debug/panels/SequenceEditorWorkspacePanel.h"
#include "gfx/debug/panels/StageAddActorPanel.h"
#include "gfx/debug/panels/StageDeleteActorPanel.h"
#include "gfx/debug/panels/StageEditorPanel.h"
#include "gfx/debug/panels/StagePlacementPanel.h"
#include "gfx/debug/panels/StagePlanetPanel.h"
#include "gfx/debug/panels/StarCollectionDebugPanel.h"
#include "gfx/debug/panels/StorybookDebugPanel.h"
#include "gfx/debug/panels/TutorialDebugPanel.h"
#include "gfx/debug/panels/UIDebugPanel.h"
#include "gfx/debug/stage/StageActorYamlWriter.h"
#include "gfx/debug/stage/StageEditCommandController.h"
#include "gfx/debug/stage/StageGizmoController.h"
#include "gfx/debug/stage/StageSelectionController.h"

#include <memory>
#include <string>

class Game;
class DebugEditorSessionCoordinator;
class UGCEditorRenderer;
class UIRenderer;

// デバッグ画面のパネル・配置編集・セッション保存を束ね、通常エディターとUGC編集画面へ処理を振り分ける。
// Game・UIRendererは非所有参照。選択Actorはステージ再読込前にClearStageSelectionForStageReloadで解除する。
class DebugUIRenderer {
public:
    DebugUIRenderer(Game* game, UIRenderer* uiRenderer);
    ~DebugUIRenderer();

    // ゲーム画面テクスチャを編集画面へ表示する。寸法は元テクスチャのピクセル数で、テクスチャは所有しない。
    void Draw(unsigned int gameViewTexture, int gameViewWidth, int gameViewHeight);
    void DrawUGCEditor(unsigned int gameViewTexture, int gameViewWidth, int gameViewHeight);
    void DrawUGCWorkBrowser();
    bool CompleteUGCVerification(const std::string& workFileName);
    void HandleUGCUndo();
    void HandleUGCRedo();
    void HandleUGCEraserToggle();
    void HandleUGCSelectionMode();
    void OpenUGCEditorMenu();
    void HandleUGCZoom(float distanceMultiplier);
    void HandleUGCLayerChange(int layerDelta);
    void HandleUGCSelectionGridMove(int gridX, int gridZ);
    void HandleUGCEditorTutorialReturnedFromPlaytest();

    // ステージ内Actorを破棄する前に呼び、選択状態とその参照を解除する。
    void ClearStageSelectionForStageReload();

    // 再読込成功後に呼び、破棄済みRuntime向けの保存失敗を解除する。失敗時には呼ばない。
    void NotifyStageReloaded();

    // 編集セッションをファイルへ保存する。失敗時はfalseとoutErrorMessageで理由を返す。
    bool SaveEditorSession(const std::string& filePath, std::string& outErrorMessage);

    // ファイルから編集対象と画面状態を復元する。失敗時はfalseとoutErrorMessageで理由を返す。
    bool RestoreEditorSession(const std::string& filePath, std::string& outErrorMessage);
    void SetBuildRestartStatus(const std::string& message, bool isError);

private:
    EditorAssetCatalog mAssetCatalog;
    DebugEditorContext mContext;
    DebugEditorLayoutController mLayoutController;
    DebugBuildRestartPanel mBuildRestartPanel;

    PerformanceDebugPanel mPerformancePanel;
    BasicInfoDebugPanel mBasicInfoPanel;
    CameraDebugPanel mCameraPanel;
    CinematicSequenceDebugPanel mCinematicCameraPanel;
    UIDebugPanel mUIPanel;
    ParameterDebugPanel mParameterPanel;
    ParticleEffectDebugPanel mParticleEffectPanel;
    SequenceDebugPanel mSequencePanel;
    EndingRollDebugPanel mEndingRollPanel;
    StorybookDebugPanel mStorybookPanel;
    StarCollectionDebugPanel mStarCollectionPanel;
    SequenceEditorWorkspacePanel mSequenceEditorPanel;
    TutorialDebugPanel mTutorialPanel;
    AssetBrowserPanel mAssetBrowserPanel;

    StageAddActorPanel mStageAddActorPanel;
    StagePlanetPanel mStagePlanetPanel;
    StageActorYamlWriter mStageActorYamlWriter;

    StageSelectionController mSelectionController;
    StagePlacementPanel mStagePlacementPanel;
    StageEditCommandController mEditCommandController;
    StageDeleteActorPanel mStageDeleteActorPanel;
    StageEditorPanel mStageEditorPanel;
    StageGizmoController mGizmoController;
    DebugGameViewportRenderer mGameViewportRenderer;
    DebugEditorWorkspaceRenderer mWorkspaceRenderer;
    std::unique_ptr<DebugEditorSessionCoordinator> mSessionCoordinator;
    std::unique_ptr<UGCEditorRenderer> mUGCEditorRenderer;
};
