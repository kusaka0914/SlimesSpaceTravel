#pragma once

#include "gfx/debug/DebugPanel.h"

class PerformanceDebugPanel : public DebugPanel {
public:
    explicit PerformanceDebugPanel(DebugEditorContext& context);

    void Draw() override;
};