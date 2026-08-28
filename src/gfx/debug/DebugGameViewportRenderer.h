#pragma once

#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/DebugEditorSection.h"

struct ImVec2;
class StageGizmoController;
class StageSelectionController;
class DebugBuildRestartPanel;

class DebugGameViewportRenderer {
public:
    DebugGameViewportRenderer(
        DebugEditorContext& context,
        StageSelectionController& selectionController,
        StageGizmoController& gizmoController,
        DebugBuildRestartPanel& buildRestartPanel);

    void Draw(
        DebugEditorSection section,
        unsigned int gameViewTexture,
        int gameViewWidth,
        int gameViewHeight);

private:
    void DrawToolbar(
        const ImVec2& toolbarMin,
        float toolbarWidth,
        bool showGizmoTranslationSpace);
    void AlignFreeCameraUpToSelectedActor();

    DebugEditorContext& mContext;
    StageSelectionController& mSelectionController;
    StageGizmoController& mGizmoController;
    DebugBuildRestartPanel& mBuildRestartPanel;
};
