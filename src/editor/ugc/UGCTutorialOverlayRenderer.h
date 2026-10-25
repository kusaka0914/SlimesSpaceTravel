#pragma once

#include "editor/core/DebugEditorContext.h"

class UGCEditorTutorial;

// チュートリアルの案内・再開UIと操作対象の強調を描画する。contextとチュートリアルは所有しない。
class UGCTutorialOverlayRenderer {
public:
    UGCTutorialOverlayRenderer(DebugEditorContext& context, UGCEditorTutorial& tutorial);

    void Draw();

    // 直前に描画したImGuiアイテムへ強調枠を重ねる。対象の描画直後に呼ぶ。
    void DrawHighlightForLastItem(bool shouldHighlight) const;

private:
    DebugEditorContext& mContext;
    UGCEditorTutorial& mEditorTutorial;
};
