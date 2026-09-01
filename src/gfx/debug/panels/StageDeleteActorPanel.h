#pragma once

#include "gfx/debug/DebugPanel.h"
#include "gfx/debug/stage/StageEditCommandController.h"

#include <string>
#include <unordered_set>

class StageDeleteActorPanel : public DebugPanel {
public:
    StageDeleteActorPanel(DebugEditorContext& context, StageEditCommandController& editCommandController);

    void Draw() override;

private:
    void DrawPlanetDeletion();

private:
    StageEditCommandController& mEditCommandController;
    std::unordered_set<std::string> mSelectedKeys;
    int mSelectedPlanetIndex = -1;
    int mPendingPlanetIndex = -1;
};
