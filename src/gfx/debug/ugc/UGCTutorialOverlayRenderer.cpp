#include "gfx/debug/ugc/UGCTutorialOverlayRenderer.h"

#include "Game.h"
#include "gfx/debug/ugc/UGCEditorTutorial.h"
#include "imgui.h"

#include <algorithm>
#include <cmath>

namespace {

float CalculateTutorialUIScale(const ImGuiViewport& viewport)
{
    constexpr float referenceWidth = 2560.0f;
    constexpr float referenceHeight = 1440.0f;
    return std::max(
        1.0f,
        std::min(
            viewport.WorkSize.x / referenceWidth,
            viewport.WorkSize.y / referenceHeight));
}

}

UGCTutorialOverlayRenderer::UGCTutorialOverlayRenderer(
    DebugEditorContext& context,
    UGCEditorTutorial& tutorial)
    : mContext(context),
      mEditorTutorial(tutorial)
{
}

void UGCTutorialOverlayRenderer::DrawHighlightForLastItem(
    bool shouldHighlight) const
{
    if (!shouldHighlight) {
        return;
    }
    const float pulse =
        0.5f + 0.5f * std::sin(static_cast<float>(ImGui::GetTime()) * 5.0f);
    const int alpha = static_cast<int>(190.0f + pulse * 65.0f);
    ImGui::GetWindowDrawList()->AddRect(
        ImGui::GetItemRectMin(),
        ImGui::GetItemRectMax(),
        IM_COL32(255, 215, 45, alpha),
        10.0f,
        0,
        5.0f);
}

