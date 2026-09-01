#pragma once

#include "gfx/debug/DebugPanel.h"
#include "gfx/debug/stage/StageActorInspector.h"
#include "gfx/debug/stage/StageSelectionController.h"
#include "gfx/debug/stage/StagePlayerEditor.h"

#include <functional>
#include <string>
#include <vector>

class Actor;
class StageActorYamlWriter;

class StagePlacementPanel : public DebugPanel {
public:
    using Callback = std::function<void()>;

    StagePlacementPanel(
        DebugEditorContext& context,
        StageSelectionController& selectionController,
        StageActorYamlWriter& stageActorYamlWriter,
        Callback pushUndoCallback = {});

    void Draw() override;
    void DrawObjectList();
    void DrawPlayerSpawn();
    void DrawPlayerDebugMover(Actor* selectedActor);
    void RequestOpenPickedActorPlacement();

private:
    struct ActorGroup {
        std::string label;
        std::string sequenceName;
        std::vector<StageActorInstance> actors;
    };

    std::vector<ActorGroup> CollectActorGroups() const;

    void DrawActorList(const ActorGroup& group);

private:
    StageSelectionController& mSelectionController;
    StagePlayerEditor mStagePlayerEditor;
    StageActorInspector mStageActorInspector;
    bool mRequestOpenPickedActorPlacement = false;
};
