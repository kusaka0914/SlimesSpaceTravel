#pragma once

#include "gfx/debug/DebugEditorContext.h"

#include <glm/glm.hpp>

class StageActorYamlWriter;
class StageAddActorPanel;
class StageEditCommandController;
class StageSelectionController;
class UGCEditorToolState;
class UGCEditorTutorial;
class UGCSelectionDragState;

class UGCSelectionDragController {
public:
    UGCSelectionDragController(
        DebugEditorContext& context,
        StageAddActorPanel& stageAddActorPanel,
        StageActorYamlWriter& stageActorYamlWriter,
        StageSelectionController& selectionController,
        StageEditCommandController& editCommandController,
        UGCEditorTutorial& editorTutorial,
        UGCEditorToolState& toolState,
        UGCSelectionDragState& dragState);

    void Update();

private:
    bool TryIntersectDragPlane(
        const glm::vec3& rayFrom,
        const glm::vec3& rayTo,
        glm::vec3& outIntersection) const;

    DebugEditorContext& mContext;
    StageAddActorPanel& mStageAddActorPanel;
    StageActorYamlWriter& mStageActorYamlWriter;
    StageSelectionController& mSelectionController;
    StageEditCommandController& mEditCommandController;
    UGCEditorTutorial& mEditorTutorial;
    UGCEditorToolState& mToolState;
    UGCSelectionDragState& mDragState;
};
