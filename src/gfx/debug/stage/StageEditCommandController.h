#pragma once

#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/stage/StageEditHistory.h"
#include "gfx/debug/stage/StageSelectionController.h"

#include <glm/glm.hpp>
#include <string>
#include <unordered_set>
#include <vector>
#include <yaml-cpp/yaml.h>

class StageEditCommandController {
public:
    StageEditCommandController(DebugEditorContext& context, StageSelectionController& selectionController);

    void UpdateShortcuts();

    void PushUndo();
    bool RestoreUndo();
    bool RestoreRedo();

    bool DeleteSelectedKeys(const std::unordered_set<std::string>& selectedKeys);
    bool DeletePlanet(int planetIndex);
    bool DeletePlanetOnly(int planetIndex);
    bool DuplicateSelectedKeys(const std::unordered_set<std::string>& selectedKeys);

    bool ConsumeRequestOpenPlacement();

private:
    void HandleDeleteShortcut();
    void HandleUndoRedoShortcut();
    void HandleDuplicateShortcut();

    void OffsetDuplicatedActorNode(YAML::Node actorNode, const glm::vec3& offset) const;

private:
    DebugEditorContext& mContext;
    StageSelectionController& mSelectionController;

    StageEditHistory mEditHistory;

    bool mZPressedPrev = false;
    bool mYPressedPrev = false;
    bool mDPressedPrev = false;

    bool mRequestOpenPlacement = false;
};
