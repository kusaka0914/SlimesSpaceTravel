#pragma once

#include "gfx/debug/stage/UGCPresetVisuals.h"

#include <string>

enum class UGCEditorTutorialStep {
    Welcome,
    AdjustView,
    RaiseLayer,
    LowerLayer,
    PlacePlatform,
    PlaceLargePlatform,
    MovePlatform,
    RaiseSelectedPlatform,
    LowerSelectedPlatform,
    UndoEdit,
    EraseObject,
    PlaceEnemy,
    PlaceGoal,
    StartPlaytest,
    ReturnFromPlaytest,
    Complete,
};

class UGCEditorTutorial {
public:
    void Start();
    void RecordViewAdjustment();
    void RecordLayerChange(int layerDelta, bool hasSelection);
    void RecordPlacement(UGCPresetKind presetKind, int footprintSideLength);
    void RecordSelectionMove();
    void RecordUndo(bool wasRestored);
    void RecordErase(bool wasErased);
    void RecordPlaytestStarted();
    void RecordReturnedFromPlaytest();
    void AdvanceFromWelcome();

    bool IsActive() const { return mIsActive; }
    void Stop() { mIsActive = false; }
    UGCEditorTutorialStep GetStep() const { return mStep; }
    std::string GetInstruction() const;
    std::string GetProgressText() const;
    int GetCurrentActionNumber() const;
    int GetActionCount() const;
    float GetProgressRatio() const;
    bool ShouldHighlightPreset(UGCPresetKind presetKind) const;
    bool ShouldHighlightZoom() const;
    bool ShouldHighlightLayerUp() const;
    bool ShouldHighlightLayerDown() const;
    bool ShouldHighlightSelection() const;
    bool ShouldHighlightFootprintOptions() const;
    bool ShouldHighlightUndo() const;
    bool ShouldHighlightEraser() const;
    bool ShouldHighlightPlaytest() const;

private:
    void AdvanceTo(UGCEditorTutorialStep nextStep);

    UGCEditorTutorialStep mStep = UGCEditorTutorialStep::Welcome;
    bool mIsActive = false;
};
