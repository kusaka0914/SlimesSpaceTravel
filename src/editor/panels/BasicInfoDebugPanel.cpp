#include "editor/panels/BasicInfoDebugPanel.h"

#include "editor/panels/PerformanceDebugPanel.h"
#include "imgui.h"

BasicInfoDebugPanel::BasicInfoDebugPanel(PerformanceDebugPanel& performancePanel)
    : mPerformancePanel(performancePanel)
{
}

void BasicInfoDebugPanel::Draw()
{
    ImGui::BeginChild("BasicInfoLeft", ImVec2(160.0f, 0.0f), true);
    ImGui::Selectable("パフォーマンス", true);
    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginChild("BasicInfoRight", ImVec2(0.0f, 0.0f), true);

    mPerformancePanel.Draw();

    ImGui::EndChild();
}
