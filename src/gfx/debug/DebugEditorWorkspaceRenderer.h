#pragma once

#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/DebugEditorSection.h"
#include "gfx/debug/session/DebugEditorSessionCoordinator.h"

class AssetBrowserPanel;
class BasicInfoDebugPanel;
class DebugBuildRestartPanel;
class DebugEditorLayoutController;
class DebugGameViewportRenderer;
class ParameterDebugPanel;
class ParticleEffectDebugPanel;
class PerformanceDebugPanel;
class SequenceEditorWorkspacePanel;
class StageAddActorPanel;
class StageEditCommandController;
class StageEditorPanel;
class StageGizmoController;
class StageSelectionController;
class TutorialDebugPanel;
class UIDebugPanel;

struct DebugEditorWorkspaceDependencies {
    DebugEditorContext& context;
    DebugEditorLayoutController& layoutController;
    DebugGameViewportRenderer& gameViewportRenderer;
    BasicInfoDebugPanel& basicInfoPanel;
    UIDebugPanel& uiPanel;
    ParameterDebugPanel& parameterPanel;
    ParticleEffectDebugPanel& particleEffectPanel;
    SequenceEditorWorkspacePanel& sequenceEditorPanel;
    TutorialDebugPanel& tutorialPanel;
    AssetBrowserPanel& assetBrowserPanel;
    StageAddActorPanel& stageAddActorPanel;
    StageSelectionController& selectionController;
    StageEditCommandController& editCommandController;
    StageEditorPanel& stageEditorPanel;
    StageGizmoController& gizmoController;
};

class DebugEditorWorkspaceRenderer {
public:
    explicit DebugEditorWorkspaceRenderer(
        const DebugEditorWorkspaceDependencies& dependencies);

    void Draw(
        unsigned int gameViewTexture,
        int gameViewWidth,
        int gameViewHeight);

    DebugEditorShellSessionState CaptureShellState() const;
    void ApplyShellState(const DebugEditorShellSessionState& shellState);
    bool IsUserInterfaceSectionActive() const;

private:
    void DrawDockedToolPanel(DebugEditorSection section);
    void DrawDockedAssetBrowser(DebugEditorSection section);

    DebugEditorContext& mContext;
    DebugEditorLayoutController& mLayoutController;
    DebugGameViewportRenderer& mGameViewportRenderer;
    BasicInfoDebugPanel& mBasicInfoPanel;
    UIDebugPanel& mUIPanel;
    ParameterDebugPanel& mParameterPanel;
    ParticleEffectDebugPanel& mParticleEffectPanel;
    SequenceEditorWorkspacePanel& mSequenceEditorPanel;
    TutorialDebugPanel& mTutorialPanel;
    AssetBrowserPanel& mAssetBrowserPanel;
    StageAddActorPanel& mStageAddActorPanel;
    StageSelectionController& mSelectionController;
    StageEditCommandController& mEditCommandController;
    StageEditorPanel& mStageEditorPanel;
    StageGizmoController& mGizmoController;

    DebugEditorSection mActiveSection = DebugEditorSection::BasicInfo;
    bool mShouldSelectRestoredSection = false;
};
