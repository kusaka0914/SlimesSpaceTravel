#pragma once

#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/stage/StageEditorTypes.h"
#include "gfx/debug/ugc/UGCSwitchConnectionState.h"

#include <optional>

class Actor;
class StageAddActorPanel;
class StageSelectionController;
class UGCEditorToolState;

class UGCSwitchConnectionController {
public:
    UGCSwitchConnectionController(
        DebugEditorContext& context,
        StageAddActorPanel& stageAddActorPanel,
        StageSelectionController& selectionController,
        UGCEditorToolState& toolState,
        UGCSwitchConnectionState& connectionState);

    bool CompletePendingConnection(
        Actor* selectedActor,
        const std::optional<StageActorRef>& selectedRef);
    void BeginConnection(
        const StageActorRef& switchRef,
        UGCSwitchConnectionAction action);

private:
    DebugEditorContext& mContext;
    StageAddActorPanel& mStageAddActorPanel;
    StageSelectionController& mSelectionController;
    UGCEditorToolState& mToolState;
    UGCSwitchConnectionState& mConnectionState;
};

