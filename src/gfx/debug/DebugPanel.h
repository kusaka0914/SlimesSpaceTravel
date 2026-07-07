#pragma once

#include "gfx/debug/DebugEditorContext.h"

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