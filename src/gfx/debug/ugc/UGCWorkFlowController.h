#pragma once

#include "gfx/debug/ugc/UGCWorkPanel.h"

#include <functional>
#include <string>

class UGCEditorMenuState;
class UGCEditorToolState;
struct DebugEditorContext;

class UGCWorkFlowController {
public:
    UGCWorkFlowController(
        DebugEditorContext& context,
        UGCEditorMenuState& menuState,
        UGCEditorToolState& toolState,
        std::function<void()> reloadSelectedWork);

    void RequestManagementOpen();
    bool IsManagementOpen() const;
    void DrawManagement();
    void StartVerification();
    void DrawBrowser();
    bool CompleteVerification(const std::string& workFileName);
    bool HasUnsavedChanges() const;

private:
    UGCEditorMenuState& mMenuState;
    UGCEditorToolState& mToolState;
    UGCWorkPanel mWorkPanel;
};
