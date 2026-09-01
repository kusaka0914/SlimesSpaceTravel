#pragma once

#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/panels/ActorParameterYamlWriter.h"

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
    ActorParameterYamlWriter mYamlWriter;
};
