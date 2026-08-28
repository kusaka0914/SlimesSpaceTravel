#include "gfx/debug/DebugBuildRestartPanel.h"

#include "Game.h"
#include "imgui.h"

DebugBuildRestartPanel::DebugBuildRestartPanel(
    DebugEditorContext& context)
    : mContext(context)
{
}

void DebugBuildRestartPanel::SetStatus(
    const std::string& message,
    bool isError)
{
    mStatusMessage = message;
    mIsStatusError = isError;
}

void DebugBuildRestartPanel::Draw()
{
    if (ImGui::Button("Build & Restart")) {
        std::string restartErrorMessage;
        if (!mContext.game ||
            !mContext.game->RequestEditorBuildAndRestart(restartErrorMessage)) {
            SetStatus(restartErrorMessage, true);
        } else {
            SetStatus("Building...", false);
        }
    }

    if (mStatusMessage.empty()) {
        return;
    }

    ImGui::SameLine();
    const ImVec4 statusColor = mIsStatusError
        ? ImVec4(1.0f, 0.38f, 0.32f, 1.0f)
        : ImVec4(0.42f, 0.88f, 0.50f, 1.0f);
    ImGui::TextColored(statusColor, "%s", mStatusMessage.c_str());
}
