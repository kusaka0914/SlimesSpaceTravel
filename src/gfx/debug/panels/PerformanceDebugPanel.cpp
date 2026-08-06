#include "gfx/debug/panels/PerformanceDebugPanel.h"

#include "imgui.h"

PerformanceDebugPanel::PerformanceDebugPanel(DebugEditorContext& context)
    : DebugPanel(context)
{
}

void PerformanceDebugPanel::Draw()
{
    const float fps = ImGui::GetIO().Framerate;

    ImGui::Text("FPS: %.1f", fps);

    if (fps > 0.0f) {
        ImGui::Text("フレームタイム: %.3f ms", 1000.0f / fps);
    }
}
