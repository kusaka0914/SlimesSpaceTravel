#pragma once

#include "editor/core/DebugPanel.h"
#include "editor/stage/StageEditCommandController.h"

#include <string>
#include <unordered_set>

// Actorと惑星の削除対象・確認UIを管理し、削除をStageEditCommandControllerへ委譲する。参照先は所有しない。
class StageDeleteActorPanel : public DebugPanel {
public:
    StageDeleteActorPanel(DebugEditorContext& context, StageEditCommandController& editCommandController);

    void Draw() override;

private:
    void DrawPlanetDeletion();

    StageEditCommandController& mEditCommandController;
    std::unordered_set<std::string> mSelectedKeys;
    int mSelectedPlanetIndex = -1;
    int mPendingPlanetIndex = -1;
};
