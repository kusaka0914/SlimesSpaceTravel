#pragma once

#include "editor/core/DebugEditorContext.h"
#include "editor/core/DebugEditorSection.h"

// 編集画面の領域寸法を制約内に収め、リサイズ操作を共有contextへ反映する。contextは非所有参照。
class DebugEditorLayoutController {
public:
    explicit DebugEditorLayoutController(DebugEditorContext& context);

    // 画面サイズと選択タブに合わせて共有layoutを初期化・補正する。
    void Resolve(DebugEditorSection section);

    // リサイズ操作を共有layoutへ反映し、Resolveで寸法を補正する。描画前にResolveを呼んでおく。
    void DrawResizeHandles(DebugEditorSection section);
    const char* ResolveToolPanelTitle(DebugEditorSection section) const;

private:
    DebugEditorContext& mContext;
};
