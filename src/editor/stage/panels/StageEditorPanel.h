#pragma once

#include "editor/core/DebugPanel.h"

#include <string>

class StageAddActorPanel;
class StagePlanetPanel;
class StagePlacementPanel;
class StageDeleteActorPanel;
class StageActorYamlWriter;
class StageSelectionController;
class StageEditCommandController;

// ステージ編集のメニュー・一覧・インスペクターを連携し、保存・履歴・ステージ切り替えを提供する。
// 渡された各パネルとControllerは所有しない。
class StageEditorPanel : public DebugPanel {
public:
    StageEditorPanel(DebugEditorContext& context, StageAddActorPanel& addActorPanel, StagePlanetPanel& planetPanel,
                     StagePlacementPanel& placementPanel, StageDeleteActorPanel& deleteActorPanel,
                     StageActorYamlWriter& stageActorYamlWriter, StageSelectionController& selectionController,
                     StageEditCommandController& editCommandController);

    void Draw() override;
    void DrawTopBar();

    // 選択中の編集へ切り替え、親タブと対象Actorの一覧を開く要求を記録する。
    void RequestOpenPlacementTab();

    // 親タブを開く要求を返し、要求を解除する。
    bool ConsumeRequestOpenMainTab();
    int GetSelectedMenu() const { return mSelectedMenu; }

    // 0:デバッグ状態、1:追加、3:選択中、6:削除。その他は選択中へ戻す。
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
    StageEditCommandController& mEditCommandController;

    int mSelectedMenu = 3;
    bool mRequestOpenMainTab = false;
    std::string mSelectedStageYamlPath;
    std::string mStageSwitchStatus;
    std::string mSaveStatus;
    std::string mDuplicatePlacementStatus;
    std::string mEditHistoryStatus;
};
