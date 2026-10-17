#pragma once

#include "gfx/debug/DebugEditorContext.h"

class StageEditCommandController;
class StageSelectionController;

// テキスト入力中の誤操作を避けながら、削除・複製・Undo/Redoのショートカットを処理する。
class StageEditShortcutHandler {
public:
    StageEditShortcutHandler(DebugEditorContext& context, StageSelectionController& selectionController,
                             StageEditCommandController& editCommandController);

    void Update();

private:
    void HandleDeleteShortcut();
    void HandleUndoRedoShortcut();
    void HandleDuplicateShortcut();

    DebugEditorContext& mContext;
    StageSelectionController& mSelectionController;
    StageEditCommandController& mEditCommandController;

    bool mWasZPressed = false;
    bool mWasYPressed = false;
    bool mWasDPressed = false;
};
