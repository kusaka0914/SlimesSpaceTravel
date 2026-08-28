#pragma once

#include "gfx/debug/DebugEditorContext.h"

class DebugBuildRestartPanel;

class StarCollectionDebugPanel {
public:
    StarCollectionDebugPanel(
        DebugEditorContext& context,
        DebugBuildRestartPanel& buildRestartPanel);

    void Draw();

private:
    DebugEditorContext& mContext;
    DebugBuildRestartPanel& mBuildRestartPanel;
};
