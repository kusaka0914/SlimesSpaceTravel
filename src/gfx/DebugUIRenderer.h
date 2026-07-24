#pragma once

#include "gfx/debug/DebugEditorContext.h"
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
#include "gfx/debug/panels/UIDebugPanel.h"
#include "gfx/debug/stage/StageEditCommandController.h"
#include "gfx/debug/stage/StageGizmoController.h"
#include "gfx/debug/stage/StageSelectionController.h"

class Game;
class UIRenderer;

class DebugUIRenderer {
public:
    DebugUIRenderer(Game* game, UIRenderer* uiRenderer);

    void Draw();

private:
    DebugEditorContext mContext;

    PerformanceDebugPanel mPerformancePanel;
    CameraDebugPanel mCameraPanel;
    UIDebugPanel mUIPanel;
    ParameterDebugPanel mParameterPanel;
    ParticleEffectDebugPanel mParticleEffectPanel;
    SequenceDebugPanel mSequencePanel;

    StageAddActorPanel mStageAddActorPanel;
    StagePlanetPanel mStagePlanetPanel;

    StageSelectionController mSelectionController;
    StagePlacementPanel mStagePlacementPanel;
    StageEditCommandController mEditCommandController;
    StageDeleteActorPanel mStageDeleteActorPanel;
    StageEditorPanel mStageEditorPanel;
    StageGizmoController mGizmoController;
};
