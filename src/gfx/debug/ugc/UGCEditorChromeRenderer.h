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
struct ImVec2;

// UGCエディタのツールバー・メニュー・保存通知を描画する。操作先の各モジュールは所有しない。
class UGCEditorChromeRenderer {
public:
    UGCEditorChromeRenderer(DebugEditorContext& context, StageAddActorPanel& stageAddActorPanel,
                            UGCEditorInteractionController& interactionController, UGCEditorTutorial& editorTutorial,
                            UGCTutorialOverlayRenderer& tutorialOverlayRenderer, UGCEditorToolState& toolState,
                            UGCEditorMenuState& menuState, UGCWorkPanel& workPanel,
                            EditorModelThumbnailRenderer* modelThumbnailRenderer);

    // 操作UIを描画・処理する。プレイテストへ移行した場合はtrueを返し、呼び出し元の編集描画を終了させる。
    bool DrawControls();

private:
    bool DrawActionIcon(const char* id, const char* texturePath, const char* tooltip, const ImVec2& size,
                        bool isSelected = false);
    void DrawProductMenu(const ImVec2& gameViewportMin, const ImVec2& menuPosition, bool shouldToggleMenu,
                         bool wasMenuOpen);
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