void UGCTutorialOverlayRenderer::Draw()
{
    if (!mContext.game) {
        return;
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float uiScale = CalculateTutorialUIScale(*viewport);
    constexpr ImGuiWindowFlags tutorialWindowFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings;
    if (!mEditorTutorial.IsActive()) {
        ImGui::SetNextWindowPos(
            ImVec2(
                viewport->WorkPos.x + viewport->WorkSize.x -
                    230.0f * uiScale,
                viewport->WorkPos.y + 16.0f * uiScale),
            ImGuiCond_Always);
        ImGui::PushStyleVar(
            ImGuiStyleVar_WindowPadding,
            ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(
            ImGuiStyleVar_FrameRounding,
            8.0f * uiScale);
        ImGui::PushStyleColor(
            ImGuiCol_Button,
            ImVec4(0.08f, 0.36f, 0.65f, 1.0f));
        ImGui::PushStyleColor(
            ImGuiCol_ButtonHovered,
            ImVec4(0.10f, 0.49f, 0.86f, 1.0f));
        ImGui::PushStyleColor(
            ImGuiCol_ButtonActive,
            ImVec4(0.06f, 0.29f, 0.54f, 1.0f));
        ImGui::Begin(
            "###UGCTutorialReplay",
            nullptr,
            tutorialWindowFlags |
                ImGuiWindowFlags_NoBackground |
                ImGuiWindowFlags_NoNavInputs);
        ImGui::SetWindowFontScale(uiScale);
        if (ImGui::Button(
                "操作練習",
                ImVec2(154.0f * uiScale, 46.0f * uiScale))) {
            mContext.game->StartUGCEditorTutorial();
        }
        ImGui::End();
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(3);
        return;
    }

    const UGCEditorTutorialStep step = mEditorTutorial.GetStep();
    const bool isWelcome = step == UGCEditorTutorialStep::Welcome;
    const bool isComplete = step == UGCEditorTutorialStep::Complete;
    const ImVec2 panelSize(
        std::min(
            680.0f * uiScale,
            viewport->WorkSize.x - 48.0f * uiScale),
        (isWelcome || isComplete ? 246.0f : 192.0f) * uiScale);
    ImGui::SetNextWindowPos(
        ImVec2(
            viewport->WorkPos.x + viewport->WorkSize.x * 0.5f,
            viewport->WorkPos.y + 112.0f * uiScale),
        ImGuiCond_Always,
        ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowSize(panelSize, ImGuiCond_Always);
    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ImVec2(24.0f * uiScale, 20.0f * uiScale));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f * uiScale);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f * uiScale);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f * uiScale);
    ImGui::PushStyleVar(
        ImGuiStyleVar_ItemSpacing,
        ImVec2(12.0f * uiScale, 12.0f * uiScale));
    ImGui::PushStyleColor(
        ImGuiCol_WindowBg,
        ImVec4(0.025f, 0.055f, 0.10f, 0.98f));
    ImGui::PushStyleColor(
        ImGuiCol_Border,
        ImVec4(0.25f, 0.66f, 1.0f, 0.90f));
    ImGui::PushStyleColor(
        ImGuiCol_Button,
        ImVec4(0.08f, 0.36f, 0.65f, 1.0f));
    ImGui::PushStyleColor(
        ImGuiCol_ButtonHovered,
        ImVec4(0.10f, 0.49f, 0.86f, 1.0f));
    ImGui::PushStyleColor(
        ImGuiCol_ButtonActive,
        ImVec4(0.06f, 0.29f, 0.54f, 1.0f));
    ImGui::PushStyleColor(
        ImGuiCol_PlotHistogram,
        ImVec4(0.18f, 0.67f, 1.0f, 1.0f));
    ImGui::PushStyleColor(
        ImGuiCol_FrameBg,
        ImVec4(0.07f, 0.14f, 0.23f, 1.0f));
    ImGui::Begin(
        "###UGCEditorTutorial",
        nullptr,
        tutorialWindowFlags);

    ImGui::SetWindowFontScale(1.22f * uiScale);
    ImGui::TextUnformatted("操作練習");
    ImGui::SetWindowFontScale(uiScale);
    if (!isComplete) {
        const char* skipLabel = "練習をスキップ";
        const float skipButtonWidth = 138.0f * uiScale;
        ImGui::SameLine(
            ImGui::GetWindowContentRegionMax().x - skipButtonWidth);
        if (ImGui::Button(
                skipLabel,
                ImVec2(skipButtonWidth, 34.0f * uiScale))) {
            mEditorTutorial.Stop();
            mContext.game->FinishUGCEditorTutorial(false);
            ImGui::End();
            ImGui::PopStyleColor(7);
            ImGui::PopStyleVar(5);
            return;
        }
    }

    if (!isWelcome) {
        const std::string actionCountText =
            isComplete
            ? "完了"
            : std::to_string(mEditorTutorial.GetCurrentActionNumber()) +
                " / " + std::to_string(mEditorTutorial.GetActionCount());
        ImGui::TextColored(
            ImVec4(1.0f, 0.84f, 0.25f, 1.0f),
            "%s",
            actionCountText.c_str());
        ImGui::SameLine();
        ImGui::ProgressBar(
            mEditorTutorial.GetProgressRatio(),
            ImVec2(-1.0f, 12.0f * uiScale),
            "");
    }

    ImGui::Separator();
    const std::string instruction =
        mEditorTutorial.GetInstruction();
    ImGui::SetWindowFontScale(1.14f * uiScale);
    ImGui::TextWrapped("%s", instruction.c_str());
    ImGui::SetWindowFontScale(uiScale);

    if (isWelcome) {
        ImGui::SetCursorPosY(panelSize.y - 66.0f * uiScale);
        if (ImGui::Button(
                "練習をはじめる",
                ImVec2(220.0f * uiScale, 44.0f * uiScale))) {
            mEditorTutorial.AdvanceFromWelcome();
        }
    } else if (isComplete) {
        ImGui::SetCursorPosY(panelSize.y - 66.0f * uiScale);
        if (ImGui::Button(
                "通常のステージ作成へ",
                ImVec2(240.0f * uiScale, 44.0f * uiScale))) {
            mEditorTutorial.Stop();
            mContext.game->FinishUGCEditorTutorial(true);
        }
    }
    ImGui::End();
    ImGui::PopStyleColor(7);
    ImGui::PopStyleVar(5);
}
