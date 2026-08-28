#pragma once

class CameraDebugPanel;
class EndingRollDebugPanel;
class SequenceDebugPanel;
class StorybookDebugPanel;
class StarCollectionDebugPanel;

class SequenceEditorWorkspacePanel {
public:
    SequenceEditorWorkspacePanel(
        CameraDebugPanel& cameraPanel,
        SequenceDebugPanel& sequencePanel,
        EndingRollDebugPanel& endingRollPanel,
        StorybookDebugPanel& storybookPanel,
        StarCollectionDebugPanel& starCollectionPanel);

    void Draw();
    int GetSelectedMenuIndex() const;
    void SetSelectedMenuIndex(int menuIndex);

private:
    CameraDebugPanel& mCameraPanel;
    SequenceDebugPanel& mSequencePanel;
    EndingRollDebugPanel& mEndingRollPanel;
    StorybookDebugPanel& mStorybookPanel;
    StarCollectionDebugPanel& mStarCollectionPanel;
    int mSelectedSequenceEditorMenu = 0;
};
