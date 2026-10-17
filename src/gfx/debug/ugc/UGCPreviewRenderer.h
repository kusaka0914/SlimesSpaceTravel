#pragma once

#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/ugc/UGCEditorToolState.h"
#include "gfx/debug/ugc/UGCPreviewPanelState.h"

#include <functional>

struct ImDrawList;
struct ImVec2;

// ゲーム画面とリサイズ可能なUGCプレビューを描画する。contextと編集ツールの状態は所有しない。
class UGCPreviewRenderer {
public:
    UGCPreviewRenderer(DebugEditorContext& context, UGCEditorToolState& toolState,
                       std::function<bool()> isAdjustingUGCUI);

    // テクスチャを指定領域へ表示し、編集用ビューポートを更新する。サイズはピクセル単位。
    void DrawGameViewport(unsigned int gameViewTexture, int gameViewWidth, int gameViewHeight,
                          const ImVec2& viewportMin, const ImVec2& viewportMax);
    void DrawPreviewOverlay();

private:
    void DrawPreviewLayerGuides(const ImVec2& previewMin, const ImVec2& previewMax, ImDrawList* drawList);

    DebugEditorContext& mContext;
    UGCEditorToolState& mToolState;
    std::function<bool()> mIsAdjustingUGCUI;
    UGCPreviewPanelState mPanelState;
};
