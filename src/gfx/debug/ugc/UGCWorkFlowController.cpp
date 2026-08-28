#include "gfx/debug/ugc/UGCWorkFlowController.h"

#include "gfx/debug/ugc/UGCEditorMenuState.h"
#include "gfx/debug/ugc/UGCEditorToolState.h"
#include "imgui.h"

#include <utility>

UGCWorkFlowController::UGCWorkFlowController(
    DebugEditorContext& context,
    UGCEditorMenuState& menuState,
    UGCEditorToolState& toolState,
    std::function<void()> reloadSelectedWork)
    : mMenuState(menuState),
      mToolState(toolState),
      mWorkPanel(context, std::move(reloadSelectedWork))
{
}

void UGCWorkFlowController::RequestManagementOpen()
{
    mMenuState.RequestWorkManagementOpen();
}

bool UGCWorkFlowController::IsManagementOpen() const
{
    return mMenuState.HasWorkManagementOpenRequest() ||
        ImGui::IsPopupOpen(
            "作品管理###UGCWorkManagement",
            ImGuiPopupFlags_AnyPopupLevel);
}

void UGCWorkFlowController::DrawManagement()
{
    if (mMenuState.ConsumeWorkManagementOpenRequest()) {
        ImGui::OpenPopup("作品管理###UGCWorkManagement");
    }
    mWorkPanel.DrawManagement(mToolState.statusMessage);
}

void UGCWorkFlowController::StartVerification()
{
    mWorkPanel.StartVerification(mToolState.statusMessage);
}

void UGCWorkFlowController::DrawBrowser()
{
    mWorkPanel.DrawBrowser();
}

bool UGCWorkFlowController::CompleteVerification(
    const std::string& workFileName)
{
    return mWorkPanel.CompleteVerification(workFileName);
}

bool UGCWorkFlowController::HasUnsavedChanges() const
{
    return mWorkPanel.HasUnsavedChanges();
}
