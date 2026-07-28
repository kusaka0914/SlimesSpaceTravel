#pragma once

#include "gfx/debug/DebugPanel.h"

#include <string>

class StageAddActorPanel;
class StagePlanetPanel;
class StagePlacementPanel;
class StageDeleteActorPanel;
class StageSelectionController;

class StageEditorPanel : public DebugPanel {
public:
    StageEditorPanel(DebugEditorContext& context, StageAddActorPanel& addActorPanel, StagePlanetPanel& planetPanel,
                     StagePlacementPanel& placementPanel, StageDeleteActorPanel& deleteActorPanel,
                     StageSelectionController& selectionController);

    void Draw() override;

    void RequestOpenPlacementTab();
    bool ConsumeRequestOpenMainTab();

private:
    void DrawStageSwitcher();
    void DrawStageClearProgressEditor();

    StageAddActorPanel& mAddActorPanel;
    StagePlanetPanel& mPlanetPanel;
    StagePlacementPanel& mPlacementPanel;
    StageDeleteActorPanel& mDeleteActorPanel;
    StageSelectionController& mSelectionController;

    int mSelectedMenu = 0;
    bool mRequestOpenMainTab = false;
    std::string mSelectedStageYamlPath;
    std::string mStageSwitchStatus;
};
