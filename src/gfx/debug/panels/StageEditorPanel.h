#pragma once

#include "gfx/debug/DebugPanel.h"

#include <functional>
#include <string>

class StageAddActorPanel;
class StagePlanetPanel;
class StagePlacementPanel;
class StageDeleteActorPanel;
class StageActorYamlWriter;
class StageSelectionController;

class StageEditorPanel : public DebugPanel {
public:
    StageEditorPanel(
        DebugEditorContext& context,
        StageAddActorPanel& addActorPanel,
        StagePlanetPanel& planetPanel,
        StagePlacementPanel& placementPanel,
        StageDeleteActorPanel& deleteActorPanel,
        StageActorYamlWriter& stageActorYamlWriter,
        StageSelectionController& selectionController,
        std::function<bool()> restoreUndo,
        std::function<bool()> restoreRedo);

    void Draw() override;
    void DrawTopBar();

    void RequestOpenPlacementTab();
    bool ConsumeRequestOpenMainTab();
    int GetSelectedMenu() const { return mSelectedMenu; }
    void SetSelectedMenu(int selectedMenu);

private:
    void DrawDebugSceneSwitcher();
    void DrawStageSwitcher();
    void DrawStageClearProgressEditor();
    void DrawPlayerPlanetDebugMover();
    void DrawWorkspaceWindows();
    void DrawInspector();
    void DrawToolbar();
    void DrawDuplicatePlacementControls();

    StageAddActorPanel& mAddActorPanel;
    StagePlanetPanel& mPlanetPanel;
    StagePlacementPanel& mPlacementPanel;
    StageDeleteActorPanel& mDeleteActorPanel;
    StageActorYamlWriter& mStageActorYamlWriter;
    StageSelectionController& mSelectionController;
    std::function<bool()> mRestoreUndo;
    std::function<bool()> mRestoreRedo;

    int mSelectedMenu = 3;
    bool mRequestOpenMainTab = false;
    std::string mSelectedStageYamlPath;
    std::string mStageSwitchStatus;
    std::string mDuplicatePlacementStatus;
    std::string mEditHistoryStatus;
};
