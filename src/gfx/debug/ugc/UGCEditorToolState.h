#pragma once

#include "gfx/debug/stage/UGCPresetVisuals.h"

#include <optional>
#include <string>

class UGCEditorToolState {
public:
    void ActivatePreset(UGCPresetKind presetKind);
    void ActivateSelection();
    void EnterEraser(
        std::optional<UGCPresetKind> presetToRestore);
    void LeaveEraser(bool didRestorePreviousPreset);
    void DeactivateEraser();
    void ResetForLoadedWork();

    bool isEraserMode = false;
    std::optional<UGCPresetKind> activePresetKind;
    std::optional<UGCPresetKind> presetBeforeEraser;
    std::string statusMessage;
    int editLayer = 0;
    int platformFootprintSideLength = 1;
};
