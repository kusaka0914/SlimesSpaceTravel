#pragma once

#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/ugc/UGCEditorToolState.h"
#include "gfx/debug/ugc/UGCPreviewPanelState.h"

#include <functional>

struct ImDrawList;
struct ImVec2;

class UGCPreviewRenderer {
public:
    UGCPreviewRenderer(
        DebugEditorContext& context,
        UGCEditorToolState& toolState,
        std::function<bool()> isAdjustingUGCUI);

    void DrawGameViewport(
        unsigned int gameViewTexture,
        int gameViewWidth,
        int gameViewHeight,
        const ImVec2& viewportMin,
        const ImVec2& viewportMax);
    void DrawPreviewOverlay();

private:
    void DrawPreviewLayerGuides(
        const ImVec2& previewMin,
        const ImVec2& previewMax,
        ImDrawList* drawList);

    DebugEditorContext& mContext;
    UGCEditorToolState& mToolState;
    std::function<bool()> mIsAdjustingUGCUI;
    UGCPreviewPanelState mPanelState;
};

