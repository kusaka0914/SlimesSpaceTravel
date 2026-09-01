#pragma once

#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/DebugEditorSection.h"

class DebugEditorLayoutController {
public:
    explicit DebugEditorLayoutController(DebugEditorContext& context);

    void Resolve(DebugEditorSection section);
    void DrawResizeHandles(DebugEditorSection section);
    const char* ResolveToolPanelTitle(DebugEditorSection section) const;

private:
    DebugEditorContext& mContext;
};

