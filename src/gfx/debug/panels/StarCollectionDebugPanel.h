#pragma once

#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/panels/ActorParameterYamlWriter.h"

class DebugBuildRestartPanel;

// ステージで最初に見つかったStarの獲得演出を調整・再生・保存する。contextとbuildRestartPanelは所有しない。
class StarCollectionDebugPanel {
public:
    StarCollectionDebugPanel(DebugEditorContext& context, DebugBuildRestartPanel& buildRestartPanel);

    void Draw();

private:
    DebugEditorContext& mContext;
    DebugBuildRestartPanel& mBuildRestartPanel;
    ActorParameterYamlWriter mYamlWriter;
};
