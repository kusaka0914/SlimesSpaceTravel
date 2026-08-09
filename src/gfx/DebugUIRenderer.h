#pragma once

#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/assets/EditorAssetCatalog.h"
#include "gfx/debug/panels/AssetBrowserPanel.h"
#include "gfx/debug/panels/CameraDebugPanel.h"
#include "gfx/debug/panels/ParameterDebugPanel.h"
#include "gfx/debug/panels/ParticleEffectDebugPanel.h"
#include "gfx/debug/panels/PerformanceDebugPanel.h"
#include "gfx/debug/panels/SequenceDebugPanel.h"
#include "gfx/debug/panels/StageAddActorPanel.h"
#include "gfx/debug/panels/StageDeleteActorPanel.h"
#include "gfx/debug/panels/StageEditorPanel.h"
#include "gfx/debug/panels/StagePlacementPanel.h"
#include "gfx/debug/panels/StagePlanetPanel.h"
#include "gfx/debug/panels/TutorialDebugPanel.h"
#include "gfx/debug/panels/UIDebugPanel.h"
#include "gfx/debug/stage/StageEditCommandController.h"
#include "gfx/debug/stage/StageGizmoController.h"
#include "gfx/debug/stage/StageSelectionController.h"

class Game;
class UIRenderer;

class DebugUIRenderer {
public:
    DebugUIRenderer(Game* game, UIRenderer* uiRenderer);

    void Draw(
        unsigned int gameViewTexture,
        int gameViewWidth,
        int gameViewHeight);

private:
    enum class EditorSection {
        BasicInfo,
        Parameters,
        Particles,
        Sequences,
        Tutorials,
        Stage,
        UserInterface,
    };

    void DrawBasicInfoTab();
    void DrawSequenceEditorTab();
    void DrawDockedToolPanel(EditorSection section);
    void DrawDockedAssetBrowser(EditorSection section);
    void DrawGameViewport(
        EditorSection section,
        unsigned int gameViewTexture,
        int gameViewWidth,
        int gameViewHeight);
    void DrawGameViewportToolbar(
        const ImVec2& toolbarMin,
        float toolbarWidth);
    void ResolveResizableLayout(EditorSection section);
    void DrawLayoutResizeHandles(EditorSection section);
    const char* ResolveToolPanelTitle(EditorSection section) const;

private:
    EditorAssetCatalog mAssetCatalog;
    DebugEditorContext mContext;

    PerformanceDebugPanel mPerformancePanel;
    CameraDebugPanel mCameraPanel;
    UIDebugPanel mUIPanel;
    ParameterDebugPanel mParameterPanel;
    ParticleEffectDebugPanel mParticleEffectPanel;
    SequenceDebugPanel mSequencePanel;
    TutorialDebugPanel mTutorialPanel;
    AssetBrowserPanel mAssetBrowserPanel;

    StageAddActorPanel mStageAddActorPanel;
    StagePlanetPanel mStagePlanetPanel;

    StageSelectionController mSelectionController;
    StagePlacementPanel mStagePlacementPanel;
    StageEditCommandController mEditCommandController;
    StageDeleteActorPanel mStageDeleteActorPanel;
    StageEditorPanel mStageEditorPanel;
    StageGizmoController mGizmoController;

    EditorSection mActiveSection = EditorSection::BasicInfo;
    int mSelectedSequenceEditorMenu = 0;
};
