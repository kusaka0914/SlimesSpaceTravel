#include "gfx/debug/ugc/UGCEditorToolState.h"

void UGCEditorToolState::ActivatePreset(UGCPresetKind presetKind)
{
    isEraserMode = false;
    presetBeforeEraser.reset();
    activePresetKind = presetKind;
}

void UGCEditorToolState::ActivateSelection()
{
    isEraserMode = false;
    presetBeforeEraser.reset();
    activePresetKind.reset();
}

void UGCEditorToolState::EnterEraser(
    std::optional<UGCPresetKind> presetToRestore)
{
    if (isEraserMode) {
        return;
    }
    isEraserMode = true;
    presetBeforeEraser = presetToRestore;
    activePresetKind.reset();
}

void UGCEditorToolState::LeaveEraser(bool didRestorePreviousPreset)
{
    isEraserMode = false;
    activePresetKind = didRestorePreviousPreset
        ? presetBeforeEraser
        : std::nullopt;
    presetBeforeEraser.reset();
}

void UGCEditorToolState::DeactivateEraser()
{
    isEraserMode = false;
    presetBeforeEraser.reset();
}

void UGCEditorToolState::ResetForLoadedWork()
{
    ActivateSelection();
    statusMessage.clear();
    editLayer = 0;
    platformFootprintSideLength = 1;
}
