#include "gfx/debug/panels/PerformanceDebugPanel.h"

#include "Game.h"
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

    if (!mContext.game) {
        return;
    }

    const FramePerformanceMetrics& metrics =
        mContext.game->GetFramePerformanceMetrics();
    ImGui::SeparatorText("フレーム計測");
    ImGui::Text(
        "フレーム全体（待機を含む）: %.3f ms",
        metrics.totalMilliseconds);
    ImGui::Text("ゲーム更新（CPU）: %.3f ms", metrics.gameUpdateMilliseconds);

    if (metrics.renderedViewportCount >= 1) {
        ImGui::Text(
            "1画面目の描画（CPU）: %.3f ms",
            metrics.firstViewportRenderMilliseconds);
        if (metrics.hasFirstViewportGpuMeasurement) {
            ImGui::Text(
                "1画面目の描画（GPU）: %.3f ms",
                metrics.firstViewportGpuMilliseconds);
        } else {
            ImGui::TextDisabled("1画面目の描画（GPU）: 測定中");
        }
    }
    if (metrics.renderedViewportCount >= 2) {
        ImGui::Text(
            "2画面目の描画（CPU）: %.3f ms",
            metrics.secondViewportRenderMilliseconds);
        if (metrics.hasSecondViewportGpuMeasurement) {
            ImGui::Text(
                "2画面目の描画（GPU）: %.3f ms",
                metrics.secondViewportGpuMilliseconds);
        } else {
            ImGui::TextDisabled("2画面目の描画（GPU）: 測定中");
        }
    }

    ImGui::Text(
        "ゲームUI（CPU）: %.3f ms",
        metrics.gameUiCpuMilliseconds);
    if (metrics.hasGameUiGpuMeasurement) {
        ImGui::Text(
            "ゲームUI（GPU）: %.3f ms",
            metrics.gameUiGpuMilliseconds);
    } else {
        ImGui::TextDisabled("ゲームUI（GPU）: 測定中");
    }

    if (mContext.game->GetIsDebugEditorShowing()) {
        ImGui::Text(
            "エディタUI（CPU）: %.3f ms",
            metrics.editorUiCpuMilliseconds);
        if (metrics.hasEditorUiGpuMeasurement) {
            ImGui::Text(
                "エディタUI（GPU）: %.3f ms",
                metrics.editorUiGpuMilliseconds);
        } else {
            ImGui::TextDisabled("エディタUI（GPU）: 測定中");
        }
    }

    ImGui::Text(
        "画面提示待機: %.3f ms",
        metrics.presentationWaitMilliseconds);

    ImGui::TextDisabled(
        "GPU時間は完了済みの過去フレームから非同期に取得します。");
}
