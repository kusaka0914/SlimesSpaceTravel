#pragma once

#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/DebugEditorSection.h"

struct ImVec2;
class StageGizmoController;
class StageSelectionController;
class DebugBuildRestartPanel;

// ゲーム画面を縦横比を保って表示し、カメラ・移動軸のツールバーを描画する。
// contextと渡されたController・Panelは非所有参照。
class DebugGameViewportRenderer {
public:
    DebugGameViewportRenderer(DebugEditorContext& context, StageSelectionController& selectionController,
                              StageGizmoController& gizmoController, DebugBuildRestartPanel& buildRestartPanel);

    // 借用テクスチャを表示し、context.gameViewportを更新する。元画像の寸法が非正またはハンドル0なら無効化する。
    void Draw(DebugEditorSection section, unsigned int gameViewTexture, int gameViewWidth, int gameViewHeight);

private:
    void DrawToolbar(const ImVec2& toolbarMin, float toolbarWidth, bool shouldShowGizmoTranslationSpace);
    void AlignFreeCameraUpToSelectedActor();

    DebugEditorContext& mContext;
    StageSelectionController& mSelectionController;
    StageGizmoController& mGizmoController;
    DebugBuildRestartPanel& mBuildRestartPanel;
};
