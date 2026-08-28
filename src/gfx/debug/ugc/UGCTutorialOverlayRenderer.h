#pragma once

#include "gfx/debug/DebugEditorContext.h"

class UGCEditorTutorial;

class UGCTutorialOverlayRenderer {
public:
    UGCTutorialOverlayRenderer(
        DebugEditorContext& context,
        UGCEditorTutorial& tutorial);

    void Draw();
    void DrawHighlightForLastItem(bool shouldHighlight) const;

private:
    DebugEditorContext& mContext;
    UGCEditorTutorial& mEditorTutorial;
};

