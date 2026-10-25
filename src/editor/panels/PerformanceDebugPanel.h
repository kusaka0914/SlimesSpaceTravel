#pragma once

#include "editor/core/DebugPanel.h"

// Gameが集計したCPU・GPUの処理時間とImGuiのFPSを表示する。
class PerformanceDebugPanel : public DebugPanel {
public:
    explicit PerformanceDebugPanel(DebugEditorContext& context);

    void Draw() override;
};