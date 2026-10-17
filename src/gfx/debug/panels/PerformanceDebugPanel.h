#pragma once

#include "gfx/debug/DebugPanel.h"

// Gameが集計したCPU・GPUの処理時間とImGuiのFPSを表示する。
class PerformanceDebugPanel : public DebugPanel {
public:
    explicit PerformanceDebugPanel(DebugEditorContext& context);

    void Draw() override;
};