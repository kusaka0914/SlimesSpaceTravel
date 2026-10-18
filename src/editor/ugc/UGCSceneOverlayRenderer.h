#pragma once

#include "editor/core/DebugEditorContext.h"
#include "editor/ugc/UGCEditorToolState.h"
#include "editor/ugc/UGCSwitchConnectionState.h"

class StageAddActorPanel;
class StageSelectionController;
class UGCEditorInteractionController;
class UGCSwitchConnectionController;

// 編集対象に重ねるグリッド・配置ガイド・接続線・操作UIを描画する。参照先は所有しない。
class UGCSceneOverlayRenderer {
public:
    UGCSceneOverlayRenderer(DebugEditorContext& context, StageAddActorPanel& stageAddActorPanel,
                            StageSelectionController& selectionController,
                            UGCEditorInteractionController& interactionController,
                            UGCSwitchConnectionController& connectionController, UGCEditorToolState& toolState,
                            UGCSwitchConnectionState& connectionState);

    void DrawBackgroundGuides();

    // 接続線・警告・選択対象の操作UIを描画する。接続操作の確定時にはステージを再読み込みする場合がある。
    void DrawSelectionOverlays();

private:
    void DrawSwitchConnectionLines();
    void DrawUnconnectedSwitchWarnings();
    void DrawPlacementPreview();
    void DrawTransformControls();
    void DrawGridOverlay();
    void DrawStackBadges();

    DebugEditorContext& mContext;
    StageAddActorPanel& mStageAddActorPanel;
    StageSelectionController& mSelectionController;
    UGCEditorInteractionController& mInteractionController;
    UGCSwitchConnectionController& mConnectionController;
    UGCEditorToolState& mToolState;
    UGCSwitchConnectionState& mConnectionState;
};
