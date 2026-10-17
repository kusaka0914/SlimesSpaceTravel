#pragma once

#include "gfx/debug/DebugEditorContext.h"

// 共有contextを借用するデバッグパネルの基底。各パネルはImGuiフレーム内で描画する。
class DebugPanel {
public:
    explicit DebugPanel(DebugEditorContext& context)
        : mContext(context)
    {
    }

    virtual ~DebugPanel() = default;

    virtual void Draw() = 0;

protected:
    DebugEditorContext& mContext;
};
