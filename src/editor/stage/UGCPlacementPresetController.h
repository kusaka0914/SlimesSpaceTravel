#pragma once

#include "editor/stage/UGCPresetVisuals.h"

class StageActorCreateService;
class StageActorPlacementController;
class StageSelectionController;
class UGCPlatformCellService;
struct DebugEditorContext;

// UGCのプリセットを生成サービスと配置操作に結び付ける。クリックを待つ配置手順もここで開始する。
class UGCPlacementPresetController {
public:
    UGCPlacementPresetController(DebugEditorContext& context, StageActorCreateService& actorCreateService,
                                 UGCPlatformCellService& platformCellService,
                                 StageActorPlacementController& placementController);

    void SetSelectionController(StageSelectionController* selectionController);
    void SetPlatformFootprintSideLength(int sideLength) { mPlatformFootprintSideLength = sideLength; }

    // プリセットに応じた配置操作を開始する。準備に失敗した場合や未対応の種類には false。
    bool ActivatePreset(UGCPresetKind presetKind);

private:
    DebugEditorContext& mContext;
    StageActorCreateService& mCreateService;
    UGCPlatformCellService& mPlatformCellService;
    StageActorPlacementController& mPlacementController;
    StageSelectionController* mSelectionController = nullptr;
    int mPlatformFootprintSideLength = 1;
};
