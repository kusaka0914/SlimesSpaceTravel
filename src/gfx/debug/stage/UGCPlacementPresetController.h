#pragma once

#include <functional>

class StageActorCreateService;
class StageActorPlacementController;
class StageSelectionController;
class UGCPlatformCellService;
struct DebugEditorContext;

enum class UGCPresetKind {
    NormalPlatform,
    NormalEnemy,
    EllipsePlanet,
    PressureSwitch,
    GoalStar,
    MovingPlatform,
    FadingPlatform,
    AdhesivePlatform,
    TwoPlayerSwitch,
};

class UGCPlacementPresetController {
public:
    UGCPlacementPresetController(
        DebugEditorContext& context,
        StageActorCreateService& actorCreateService,
        UGCPlatformCellService& platformCellService,
        StageActorPlacementController& placementController);

    void SetSelectionController(
        StageSelectionController* selectionController);
    void SetPushUndoCallback(std::function<void()> pushUndoCallback);
    void SetPlatformFootprintSideLength(int sideLength)
    {
        mPlatformFootprintSideLength = sideLength;
    }
    bool ActivatePreset(UGCPresetKind presetKind);

private:
    DebugEditorContext& mContext;
    StageActorCreateService& mCreateService;
    UGCPlatformCellService& mPlatformCellService;
    StageActorPlacementController& mPlacementController;
    StageSelectionController* mSelectionController = nullptr;
    std::function<void()> mPushUndoCallback;
    int mPlatformFootprintSideLength = 1;
};
