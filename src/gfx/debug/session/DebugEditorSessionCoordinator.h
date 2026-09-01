#pragma once

#include <string>

struct DebugEditorContext;
class StageEditorPanel;
class StagePlanetPanel;
class StageSelectionController;

struct DebugEditorShellSessionState {
    int activeSectionIndex = 0;
    int sequenceEditorMenuIndex = 0;
};

class DebugEditorSessionCoordinator {
public:
    DebugEditorSessionCoordinator(
        DebugEditorContext& context,
        StagePlanetPanel& stagePlanetPanel,
        StageEditorPanel& stageEditorPanel,
        StageSelectionController& selectionController);

    bool Save(
        const std::string& filePath,
        const DebugEditorShellSessionState& shellState,
        std::string& outErrorMessage);
    bool Restore(
        const std::string& filePath,
        DebugEditorShellSessionState& outShellState,
        std::string& outErrorMessage);

private:
    DebugEditorContext& mContext;
    StagePlanetPanel& mStagePlanetPanel;
    StageEditorPanel& mStageEditorPanel;
    StageSelectionController& mSelectionController;
};
