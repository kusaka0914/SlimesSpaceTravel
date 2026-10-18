#pragma once

#include "editor/core/DebugEditorContext.h"
#include "editor/core/DebugEditorSection.h"
#include "editor/session/DebugEditorSessionCoordinator.h"
#include "editor/stage/StageEditShortcutHandler.h"

class AssetBrowserPanel;
class BasicInfoDebugPanel;
class DebugEditorLayoutController;
class DebugGameViewportRenderer;
class ParameterDebugPanel;
class ParticleEffectDebugPanel;
class SequenceEditorWorkspacePanel;
class StageAddActorPanel;
class StageEditCommandController;
class StageEditorPanel;
class StageGizmoController;
class StageSelectionController;
class TutorialDebugPanel;
class UIDebugPanel;

// ワークスペースが借用する共有状態と各機能。生成時にすべての参照を設定する。
struct DebugEditorWorkspaceDependencies {
    DebugEditorContext& context;
    DebugEditorLayoutController& layoutController;
    DebugGameViewportRenderer& gameViewportRenderer;
    BasicInfoDebugPanel& basicInfoPanel;
    UIDebugPanel& uiPanel;
    ParameterDebugPanel& parameterPanel;
    ParticleEffectDebugPanel& particleEffectPanel;
    SequenceEditorWorkspacePanel& sequenceEditorPanel;
    TutorialDebugPanel& tutorialPanel;
    AssetBrowserPanel& assetBrowserPanel;
    StageAddActorPanel& stageAddActorPanel;
    StageSelectionController& selectionController;
    StageEditCommandController& editCommandController;
    StageEditorPanel& stageEditorPanel;
    StageGizmoController& gizmoController;
};

// タブ・ゲームビュー・各編集パネルの描画と入力処理の順序を管理する。
// dependenciesの参照先は所有せず、このインスタンスより長く有効に保つ。
class DebugEditorWorkspaceRenderer {
public:
    explicit DebugEditorWorkspaceRenderer(const DebugEditorWorkspaceDependencies& dependencies);

    // 借用したゲーム画面テクスチャを表示し、アクティブな編集機能を更新する。寸法は元画像のピクセル数。
    void Draw(unsigned int gameViewTexture, int gameViewWidth, int gameViewHeight);

    DebugEditorShellSessionState CaptureShellState() const;

    // タブ・演出メニューを復元する。タブ番号は有効範囲へ補正し、次回描画で選択を反映する。
    void ApplyShellState(const DebugEditorShellSessionState& shellState);

    bool IsUserInterfaceSectionActive() const;

private:
    void DrawTopBar();
    void UpdateStageEditing();
    void DrawDockedToolPanel(DebugEditorSection section);
    void DrawDockedAssetBrowser(DebugEditorSection section);

    DebugEditorContext& mContext;
    DebugEditorLayoutController& mLayoutController;
    DebugGameViewportRenderer& mGameViewportRenderer;
    BasicInfoDebugPanel& mBasicInfoPanel;
    UIDebugPanel& mUIPanel;
    ParameterDebugPanel& mParameterPanel;
    ParticleEffectDebugPanel& mParticleEffectPanel;
    SequenceEditorWorkspacePanel& mSequenceEditorPanel;
    TutorialDebugPanel& mTutorialPanel;
    AssetBrowserPanel& mAssetBrowserPanel;
    StageAddActorPanel& mStageAddActorPanel;
    StageSelectionController& mSelectionController;
    StageEditCommandController& mEditCommandController;
    StageEditShortcutHandler mStageEditShortcutHandler;
    StageEditorPanel& mStageEditorPanel;
    StageGizmoController& mGizmoController;

    DebugEditorSection mActiveSection = DebugEditorSection::BasicInfo;
    bool mShouldSelectRestoredSection = false;
};
