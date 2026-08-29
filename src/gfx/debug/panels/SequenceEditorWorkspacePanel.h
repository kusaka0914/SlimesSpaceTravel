#pragma once

class CinematicSequenceDebugPanel;
class EndingRollDebugPanel;
class SequenceDebugPanel;
class StorybookDebugPanel;
class StarCollectionDebugPanel;

class SequenceEditorWorkspacePanel {
public:
    SequenceEditorWorkspacePanel(
        CinematicSequenceDebugPanel& cinematicCameraPanel,
        SequenceDebugPanel& sequencePanel,
        EndingRollDebugPanel& endingRollPanel,
        StorybookDebugPanel& storybookPanel,
        StarCollectionDebugPanel& starCollectionPanel);

    void Draw();
    int GetSelectedMenuIndex() const;
    void SetSelectedMenuIndex(int menuIndex);

private:
    CinematicSequenceDebugPanel& mCinematicCameraPanel;
    SequenceDebugPanel& mSequencePanel;
    EndingRollDebugPanel& mEndingRollPanel;
    StorybookDebugPanel& mStorybookPanel;
    StarCollectionDebugPanel& mStarCollectionPanel;
    int mSelectedSequenceEditorMenu = 0;
};
