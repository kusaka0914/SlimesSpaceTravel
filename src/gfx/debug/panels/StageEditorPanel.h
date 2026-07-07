#pragma once

#include "gfx/debug/DebugPanel.h"

class StageAddActorPanel;
class StagePlanetPanel;
class StagePlacementPanel;
class StageDeleteActorPanel;

class StageEditorPanel : public DebugPanel {
public:
    StageEditorPanel(DebugEditorContext& context, StageAddActorPanel& addActorPanel, StagePlanetPanel& planetPanel,
                     StagePlacementPanel& placementPanel, StageDeleteActorPanel& deleteActorPanel);

    void Draw() override;

    void RequestOpenPlacementTab();
    bool ConsumeRequestOpenMainTab();

private:
    StageAddActorPanel& mAddActorPanel;
    StagePlanetPanel& mPlanetPanel;
    StagePlacementPanel& mPlacementPanel;
    StageDeleteActorPanel& mDeleteActorPanel;

    int mSelectedMenu = 0;
    bool mRequestOpenMainTab = false;
};