#pragma once

class CinematicSequenceDebugPanel;
class EndingRollDebugPanel;
class SequenceDebugPanel;
class StorybookDebugPanel;
class StarCollectionDebugPanel;

// 演出編集のメニューを描画し、対応するパネルへ委譲する。渡された各パネルは所有しない。
class SequenceEditorWorkspacePanel {
public:
    SequenceEditorWorkspacePanel(CinematicSequenceDebugPanel& cinematicCameraPanel, SequenceDebugPanel& sequencePanel,
                                 EndingRollDebugPanel& endingRollPanel, StorybookDebugPanel& storybookPanel,
                                 StarCollectionDebugPanel& starCollectionPanel);

    void Draw();
    int GetSelectedMenuIndex() const;

    // menuIndexを0〜4の範囲に制限する。
    void SetSelectedMenuIndex(int menuIndex);

private:
    CinematicSequenceDebugPanel& mCinematicCameraPanel;
    SequenceDebugPanel& mSequencePanel;
    EndingRollDebugPanel& mEndingRollPanel;
    StorybookDebugPanel& mStorybookPanel;
    StarCollectionDebugPanel& mStarCollectionPanel;
    int mSelectedSequenceEditorMenu = 0;
};
